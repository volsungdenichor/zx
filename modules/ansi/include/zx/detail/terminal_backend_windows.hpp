#pragma once

#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <cstdio>
#include <cstring>
#include <zx/detail/terminal_backend_base.hpp>

namespace zx
{
namespace ansi
{
namespace detail
{

class windows_terminal_backend_t final : public terminal_backend_t
{
public:
    windows_terminal_backend_t() = default;

    void setup(const terminal_setup_options_t& options) override
    {
        m_stdin_handle = ::GetStdHandle(STD_INPUT_HANDLE);
        m_stdout_handle = ::GetStdHandle(STD_OUTPUT_HANDLE);

        if (m_stdin_handle != INVALID_HANDLE_VALUE && m_stdout_handle != INVALID_HANDLE_VALUE
            && ::GetConsoleMode(m_stdin_handle, &m_saved_stdin_mode)
            && ::GetConsoleMode(m_stdout_handle, &m_saved_stdout_mode))
        {
            DWORD input_mode = m_saved_stdin_mode;
            input_mode &= ~static_cast<DWORD>(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
            input_mode &= ~static_cast<DWORD>(ENABLE_QUICK_EDIT_MODE);
            input_mode |= static_cast<DWORD>(ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);
            ::SetConsoleMode(m_stdin_handle, input_mode);

            DWORD output_mode = m_saved_stdout_mode;
            output_mode |= static_cast<DWORD>(ENABLE_VIRTUAL_TERMINAL_PROCESSING);
            ::SetConsoleMode(m_stdout_handle, output_mode);

            m_raw_mode = true;
        }

        if (options.use_alt_screen)
        {
            write_escape("\033[?1049h");
            m_alt_screen = true;
        }
        if (options.hide_cursor)
        {
            write_escape("\033[?25l");
        }

        set_mouse_reporting(options.mouse);

        write_escape("\033[2J");
    }

    void set_mouse_reporting(const mouse_reporting_options_t& options) override { m_mouse_reporting = options; }

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

        if (m_stdin_handle != INVALID_HANDLE_VALUE)
        {
            ::SetConsoleMode(m_stdin_handle, m_saved_stdin_mode);
        }
        if (m_stdout_handle != INVALID_HANDLE_VALUE)
        {
            ::SetConsoleMode(m_stdout_handle, m_saved_stdout_mode);
        }
    }

    int wait_for_input(int tick_ms) const override
    {
        if (m_stdin_handle == INVALID_HANDLE_VALUE)
        {
            return -1;
        }

        const DWORD timeout_ms = (tick_ms > 0) ? static_cast<DWORD>(tick_ms) : INFINITE;
        const DWORD result = ::WaitForSingleObject(m_stdin_handle, timeout_ms);
        if (result == WAIT_OBJECT_0)
        {
            return 1;
        }
        if (result == WAIT_TIMEOUT)
        {
            return 0;
        }
        return -1;
    }

    bool should_retry_wait(int wait_result) const override
    {
        (void)wait_result;
        return false;
    }

    mat::vector_t<2, std::ptrdiff_t> get_terminal_size() const override
    {
        CONSOLE_SCREEN_BUFFER_INFO csbi{};
        if (m_stdout_handle != INVALID_HANDLE_VALUE && ::GetConsoleScreenBufferInfo(m_stdout_handle, &csbi))
        {
            return { static_cast<std::ptrdiff_t>(csbi.srWindow.Bottom - csbi.srWindow.Top + 1),
                     static_cast<std::ptrdiff_t>(csbi.srWindow.Right - csbi.srWindow.Left + 1) };
        }
        else
        {
            return { 24, 80 };
        }
    }

    std::optional<event_t> read_event(std::atomic<bool>& resize_pending) override
    {
        if (m_stdin_handle == INVALID_HANDLE_VALUE)
        {
            return std::nullopt;
        }

        while (true)
        {
            DWORD pending = 0;
            if (::GetNumberOfConsoleInputEvents(m_stdin_handle, &pending) == FALSE || pending == 0)
            {
                return std::nullopt;
            }

            INPUT_RECORD rec{};
            DWORD read_count = 0;
            if (::ReadConsoleInputW(m_stdin_handle, &rec, 1, &read_count) == FALSE || read_count == 0)
            {
                return std::nullopt;
            }

            if (rec.EventType == WINDOW_BUFFER_SIZE_EVENT)
            {
                resize_pending.store(true);
                continue;
            }

            if (rec.EventType == MOUSE_EVENT)
            {
                if (!m_mouse_reporting.button && !m_mouse_reporting.drag && !m_mouse_reporting.motion)
                {
                    continue;
                }

                const MOUSE_EVENT_RECORD& mouse_rec = rec.Event.MouseEvent;
                const key_modifiers_t modifiers = make_key_modifiers(mouse_rec.dwControlKeyState);
                const std::ptrdiff_t x = static_cast<std::ptrdiff_t>(mouse_rec.dwMousePosition.X);
                const std::ptrdiff_t y = static_cast<std::ptrdiff_t>(mouse_rec.dwMousePosition.Y);
                const auto loc = mat::vector_t<2, std::ptrdiff_t>{ y, x };

                if (mouse_rec.dwEventFlags == MOUSE_MOVED)
                {
                    if (!m_mouse_reporting.motion)
                    {
                        continue;
                    }
                    return mouse_event_t{ mouse_event_kind_t::move, mouse_button_t::none, loc, 0, modifiers };
                }

                if (mouse_rec.dwEventFlags == MOUSE_WHEELED)
                {
                    const SHORT wheel_delta = static_cast<SHORT>(HIWORD(mouse_rec.dwButtonState));
                    const int delta_y = (wheel_delta > 0) ? 1 : (wheel_delta < 0) ? -1 : 0;
                    return mouse_event_t{ mouse_event_kind_t::scroll, mouse_button_t::none, loc, delta_y, modifiers };
                }

                if (mouse_rec.dwEventFlags == 0)
                {
                    const mouse_button_t prev_btn = decode_mouse_button(m_last_mouse_button_state);
                    const mouse_button_t curr_btn = decode_mouse_button(mouse_rec.dwButtonState);

                    if (mouse_rec.dwButtonState == 0 && m_last_mouse_button_state != 0)
                    {
                        m_last_mouse_button_state = mouse_rec.dwButtonState;
                        if (!m_mouse_reporting.button)
                        {
                            continue;
                        }
                        return mouse_event_t{ mouse_event_kind_t::up, prev_btn, loc, 0, modifiers };
                    }

                    if (mouse_rec.dwButtonState != 0
                        && (m_last_mouse_button_state == 0 || m_last_mouse_button_state != mouse_rec.dwButtonState))
                    {
                        m_last_mouse_button_state = mouse_rec.dwButtonState;
                        if (!m_mouse_reporting.button)
                        {
                            continue;
                        }
                        return mouse_event_t{ mouse_event_kind_t::down, curr_btn, loc, 0, modifiers };
                    }

                    if (mouse_rec.dwButtonState != 0)
                    {
                        m_last_mouse_button_state = mouse_rec.dwButtonState;
                        if (!m_mouse_reporting.drag)
                        {
                            continue;
                        }
                        return mouse_event_t{ mouse_event_kind_t::drag, curr_btn, loc, 0, modifiers };
                    }
                }

                continue;
            }

            if (rec.EventType != KEY_EVENT || !rec.Event.KeyEvent.bKeyDown)
            {
                continue;
            }

            const KEY_EVENT_RECORD& key_rec = rec.Event.KeyEvent;
            const WORD vk = key_rec.wVirtualKeyCode;
            const key_modifiers_t modifiers = make_key_modifiers(key_rec.dwControlKeyState);

            switch (vk)
            {
                case VK_ESCAPE: return key_event_t{ key_t::escape, modifiers };
                case VK_RETURN: return key_event_t{ key_t::enter, modifiers };
                case VK_TAB:
                    return (key_rec.dwControlKeyState & SHIFT_PRESSED) ? key_event_t{ key_t::backtab, modifiers }
                                                                       : key_event_t{ key_t::tab, modifiers };
                case VK_BACK: return key_event_t{ key_t::backspace, modifiers };
                case VK_UP: return key_event_t{ key_t::up, modifiers };
                case VK_DOWN: return key_event_t{ key_t::down, modifiers };
                case VK_LEFT: return key_event_t{ key_t::left, modifiers };
                case VK_RIGHT: return key_event_t{ key_t::right, modifiers };
                case VK_HOME: return key_event_t{ key_t::home, modifiers };
                case VK_END: return key_event_t{ key_t::end, modifiers };
                case VK_PRIOR: return key_event_t{ key_t::page_up, modifiers };
                case VK_NEXT: return key_event_t{ key_t::page_down, modifiers };
                case VK_DELETE: return key_event_t{ key_t::del, modifiers };
                case VK_F1: return key_event_t{ key_t::f1, modifiers };
                case VK_F2: return key_event_t{ key_t::f2, modifiers };
                case VK_F3: return key_event_t{ key_t::f3, modifiers };
                case VK_F4: return key_event_t{ key_t::f4, modifiers };
                case VK_F5: return key_event_t{ key_t::f5, modifiers };
                case VK_F6: return key_event_t{ key_t::f6, modifiers };
                case VK_F7: return key_event_t{ key_t::f7, modifiers };
                case VK_F8: return key_event_t{ key_t::f8, modifiers };
                case VK_F9: return key_event_t{ key_t::f9, modifiers };
                case VK_F10: return key_event_t{ key_t::f10, modifiers };
                case VK_F11: return key_event_t{ key_t::f11, modifiers };
                case VK_F12: return key_event_t{ key_t::f12, modifiers };
            }

            const wchar_t wc = key_rec.uChar.UnicodeChar;
            const bool ctrl = has(modifiers, key_modifiers_t::ctrl);

            if (ctrl && wc >= 0x0001 && wc <= 0x001A)
            {
                if (wc == 0x0003)
                {
                    return quit_event_t{};
                }
                return key_event_t{ code_point_t{ static_cast<char32_t>('a' + wc - 1) }, modifiers };
            }

            if (wc != 0)
            {
                return key_event_t{ code_point_t{ static_cast<char32_t>(wc) }, modifiers };
            }
        }
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
    static key_modifiers_t make_key_modifiers(DWORD mods)
    {
        key_modifiers_t out = key_modifiers_t::none;
        if ((mods & SHIFT_PRESSED) != 0)
        {
            out |= key_modifiers_t::shift;
        }
        if ((mods & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0)
        {
            out |= key_modifiers_t::ctrl;
        }
        if ((mods & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) != 0)
        {
            out |= key_modifiers_t::alt;
        }
        return out;
    }

    static mouse_button_t decode_mouse_button(DWORD state)
    {
        if (state & FROM_LEFT_1ST_BUTTON_PRESSED)
        {
            return mouse_button_t::left;
        }
        if (state & FROM_LEFT_2ND_BUTTON_PRESSED)
        {
            return mouse_button_t::middle;
        }
        if (state & RIGHTMOST_BUTTON_PRESSED)
        {
            return mouse_button_t::right;
        }
        return mouse_button_t::none;
    }

    HANDLE m_stdin_handle = INVALID_HANDLE_VALUE;
    HANDLE m_stdout_handle = INVALID_HANDLE_VALUE;
    DWORD m_saved_stdin_mode = 0;
    DWORD m_saved_stdout_mode = 0;
    DWORD m_last_mouse_button_state = 0;
    mouse_reporting_options_t m_mouse_reporting = {};
    bool m_raw_mode = false;
    bool m_alt_screen = false;
};

}  // namespace detail
}  // namespace ansi
}  // namespace zx

#endif
