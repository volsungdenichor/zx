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
    mat::extent_base_t cols = 0;
    mat::extent_base_t current_cols = 0;
    mat::extent_base_t rows = 1;

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

    location_t loc{ 0, 0 };

    for (const auto& cp : content)
    {
        if (loc[0] >= size[0])
        {
            break;
        }

        if (cp.m_data == U'\n')
        {
            ++loc[0];
            loc[1] = 0;
            continue;
        }

        if (loc[1] >= size[1])
        {
            ++loc[0];
            loc[1] = 0;
        }

        if (loc[0] >= size[0])
        {
            break;
        }

        put_cell(loc, cp);
        ++loc[1];
    }
}

}  // namespace detail

struct label_fn
{
    struct config_t
    {
        style_t normal_style = {};
        style_t focused_style = {};
    };

    class model_t : public widget_t::interface_t
    {
    public:
        explicit model_t(string_t content, config_t cfg) : m_content(std::move(content)), m_cfg(std::move(cfg)) { }

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

        void on_attach(message_bus_t&) override { }

        void on_detach(message_bus_t&) override { }

    private:
        string_t m_content;
        config_t m_cfg;
        bool m_focused = false;
    };

    inline widget_t operator()(string_t content, config_t cfg) const
    {
        return widget_t::make<model_t>(std::move(content), std::move(cfg));
    }

    inline widget_t operator()(string_t content) const { return widget_t::make<model_t>(std::move(content), config_t{}); }
};

constexpr auto label = label_fn{};

}  // namespace widgets
}  // namespace ansi
}  // namespace zx
