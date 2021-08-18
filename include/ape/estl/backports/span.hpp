#pragma once

#ifndef APE_ESTL_BACKPORTS_SPAN_H
#define APE_ESTL_BACKPORTS_SPAN_H

/// reference: https://github.com/microsoft/STL/blob/main/stl/inc/span

#include <ape/config.hpp>
#if CPP_STANDARD >= CPP_STD_20
#include <span>
BEGIN_APE_NAMESPACE
namespace stl{
    using std::span;
    using std::dynamic_extent;
    using std::as_bytes;
    using std::as_writable_bytes;
}
END_APE_NAMESPACE
#else

#include <type_traits>
#include <iterator>
#include <array>

#include <gsl/gsl_assert>

BEGIN_APE_NAMESPACE
namespace stl{
inline constexpr std::size_t dynamic_extent = static_cast<std::size_t>(-1);

template <typename T, std::size_t Ext>
class span;

namespace detail
{
    template <typename T, std::size_t Ext>
    struct span_storage_
    {
        using pointer = T *;

        constexpr span_storage_() noexcept = default;

        constexpr explicit span_storage_(const pointer data, size_t) noexcept : data_{data} {}

        pointer data_{nullptr};
        static constexpr std::size_t size_ = Ext;
    };
    template <typename T>
    struct span_storage_<T, dynamic_extent>
    {
        using pointer = T *;

        constexpr span_storage_() noexcept = default;

        constexpr explicit span_storage_(const pointer data, size_t size) noexcept : data_{data}, size_(size) {}

        pointer data_{nullptr};
        std::size_t size_{0};
    };

    template <typename>
    struct is_span : std::false_type
    {
    };

    template <typename T, std::size_t Ext>
    struct is_span<span<T, Ext>> : std::true_type
    {
    };

    template <typename>
    struct is_std_array : std::false_type
    {
    };

    template <typename T, std::size_t N>
    struct is_std_array<std::array<T, N>> : std::true_type
    {
    };

    template <typename Rng, typename T>
    struct is_span_convertible_range
        : std::bool_constant<std::is_convertible_v<std::remove_pointer_t<decltype(std::data(std::declval<Rng &>()))> (*)[], T (*)[]>>
    {
    };

    template <typename, typename = void>
    struct has_container_interface : std::false_type
    {
    };

    template <typename Rng>
    struct has_container_interface<Rng,
                                   std::void_t<decltype(std::data(std::declval<Rng &>())), decltype(std::size(std::declval<Rng &>()))>> : std::true_type
    {
    };

    template <typename Rng, typename T>
    inline constexpr bool is_span_compatible_range = std::conjunction_v<
        std::negation<std::is_array<Rng>>,
        std::negation<is_span<std::remove_const_t<Rng>>>,
        std::negation<is_std_array<std::remove_const_t<Rng>>>,
        has_container_interface<Rng>,
        is_span_convertible_range<Rng, T>>;
} // namespace detail

template <typename T, std::size_t Ext = dynamic_extent>
class span : public detail::span_storage_<T, Ext>
{
    using base_type_ = detail::span_storage_<T, Ext>;
    using base_type_::data_;
    using base_type_::size_;
public:
    using element_type = T;
    using value_type = std::remove_cv_t<T>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using pointer = T *;
    using const_pointer = const T *;
    using reference = T &;
    using const_reference = const T &;
    using iterator = pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    static constexpr size_type extent = Ext;

    template <std::size_t Ext_ = Ext, std::enable_if_t<Ext_ == 0 || Ext_ == dynamic_extent, int> = 0>
    constexpr span() noexcept {}
    constexpr span(pointer ptr, size_type count) noexcept
        : base_type_(ptr, count)
    {
        APE_Expects(Ext == dynamic_extent || count == Ext,
            "Cannot construct span with static extent from range [ptr, ptr + count) as count != extent");
    }

    constexpr span(pointer first, pointer last) noexcept
        : base_type_(first, static_cast<size_type>(last - first))
    {
        APE_Expects(Ext == dynamic_extent || (last - first) == Ext,
            "Cannot construct span with static extent from range [ptr, ptr + count) as count != extent");
    }

    template <std::size_t N, std::enable_if_t<Ext == dynamic_extent || Ext == N, int> = 0>
    constexpr span(element_type (&arr)[N]) noexcept : base_type_(arr, N) {}

    template <typename U, std::size_t N,
              std::enable_if_t<std::conjunction_v<std::bool_constant<Ext == dynamic_extent || Ext == N>,
                                                  std::is_convertible<U (*)[], element_type (*)[]>>,
                               int> = 0>
    constexpr span(std::array<U, N> &arr) noexcept : base_type_(arr.data(), N) {}

    template <typename U, std::size_t N,
              std::enable_if_t<std::conjunction_v<std::bool_constant<Ext == dynamic_extent || Ext == N>,
                                                  std::is_convertible<const U (*)[], element_type (*)[]>>,
                               int> = 0>
    constexpr span(const std::array<U, N> &arr) noexcept : base_type_(arr.data(), N) {}

    template <typename Rng, std::enable_if_t<detail::is_span_compatible_range<Rng, element_type>, int> = 0>
    constexpr span(Rng &rng)
        : base_type_(std::data(rng), static_cast<size_type>(std::size(rng)))
    {
        APE_Expects(Ext == dynamic_extent || std::size(rng) == Ext,
            "Cannot construct span with static extent from range r as std::size(r) != extent");
    }

    template <typename Rng, std::enable_if_t<detail::is_span_compatible_range<const Rng, element_type>, int> = 0>
    constexpr span(const Rng &rng)
        : base_type_(std::data(rng), static_cast<size_type>(std::size(rng)))
    {
        APE_Expects(Ext == dynamic_extent || std::size(rng) == Ext,
            "Cannot construct span with static extent from range r as std::size(r) != extent");
    }

    template <class U, std::size_t UExt,
              std::enable_if_t<std::conjunction_v<std::bool_constant<Ext == dynamic_extent || UExt == dynamic_extent || Ext == UExt>,
                                                  std::is_convertible<U (*)[], element_type (*)[]>>,
                               int> = 0>
    span(const span<U, UExt> &other) noexcept
        : base_type_(other.data(), other.size())
    {
        APE_Expects(Ext == dynamic_extent || other.size() == Ext,
            "Cannot construct span with static extent from other span as other.size() != extent");
    }

     template <std::size_t Count>
    [[nodiscard]] constexpr auto first() const noexcept {
        if constexpr (Ext != dynamic_extent) {
            static_assert(Count <= Ext, "Count out of range in span::first()");
        }
        APE_Expects(Count <= size(), "Count out of range in span::first()");
        return span<element_type, Count>{data(), Count};
    }
    [[nodiscard]] constexpr auto first(size_type Count) const noexcept
    {
        APE_Expects(Count <= size(), "Count out of range in span::first()");
        return span<element_type, dynamic_extent>{data(), Count};
    }

    template <std::size_t Count>
    [[nodiscard]] constexpr auto last() const noexcept{
        if constexpr (Ext != dynamic_extent) {
            static_assert(Count <= Ext, "Count out of range in span::last()");
        }
        APE_Expects(Count <= size(), "Count out of range in span::last()");
        return span<element_type, Count>{data() + (size() - Count), Count};
    }

    [[nodiscard]] constexpr auto last(size_type Count) const noexcept{
        APE_Expects(Count <= size(), "Count out of range in span::last()");
        return span<element_type, dynamic_extent>{data() + (size() - Count), Count};
    }

    template <std::size_t Offset, size_t Count = dynamic_extent>
    [[nodiscard]] constexpr auto subspan() const noexcept  {
        if constexpr (Ext != dynamic_extent) {
            static_assert(Offset <= Ext, "Offset out of range in span::subspan()");
            static_assert(
                Count == dynamic_extent || Count <= Ext - Offset, "Count out of range in span::subspan()");
        }
        APE_Expects(Offset <= size(), "Offset out of range in span::subspan()");
        APE_Expects(Count == dynamic_extent || Count <= size() - Offset,
            "Count out of range in span::subspan()")      ;

        using ReturnType = span<element_type,
            Count != dynamic_extent ? Count : (Ext != dynamic_extent ? Ext - Offset : dynamic_extent)>;
        return ReturnType{data() + Offset, Count == dynamic_extent ? size() - Offset : Count};
    }

    [[nodiscard]] constexpr auto subspan(const size_type Offset, const size_type Count = dynamic_extent) const noexcept
    {
        APE_Expects(Offset <= size(), "Offset out of range in span::subspan()");
        APE_Expects(Count == dynamic_extent || Count <= size() - Offset,
            "Count out of range in span::subspan()");

        using ReturnType = span<element_type, dynamic_extent>;
        return ReturnType{data_ + Offset, Count == dynamic_extent ? size() - Offset : Count};
    }

    // [span.obs] Observers
    [[nodiscard]] constexpr std::size_t size() const noexcept {
        return size_;
    }

    [[nodiscard]] constexpr size_type size_bytes() const noexcept {
        return size() * sizeof(element_type);
    }

    [[nodiscard]] constexpr bool empty() const noexcept {
        return size() == 0;
    }

    // [span.elem] Element access
    [[nodiscard]] constexpr reference operator[](const size_type offset) const noexcept{
        APE_Expects(offset < size(), "span index out of range");
        return data_[offset];
    }

    [[nodiscard]] constexpr reference front() const noexcept  {
        APE_Expects(!empty(), "front of empty span");
        return data_[0];
    }

    [[nodiscard]] constexpr reference back() const noexcept  {
        APE_Expects(!empty(), "back of empty span");
        return data_[size() - 1];
    }

    [[nodiscard]] constexpr pointer data() const noexcept {
        return data_;
    }

    // [span.iterators] Iterator support
    [[nodiscard]] constexpr iterator begin() const noexcept {
        return data_;
    }

    [[nodiscard]] constexpr iterator end() const noexcept {
        return data_ + size();
    }

    [[nodiscard]] constexpr reverse_iterator rbegin() const noexcept {
        return reverse_iterator{end()};
    }

    [[nodiscard]] constexpr reverse_iterator rend() const noexcept {
        return reverse_iterator{begin()};
    }
};

// DEDUCTION GUIDES
template <class T, std::size_t Ext>
span(T (&)[Ext]) -> span<T, Ext>;

template <class T, std::size_t Size>
span(std::array<T, Size>&) -> span<T, Size>;

template <class T, std::size_t Size>
span(const std::array<T, Size>&) -> span<const T, Size>;

template <class Rng>
span(Rng&) -> span<typename Rng::value_type>;

template <class Rng>
span(const Rng&) -> span<const typename Rng::value_type>;

// [span.objectrep] Views of object representation
// FUNCTION TEMPLATE as_bytes
template <class T, std::size_t Ext>
[[nodiscard]] auto as_bytes(span<T, Ext> sp_) noexcept {
    using ReturnType = span<const std::byte, Ext == dynamic_extent ? dynamic_extent : sizeof(T) * Ext>;
    return ReturnType{reinterpret_cast<const std::byte*>(sp_.data()), sp_.size_bytes()};
}

// FUNCTION TEMPLATE as_writable_bytes
template <class T, std::size_t Ext, std::enable_if_t<!std::is_const_v<T>, int> = 0>
[[nodiscard]] auto as_writable_bytes(span<T, Ext> sp_) noexcept {
    using ReturnType = span<std::byte, Ext == dynamic_extent ? dynamic_extent : sizeof(T) * Ext>;
    return ReturnType{reinterpret_cast<std::byte*>(sp_.data()), sp_.size_bytes()};
}

template<typename T, size_t N>
inline [[nodiscard]] auto as_bytes(const std::array<T, N>& buf) noexcept {
    using ReturnType = span<const std::byte, sizeof(T) * N>;
    return ReturnType{reinterpret_cast<const std::byte*>(buf.data()), sizeof(T) * buf.size()};
}
template<typename T, size_t N>
inline [[nodiscard]] auto as_writable_bytes(std::array<T, N>& buf) noexcept {
    using ReturnType = span<std::byte, sizeof(T) * N>;
    return ReturnType{ reinterpret_cast<std::byte*>(buf.data()), sizeof(T) * buf.size()};
}

template <typename T, typename Rng, std::enable_if_t<detail::is_span_compatible_range<Rng, T>, int> = 0>
inline [[nodiscard]] span<const byte> as_bytes(const Rng& buf) noexcept {
{
    return span<const std::byte>{reinterpret_cast<const std::byte*>(buf.data()), sizeof(T) * buf.size()};
}

template <typename T, typename Rng, std::enable_if_t<detail::is_span_compatible_range<Rng, T>, int> = 0>
inline [[nodiscard]] span<std::byte> as_writable_bytes(Rng& buf) noexcept {
    return span<std::byte>{ reinterpret_cast<std::byte*>(buf.data()), sizeof(T) * buf.size()};
}

} // end namespace stl
END_APE_NAMESPACE

#endif //END CPP_STANDARD >= CPP_STD_20
#endif //end APE_ESTL_BACKPORTS_SPAN_H
