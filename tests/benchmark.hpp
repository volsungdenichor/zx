#pragma once

#include <algorithm>
#include <cassert>
#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

struct benchmark
{
    struct result_t : public std::vector<std::pair<std::string, std::chrono::microseconds>>
    {
        friend std::ostream& operator<<(std::ostream& os, const result_t& item)
        {
            if (item.empty())
            {
                return os;
            }
            assert(std::is_sorted(
                item.begin(),
                item.end(),
                [](const auto& lhs, const auto& rhs) { return std::get<1>(lhs) < std::get<1>(rhs); }));
            const auto min = std::get<1>(item.front());
            for (std::size_t i = 0; i < item.size(); ++i)
            {
                const auto& [name, duration] = item[i];
                const auto ratio = static_cast<double>(duration.count()) / min.count();
                os << "| " << std::left << std::setw(32) << name << " | " << duration.count() << " | " << std::fixed
                   << std::setprecision(3) << ratio << " |" << std::endl;
            }
            return os;
        }
    };

    std::vector<std::pair<std::string, std::function<void()>>> m_funcs;

    benchmark& add(std::string name, std::function<void()> func)
    {
        m_funcs.emplace_back(std::move(name), std::move(func));
        return *this;
    }

    result_t run(int rep = 100) const
    {
        result_t result{};
        for (const auto& [name, func] : m_funcs)
        {
            const auto start = std::chrono::steady_clock::now();
            for (int i = 0; i < rep; ++i)
            {
                func();
            }
            const auto stop = std::chrono::steady_clock::now();
            result.emplace_back(name, std::chrono::duration_cast<std::chrono::microseconds>((stop - start) / rep));
        }
        std::sort(
            result.begin(),
            result.end(),
            [](const auto& lhs, const auto& rhs) { return std::get<1>(lhs) < std::get<1>(rhs); });
        return result;
    }
};
