#pragma once

#ifndef _WIN32

#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <vector>
#include <zx/detail/terminal_backend_base.hpp>

namespace zx
{
namespace ansi
{
namespace detail
{

class posix_terminal_backend_t final : public terminal_backend_t
{
public:
    posix_terminal_backend_t() = default;

    void setup(const terminal_setup_options_t& options) override
    {
        m_seen_resize_epoch = s_resize_epoch;

        struct sigaction sa = {};

        sa.sa_handler = &posix_terminal_backend_t::on_sigwinch;
        ::sigaction(SIGWINCH, &sa, nullptr);

        ::tcgetattr(STDIN_FILENO, &m_saved_termios);
        struct termios raw = m_saved_termios;
        raw.c_lflag &= ~static_cast<tcflag_t>(ECHO | ICANON | ISIG | IEXTEN);
        raw.c_iflag &= ~static_cast<tcflag_t>(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
        raw.c_cflag |= static_cast<tcflag_t>(CS8);
        raw.c_oflag &= ~static_cast<tcflag_t>(OPOST);
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        ::tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
        m_raw_mode = true;

        if (options.use_alt_screen)
        {
            write_escape("\033[?1049h");
            m_alt_screen = true;
        }
        if (options.hide_cursor)
        {
            write_escape("\033[?25l");
        }

        apply_mouse_reporting(options.mouse);

        write_escape("\033[2J");
    }

    void set_mouse_reporting(const mouse_reporting_options_t& options) override { apply_mouse_reporting(options); }

    void cleanup(bool hide_cursor) override
    {
        if (!m_raw_mode)
        {
            return;
        }

        m_raw_mode = false;

        write_escape("\033[0m");

        if (hide_cursor)
        {
            write_escape("\033[?25h");
        }

        if (m_alt_screen)
        {
            write_escape("\033[?1049l");
            m_alt_screen = false;
        }

        write_escape("\033[?1006l");
        write_escape("\033[?1015l");
        write_escape("\033[?1005l");
        write_escape("\033[?1002l");
        write_escape("\033[?1000l");

        ::tcsetattr(STDIN_FILENO, TCSAFLUSH, &m_saved_termios);
    }

    int wait_for_input(int tick_ms) const override
    {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        struct timeval tv = {};
        struct timeval* tvp = nullptr;
        if (tick_ms > 0)
        {
            std::tie(tv.tv_sec, tv.tv_usec) = std::make_tuple(tick_ms / 1000, (tick_ms % 1000) * 1000);
            tvp = &tv;
        }

        return ::select(STDIN_FILENO + 1, &read_fds, nullptr, nullptr, tvp);
    }

    bool should_retry_wait(int wait_result) const override { return wait_result < 0 && errno == EINTR; }

    mat::vector_t<std::ptrdiff_t, 2> get_terminal_size() const override
    {
        struct winsize ws = {};

        if (::ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0)
        {
            return { ws.ws_row, ws.ws_col };
        }
        else
        {
            return { 24, 80 };
        }
    }

    std::optional<event_t> read_event(std::atomic<bool>& resize_pending) override
    {
        if (m_seen_resize_epoch != s_resize_epoch)
        {
            m_seen_resize_epoch = s_resize_epoch;
            resize_pending.store(true);
        }

        std::uint8_t buf[32];
        const ssize_t n = ::read(STDIN_FILENO, buf, sizeof(buf));
        if (n <= 0)
        {
            return std::nullopt;
        }

        std::vector<std::uint8_t> packet;
        if (!m_pending_escape_seq.empty())
        {
            packet = m_pending_escape_seq;
            m_pending_escape_seq.clear();
            packet.insert(packet.end(), buf, buf + n);
        }
        else
        {
            packet.assign(buf, buf + n);
        }

        auto store_if_incomplete_escape = [&](const std::vector<std::uint8_t>& seq) -> bool
        {
            if (seq.empty() || seq[0] != 0x1B)
            {
                return false;
            }

            if (seq.size() == 1)
            {
                m_pending_escape_seq = seq;
                return true;
            }

            if (seq.size() >= 3 && seq[1] == '[' && seq[2] == '<')
            {
                if (std::find(seq.begin(), seq.end(), static_cast<std::uint8_t>('M')) == seq.end()
                    && std::find(seq.begin(), seq.end(), static_cast<std::uint8_t>('m')) == seq.end())
                {
                    m_pending_escape_seq = seq;
                    return true;
                }
            }

            if (seq.size() >= 3 && seq[1] == '[' && seq[2] == 'M' && seq.size() < 6)
            {
                m_pending_escape_seq = seq;
                return true;
            }

            if (seq.size() < 3 && (seq[1] == '[' || seq[1] == 'O'))
            {
                m_pending_escape_seq = seq;
                return true;
            }

            return false;
        };

        ssize_t total = static_cast<ssize_t>(packet.size());

        if (packet[0] == 0x1B)
        {
            if (total == 1)
            {
                fd_set read_fds;
                FD_ZERO(&read_fds);
                FD_SET(STDIN_FILENO, &read_fds);

                struct timeval tv = {};
                tv.tv_sec = 0;
                tv.tv_usec = 10000;

                const int ready = ::select(STDIN_FILENO + 1, &read_fds, nullptr, nullptr, &tv);
                if (ready > 0)
                {
                    std::uint8_t extra_buf[32];
                    const ssize_t extra = ::read(STDIN_FILENO, extra_buf, sizeof(extra_buf));
                    if (extra > 0)
                    {
                        packet.insert(packet.end(), extra_buf, extra_buf + extra);
                        total = static_cast<ssize_t>(packet.size());
                    }
                }

                if (total == 1)
                {
                    return key_event_t{ key_t::escape };
                }
            }

            while (total < 256)
            {
                fd_set read_fds;
                FD_ZERO(&read_fds);
                FD_SET(STDIN_FILENO, &read_fds);

                struct timeval tv = {};
                tv.tv_sec = 0;
                tv.tv_usec = 0;

                const int ready = ::select(STDIN_FILENO + 1, &read_fds, nullptr, nullptr, &tv);
                if (ready <= 0)
                {
                    break;
                }

                std::uint8_t extra_buf[32];
                const ssize_t extra = ::read(STDIN_FILENO, extra_buf, sizeof(extra_buf));
                if (extra <= 0)
                {
                    break;
                }
                packet.insert(packet.end(), extra_buf, extra_buf + extra);
                total = static_cast<ssize_t>(packet.size());
            }

            if (store_if_incomplete_escape(packet))
            {
                return std::nullopt;
            }
        }

        const auto* bytes = packet.data();
        total = static_cast<ssize_t>(packet.size());

        const auto preserve_tail_from_next_escape = [&](ssize_t consumed)
        {
            if (consumed >= total)
            {
                return;
            }

            const auto* tail_begin = bytes + consumed;
            const auto* tail_end = bytes + total;
            const auto* next_escape = std::find(tail_begin, tail_end, static_cast<std::uint8_t>(0x1B));
            if (next_escape != tail_end)
            {
                m_pending_escape_seq.assign(next_escape, tail_end);
            }
        };

        if (bytes[0] == 0x1B)
        {
            ssize_t mouse_consumed = 0;
            if (auto me = parse_sgr_mouse(bytes, total, mouse_consumed); me)
            {
                preserve_tail_from_next_escape(mouse_consumed);
                return *me;
            }

            if (auto me = parse_legacy_mouse(bytes, total, mouse_consumed); me)
            {
                preserve_tail_from_next_escape(mouse_consumed);
                return *me;
            }

            if (total >= 3 && bytes[1] == '[' && (bytes[2] == '<' || bytes[2] == 'M'))
            {
                if (store_if_incomplete_escape(packet))
                {
                    return std::nullopt;
                }

                preserve_tail_from_next_escape(0);
                return std::nullopt;
            }

            if (total == 1)
            {
                return key_event_t{ key_t::escape };
            }

            if (total >= 3 && bytes[1] == '[')
            {
                const auto flush_tail_after_csi3 = [&]()
                {
                    if (total > 3)
                    {
                        m_pending_escape_seq.assign(bytes + 3, bytes + total);
                    }
                };

                const bool has_exact_csi3 = (total == 3);
                const bool has_csi3_with_following_packet = (total > 3 && bytes[3] == 0x1B);

                if (has_exact_csi3 || has_csi3_with_following_packet)
                {
                    switch (bytes[2])
                    {
                        case 'A': flush_tail_after_csi3(); return key_event_t{ key_t::up };
                        case 'B': flush_tail_after_csi3(); return key_event_t{ key_t::down };
                        case 'C': flush_tail_after_csi3(); return key_event_t{ key_t::right };
                        case 'D': flush_tail_after_csi3(); return key_event_t{ key_t::left };
                        case 'H': flush_tail_after_csi3(); return key_event_t{ key_t::home };
                        case 'F': flush_tail_after_csi3(); return key_event_t{ key_t::end };
                        case 'Z': flush_tail_after_csi3(); return key_event_t{ key_t::backtab };
                    }
                }

                if (total >= 4 && bytes[total - 1] == '~')
                {
                    int code = 0;
                    bool have_code = false;
                    for (ssize_t i = 2; i < total - 1; ++i)
                    {
                        if (bytes[i] >= '0' && bytes[i] <= '9')
                        {
                            have_code = true;
                            code = code * 10 + (bytes[i] - '0');
                            continue;
                        }
                        if (bytes[i] == ';')
                        {
                            break;
                        }
                        have_code = false;
                        break;
                    }

                    if (have_code)
                    {
                        switch (code)
                        {
                            case 1: return key_event_t{ key_t::home };
                            case 3: return key_event_t{ key_t::del };
                            case 4: return key_event_t{ key_t::end };
                            case 5: return key_event_t{ key_t::page_up };
                            case 6: return key_event_t{ key_t::page_down };
                            case 11: return key_event_t{ key_t::f1 };
                            case 12: return key_event_t{ key_t::f2 };
                            case 13: return key_event_t{ key_t::f3 };
                            case 14: return key_event_t{ key_t::f4 };
                            case 15: return key_event_t{ key_t::f5 };
                            case 17: return key_event_t{ key_t::f6 };
                            case 18: return key_event_t{ key_t::f7 };
                            case 19: return key_event_t{ key_t::f8 };
                            case 20: return key_event_t{ key_t::f9 };
                            case 21: return key_event_t{ key_t::f10 };
                            case 23: return key_event_t{ key_t::f11 };
                            case 24: return key_event_t{ key_t::f12 };
                        }
                    }
                }

                preserve_tail_from_next_escape(0);
                return std::nullopt;
            }

            if (total >= 3 && bytes[1] == 'O')
            {
                switch (bytes[2])
                {
                    case 'P': return key_event_t{ key_t::f1 };
                    case 'Q': return key_event_t{ key_t::f2 };
                    case 'R': return key_event_t{ key_t::f3 };
                    case 'S': return key_event_t{ key_t::f4 };
                }
            }

            if (total == 2 && bytes[1] >= 0x20)
            {
                return key_event_t{ code_point_t{ static_cast<char32_t>(bytes[1]) }, key_modifiers_t::ctrl };
            }

            return std::nullopt;
        }

        if (total >= 2 && bytes[0] == '[' && (bytes[1] == '<' || bytes[1] == 'M'))
        {
            std::vector<std::uint8_t> escaped_packet;
            escaped_packet.reserve(static_cast<std::size_t>(total) + 1);
            escaped_packet.push_back(0x1B);
            escaped_packet.insert(escaped_packet.end(), bytes, bytes + total);

            ssize_t mouse_consumed = 0;
            if (auto me
                = parse_sgr_mouse(escaped_packet.data(), static_cast<ssize_t>(escaped_packet.size()), mouse_consumed);
                me)
            {
                const ssize_t consumed_without_escape = std::max<ssize_t>(0, mouse_consumed - 1);
                preserve_tail_from_next_escape(consumed_without_escape);
                return *me;
            }

            if (auto me
                = parse_legacy_mouse(escaped_packet.data(), static_cast<ssize_t>(escaped_packet.size()), mouse_consumed);
                me)
            {
                const ssize_t consumed_without_escape = std::max<ssize_t>(0, mouse_consumed - 1);
                preserve_tail_from_next_escape(consumed_without_escape);
                return *me;
            }

            return std::nullopt;
        }

        switch (bytes[0])
        {
            case '\r':
            case '\n': return key_event_t{ key_t::enter };
            case '\t': return key_event_t{ key_t::tab };
            case 127: return key_event_t{ key_t::backspace };
            case 8: return key_event_t{ key_t::backspace };
        }

        if (bytes[0] >= 0x01 && bytes[0] <= 0x1A)
        {
            if (bytes[0] == 0x03)
            {
                return quit_event_t{};
            }
            return key_event_t{ code_point_t{ static_cast<char32_t>('a' + bytes[0] - 1) }, key_modifiers_t::ctrl };
        }

        char32_t cp = 0;
        std::size_t pos = 0;
        std::uint8_t c = bytes[pos++];
        if (c < 0x80)
        {
            cp = c;
        }
        else
        {
            const int extra = (c < 0xE0) ? 1 : (c < 0xF0) ? 2 : 3;
            cp = c & (0x7F >> extra);
            while (pos < static_cast<std::size_t>(total) && pos <= static_cast<std::size_t>(extra))
            {
                cp = (cp << 6) | (bytes[pos++] & 0x3F);
            }
        }
        return key_event_t{ code_point_t{ cp } };
    }

    void write_escape(const char* seq) const override
    {
        std::fwrite(seq, 1, std::strlen(seq), stdout);
        std::fflush(stdout);
    }

    void write_output(const char* data, std::size_t size) const override
    {
        std::fwrite(data, 1, size, stdout);
        std::fflush(stdout);
    }

private:
    void apply_mouse_reporting(const mouse_reporting_options_t& options) const
    {
        write_escape("\033[?1015l");
        write_escape("\033[?1005l");
        write_escape(options.button ? "\033[?1000h" : "\033[?1000l");
        write_escape(options.drag ? "\033[?1002h" : "\033[?1002l");
        write_escape(options.motion ? "\033[?1003h" : "\033[?1003l");
        write_escape(options.sgr ? "\033[?1006h" : "\033[?1006l");
    }

    static mouse_button_t decode_mouse_button(int code)
    {
        switch (code)
        {
            case 0: return mouse_button_t::left;
            case 1: return mouse_button_t::middle;
            case 2: return mouse_button_t::right;
            default: return mouse_button_t::none;
        }
    }

    static std::optional<mouse_event_t> parse_sgr_mouse(const std::uint8_t* buf, ssize_t n, ssize_t& consumed)
    {
        consumed = 0;

        if (n < 3 || buf[0] != 0x1B || buf[1] != '[' || buf[2] != '<')
        {
            return std::nullopt;
        }

        int parts[3] = { 0, 0, 0 };
        int part_idx = 0;
        int value = 0;
        bool have_digit = false;
        std::uint8_t final = 0;

        for (ssize_t i = 3; i < n; ++i)
        {
            const std::uint8_t ch = buf[i];
            if (ch >= '0' && ch <= '9')
            {
                value = value * 10 + static_cast<int>(ch - '0');
                have_digit = true;
                continue;
            }

            if (ch == ';')
            {
                if (!have_digit || part_idx >= 3)
                {
                    return std::nullopt;
                }
                parts[part_idx++] = value;
                value = 0;
                have_digit = false;
                continue;
            }

            if (ch == 'M' || ch == 'm')
            {
                if (!have_digit || part_idx != 2)
                {
                    return std::nullopt;
                }
                parts[2] = value;
                final = ch;
                consumed = i + 1;
                break;
            }

            return std::nullopt;
        }

        if (consumed == 0)
        {
            return std::nullopt;
        }

        const int cb = parts[0];
        const int x = (parts[1] > 0) ? (parts[1] - 1) : 0;
        const int y = (parts[2] > 0) ? (parts[2] - 1) : 0;
        const auto pos = mat::vector_t<std::ptrdiff_t, 2>{ y, x };

        const bool shift = (cb & 0x04) != 0;
        const bool alt = (cb & 0x08) != 0;
        const bool ctrl = (cb & 0x10) != 0;
        const bool motion = (cb & 0x20) != 0;
        const bool wheel = (cb & 0x40) != 0;
        const int button_code = (cb & 0x03);
        const auto modifiers = key_modifiers_t{ (shift ? key_modifiers_t::shift : key_modifiers_t::none)
                                                | (ctrl ? key_modifiers_t::ctrl : key_modifiers_t::none)
                                                | (alt ? key_modifiers_t::alt : key_modifiers_t::none) };
        const mouse_button_t btn = decode_mouse_button(button_code);

        if (wheel)
        {
            const int delta_y = (button_code == 0) ? 1 : (button_code == 1) ? -1 : 0;
            return mouse_event_t{ mouse_event_kind_t::scroll, mouse_button_t::none, pos, delta_y, modifiers };
        }

        if (motion)
        {
            if (btn != mouse_button_t::none)
            {
                return mouse_event_t{ mouse_event_kind_t::drag, btn, pos, 0, modifiers };
            }
            return mouse_event_t{ mouse_event_kind_t::move, mouse_button_t::none, pos, 0, modifiers };
        }

        if (final == 'm')
        {
            return mouse_event_t{ mouse_event_kind_t::up, btn, pos, 0, modifiers };
        }

        if (btn != mouse_button_t::none)
        {
            return mouse_event_t{ mouse_event_kind_t::down, btn, pos, 0, modifiers };
        }
        return mouse_event_t{ mouse_event_kind_t::move, mouse_button_t::none, pos, 0, modifiers };
    }

    static std::optional<mouse_event_t> parse_legacy_mouse(const std::uint8_t* buf, ssize_t n, ssize_t& consumed)
    {
        consumed = 0;

        if (n < 6 || buf[0] != 0x1B || buf[1] != '[' || buf[2] != 'M')
        {
            return std::nullopt;
        }

        consumed = 6;

        const int cb = static_cast<int>(buf[3]) - 32;
        const int cx = static_cast<int>(buf[4]) - 32;
        const int cy = static_cast<int>(buf[5]) - 32;

        if (cb < 0 || cx <= 0 || cy <= 0)
        {
            return std::nullopt;
        }

        const auto pos = mat::vector_t<std::ptrdiff_t, 2>{ cy - 1, cx - 1 };

        const bool shift = (cb & 0x04) != 0;
        const bool alt = (cb & 0x08) != 0;
        const bool ctrl = (cb & 0x10) != 0;
        const bool motion = (cb & 0x20) != 0;
        const bool wheel = (cb & 0x40) != 0;
        const int button_code = (cb & 0x03);
        const auto modifiers = key_modifiers_t{ (shift ? key_modifiers_t::shift : key_modifiers_t::none)
                                                | (ctrl ? key_modifiers_t::ctrl : key_modifiers_t::none)
                                                | (alt ? key_modifiers_t::alt : key_modifiers_t::none) };
        const mouse_button_t btn = decode_mouse_button(button_code);

        if (wheel)
        {
            const int delta_y = (button_code == 0) ? 1 : (button_code == 1) ? -1 : 0;
            return mouse_event_t{ mouse_event_kind_t::scroll, mouse_button_t::none, pos, delta_y, modifiers };
        }

        if (motion)
        {
            if (btn != mouse_button_t::none)
            {
                return mouse_event_t{ mouse_event_kind_t::drag, btn, pos, 0, modifiers };
            }
            return mouse_event_t{ mouse_event_kind_t::move, mouse_button_t::none, pos, 0, modifiers };
        }

        if (button_code == 3)
        {
            return mouse_event_t{ mouse_event_kind_t::up, mouse_button_t::none, pos, 0, modifiers };
        }

        if (btn != mouse_button_t::none)
        {
            return mouse_event_t{ mouse_event_kind_t::down, btn, pos, 0, modifiers };
        }

        return std::nullopt;
    }

    static void on_sigwinch(int) { ++s_resize_epoch; }

private:
    struct termios m_saved_termios = {};
    std::sig_atomic_t m_seen_resize_epoch = 0;
    bool m_raw_mode = false;
    bool m_alt_screen = false;
    std::vector<std::uint8_t> m_pending_escape_seq;

    static volatile std::sig_atomic_t s_resize_epoch;
};

inline volatile std::sig_atomic_t posix_terminal_backend_t::s_resize_epoch = 0;

}  // namespace detail
}  // namespace ansi
}  // namespace zx

#endif
