#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <utility>
#include <zx/message_bus.hpp>
#include <zx/string.hpp>

namespace zx
{
namespace ansi
{
namespace widgets
{

struct set_content_t
{
    string_t value;
};

namespace detail
{

inline extent_t measure_multiline_text(const string_t& content)
{
    std::ptrdiff_t cols = 0;
    std::ptrdiff_t current_cols = 0;
    std::ptrdiff_t rows = 1;

    for (const auto& cp : content)
    {
        if (cp.m_data == U'\n')
        {
            cols = std::max(cols, current_cols);
            current_cols = 0;
            ++rows;
        }
        else
        {
            ++current_cols;
        }
    }

    cols = std::max(cols, current_cols);
    return { rows, cols };
}

template <class PutCellFn>
void render_multiline_text(const string_t& content, const extent_t& size, PutCellFn put_cell)
{
    if (size[0] <= 0 || size[1] <= 0)
    {
        return;
    }

    std::ptrdiff_t row = 0;
    std::ptrdiff_t col = 0;

    for (const auto& cp : content)
    {
        if (row >= size[0])

        {
            break;
        }

        if (cp.m_data == U'\n')
        {
            ++row;
            col = 0;
            continue;
        }

        if (col >= size[1])
        {
            ++row;
            col = 0;
        }

        if (row >= size[0])
        {
            break;
        }

        put_cell(location_t{ row, col }, cp);
        ++col;
    }
}

}  // namespace detail

struct label_fn
{
    struct config_t
    {
        style_t normal_style;
        style_t focused_style;
    };

    class model_t : public widget_t::interface
    {
    public:
        explicit model_t(string_t content, config_t cfg = config_t{}) : m_content(std::move(content)), m_cfg(std::move(cfg))
        {
        }

        const string_t& content() const { return m_content; }

        extent_t preferred_size() const override { return detail::measure_multiline_text(m_content); }

        bool is_focused() const override { return m_focused; }

        void set_focused(bool focused) override { m_focused = focused; }

        void render(surface_t::mut_view_type view) const override
        {
            const style_t& style = m_focused ? m_cfg.focused_style : m_cfg.normal_style;

            detail::render_multiline_text(
                m_content,
                view.extent(),
                [&](const location_t& loc, const code_point_t& cp) {
                    view[loc] = cell_t{ cp, style };
                });
        }

        void on_attach() override { }

        void on_detach() override { }

    private:
        string_t m_content;
        config_t m_cfg;
        bool m_focused = false;
    };

    inline widget_t operator()(string_t content, config_t cfg = config_t{}) const
    {
        return widget_t::make<model_t>(std::move(content), std::move(cfg));
    }
};

constexpr auto label = label_fn{};

}  // namespace widgets
}  // namespace ansi
}  // namespace zx
