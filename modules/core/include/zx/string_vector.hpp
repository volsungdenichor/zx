#pragma once

#include <cassert>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace zx
{

class string_vector
{
    struct entry_t
    {
        std::size_t begin;
        std::size_t end;
    };

    struct impl_t
    {
        std::vector<char> m_buffer;
        std::vector<entry_t> m_entries;
    };

public:
    struct const_iterator
    {
        const impl_t* m_impl = {};
        std::size_t m_index = {};
        using value_type = std::string_view;
        using reference = std::string_view;
        struct pointer
        {
            std::string_view item;
            std::string_view* operator->() { return &item; }
        };
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::random_access_iterator_tag;

        const_iterator() = default;
        const_iterator(const impl_t& impl, std::size_t index) : m_impl{ &impl }, m_index{ index } { }
        const_iterator(const const_iterator&) = default;
        const_iterator(const_iterator&&) noexcept = default;
        const_iterator& operator=(const const_iterator&) = default;
        const_iterator& operator=(const_iterator&&) noexcept = default;

        friend bool operator==(const const_iterator& lhs, const const_iterator& rhs) { return lhs.m_index == rhs.m_index; }
        friend bool operator!=(const const_iterator& lhs, const const_iterator& rhs) { return !(lhs == rhs); }
        friend bool operator<(const const_iterator& lhs, const const_iterator& rhs) { return lhs.m_index < rhs.m_index; }
        friend bool operator>(const const_iterator& lhs, const const_iterator& rhs) { return rhs < lhs; }
        friend bool operator<=(const const_iterator& lhs, const const_iterator& rhs) { return !(rhs < lhs); }
        friend bool operator>=(const const_iterator& lhs, const const_iterator& rhs) { return !(lhs < rhs); }

        friend const_iterator& operator+=(const_iterator& self, difference_type n)
        {
            assert(self.m_impl);
            const auto next = static_cast<difference_type>(self.m_index) + n;
            assert(0 <= next && next <= static_cast<difference_type>(self.m_impl->m_entries.size()));
            self.m_index = static_cast<size_type>(next);
            return self;
        }

        friend const_iterator operator+(const_iterator self, difference_type n) { return self += n; }

        friend const_iterator& operator++(const_iterator& self) { return self += 1; }
        friend const_iterator operator++(const_iterator& self, int) { return self + 1; }
        friend const_iterator& operator--(const_iterator& self) { return self += -1; }
        friend const_iterator operator--(const_iterator& self, int) { return self - 1; }
        friend const_iterator& operator-=(const_iterator& self, difference_type n) { return self += -n; }
        friend const_iterator operator-(const_iterator self, difference_type n) { return self -= n; }

        friend difference_type operator-(const const_iterator& lhs, const const_iterator& rhs)
        {
            assert(lhs.m_impl == rhs.m_impl);
            return static_cast<difference_type>(lhs.m_index) - static_cast<difference_type>(rhs.m_index);
        }

        reference operator*() const
        {
            assert(m_impl);
            assert(m_index < m_impl->m_entries.size());
            const auto& entry = m_impl->m_entries[m_index];
            return std::string_view{ m_impl->m_buffer.data(), m_impl->m_buffer.size() }.substr(
                entry.begin, entry.end - entry.begin);
        }

        reference operator[](difference_type n) const { return *(*this + n); }

        pointer operator->() const { return pointer{ **this }; }
    };

    using value_type = const_iterator::value_type;
    using const_reference = const_iterator::reference;
    using reference = const_reference;
    using const_pointer = const_iterator::pointer;
    using pointer = const_pointer;
    using const_iterator = const_iterator;
    using iterator = const_iterator;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using reverse_iterator = const_reverse_iterator;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    string_vector() = default;
    string_vector(std::initializer_list<std::string_view> init)
    {
        for (std::string_view str : init)
        {
            push_back(str);
        }
    }
    string_vector(const string_vector&) = default;
    string_vector(string_vector&&) noexcept = default;
    string_vector& operator=(const string_vector&) = default;
    string_vector& operator=(string_vector&&) noexcept = default;

    void push_back(std::string_view str)
    {
        const auto begin = m_impl.m_entries.empty() ? 0 : m_impl.m_entries.back().end;
        m_impl.m_buffer.insert(m_impl.m_buffer.end(), str.begin(), str.end());
        m_impl.m_entries.push_back({ begin, begin + str.size() });
    }

    size_type size() const { return m_impl.m_entries.size(); }
    bool empty() const { return m_impl.m_entries.empty(); }

    const_iterator begin() const { return const_iterator{ m_impl, 0 }; }
    const_iterator end() const { return const_iterator{ m_impl, size() }; }
    const_reverse_iterator rbegin() const { return const_reverse_iterator{ end() }; }
    const_reverse_iterator rend() const { return const_reverse_iterator{ begin() }; }

    const_reference operator[](size_type index) const
    {
        if (index >= size())
        {
            throw std::out_of_range{ "string_vector::at: index out of range" };
        }
        return *(begin() + static_cast<difference_type>(index));
    }

    const_reference at(size_type index) const { return (*this)[index]; }

    const reference front() const
    {
        if (empty())
        {
            throw std::out_of_range{ "string_vector::front: vector is empty" };
        }
        return *begin();
    }

    const reference back() const
    {
        if (empty())
        {
            throw std::out_of_range{ "string_vector::back: vector is empty" };
        }
        return *std::prev(end());
    }

private:
    impl_t m_impl;
};

}  // namespace zx