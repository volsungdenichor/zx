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

inline surface_t::size_type measure_multiline_text(const string_t& content)
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
void render_multiline_text(const string_t& content, const surface_t::size_type& size, PutCellFn put_cell)
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

        put_cell(surface_t::location_type{ row, col }, cp);
        ++col;
    }
}

}  // namespace detail

class label_model : public widget_t::interface
{
public:
    struct config_t
    {
        style_t normal_style     = style_t{};
        style_t focused_style = style_t{ none, none, font_t::inverse };
    };

    explicit label_model(string_t content, config_t cfg = config_t{}) : m_content(std::move(content)), m_cfg(std::move(cfg))
    {
    }

    const string_t& content() const { return m_content; }

    surface_t::size_type preferred_size() const override { return detail::measure_multiline_text(m_content); }

    bool is_focused() const override { return m_focused; }

    void set_focused(bool focused) override { m_focused = focused; }

    void render(surface_t::mut_view_type view) const override
    {
        const style_t& style = m_focused ? m_cfg.focused_style : m_cfg.normal_style;

        detail::render_multiline_text(
            m_content,
            view.size(),
            [&](const surface_t::location_type& loc, const code_point_t& cp) {
                view[loc] = cell_t{ cp, style };
            });
    }

    void on_attach(subscription_bus_t& bus) override { bus.subscribe<set_content_t>(id(), std::ref(*this)); }

    void on_detach(subscription_bus_t& bus) override { bus.unsubscribe_subscriber(id()); }

    void operator()(subscription_bus_t::control_t&, const set_content_t& event) { m_content = event.value; }

private:
    string_t m_content;
    config_t m_cfg;
    bool m_focused = false;
};

inline widget_t label(string_t content, label_model::config_t cfg = label_model::config_t{})
{
    return widget_t::make<label_model>(std::move(content), std::move(cfg));
}

}  // namespace widgets
}  // namespace ansi
}  // namespace zx
