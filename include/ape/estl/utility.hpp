#pragma once
#ifndef APE_ESTL_UTILITY_H
#define APE_ESTL_UTILITY_H
#include <ape/config.hpp>
#include <ape/estl/concepts.hpp>
#include <gsl/gsl_assert>

#include <cstdint>

BEGIN_APE_NAMESPACE
template <typename... Args> void unused(Args &&...) {}

using long_size_t = std::uint64_t;
using long_offset_t = std::int64_t;

struct long_offset_range{
    long_offset_t begin{0}, end{0};
};
inline constexpr long_size_t size(long_offset_range h) noexcept
{
    APE_Expects(h.begin <= h.end);
    return long_size_t(h.end - h.begin);
}

inline constexpr bool in_size_t_range(long_size_t n) noexcept
{
    return (n & ~long_size_t(std::numeric_limits<std::size_t>::max())) == 0;
}
inline constexpr bool in_size_t_range(long_offset_t n) noexcept
{
    return in_size_t_range(long_size_t(n));
}
inline constexpr bool in_ptrdiff_range(long_offset_t n) noexcept
{
    std::ptrdiff_t v(static_cast<ptrdiff_t>(n));
    return v == n;
}
inline constexpr std::size_t narrow_cast(long_size_t n) noexcept
{
    APE_Expects(in_size_t_range(n));
    return std::size_t(n);
}
inline constexpr std::ptrdiff_t narrow_cast(long_offset_t n) noexcept
{
    APE_Expects(in_ptrdiff_range(n));
    return std::ptrdiff_t(n);
}

template<typename T>
class not_own{
    T* ptr{nullptr};
public:
    constexpr not_own() noexcept = default;

    constexpr not_own(std::nullptr_t) noexcept {};

    template<ptr_convertible_to<T> U>
    explicit constexpr not_own(U* p) noexcept: ptr(p) {}

    template<ptr_convertible_to<T> U>
    constexpr not_own(not_own<U> p) noexcept: ptr(p.get()) {}

    constexpr not_own& operator=(std::nullptr_t) noexcept {
            ptr = nullptr;
            return *this;
    }

    template<ptr_convertible_to<T> U>
    constexpr not_own& operator=(U* p) noexcept {
        ptr = p;
        return *this;
    }
    template<ptr_convertible_to<T> U>
    constexpr not_own& operator=(not_own<U> p) noexcept {
        ptr = p.get();
        return *this;
    }

    explicit constexpr operator bool() const noexcept {
        return ptr != nullptr;
    }


    constexpr T* get() const noexcept {
        return ptr;
    }

    template<ptr_convertible_from<T> U>
    constexpr U* get() const noexcept {
        return get();
    }

    constexpr T* operator->() const noexcept {
        APE_Expects(ptr);
        return get();
    }

    constexpr T& operator*() const noexcept {
        APE_Expects(ptr);
        return *get();
    }
    template<ptr_convertible_from<T> U>
    constexpr U& operator*()const noexcept {
        APE_Expects(ptr);
        return *get();
    }
};

template<typename T>  not_own(T*) -> not_own<T>;

template<typename T, typename U>
requires std::common_with<T*, U*>
inline constexpr bool operator==(not_own<T> lhs, not_own<U> rhs) noexcept{
    return lhs.get() == rhs.get();
}

template<typename T, typename U>
requires std::common_with<T*, U*>
inline constexpr decltype(auto) operator<=>(not_own<T> lhs, not_own<U> rhs) noexcept{
    return lhs.get() <=> rhs.get();
}

template <typename T> class own_ptr {
    T *ptr{nullptr};

  public:
    template <typename U> friend class own_ptr;

    ~own_ptr() { APE_Expects(!valid()); }

    constexpr own_ptr() noexcept = default;

    template <ptr_convertible_to<T> U>
    explicit constexpr own_ptr(U *p) noexcept : ptr(p) {}

    template <ptr_convertible_to<T> U>
    constexpr own_ptr(own_ptr<U> &&p) noexcept {
        std::swap(ptr, p.ptr);
    }

    template <ptr_convertible_to<T> U>
    constexpr own_ptr &operator=(U *p) noexcept {
        APE_Expects(!valid());
        ptr = p;
        return *this;
    }

    template <ptr_convertible_to<T> U>
    constexpr own_ptr &operator=(own_ptr<U> &&p) noexcept {
        APE_Expects(!valid());
        if (this != &p) {
          ptr = p.get();
          p.ptr = nullptr;
        }
        return *this;
    }

    explicit constexpr operator bool() const noexcept { return valid(); }

    constexpr T *get() const noexcept { return ptr; }

    template <ptr_convertible_from<T> U> constexpr U *get() const noexcept {
        return get();
    }

    constexpr T *operator->() const noexcept {
        APE_Expects(valid());
        return get();
    }

    constexpr T &operator*() const noexcept {
        APE_Expects(valid());
        return *get();
    }
    template <ptr_convertible_from<T> U>
    constexpr U &operator*() const noexcept {
        APE_Expects(valid());
        return *get();
    }

    constexpr bool valid() const noexcept { return ptr != nullptr; }

    constexpr bool release() noexcept {
        auto r = valid();
        ptr = nullptr;
        return r;
    }
};

template <typename T> own_ptr(T *) -> own_ptr<T>;

template <typename T, typename U>
  requires std::common_with<T *, U *>
inline constexpr bool operator==(own_ptr<T> lhs, own_ptr<U> rhs) noexcept {
    return lhs.get() == rhs.get();
}

template <typename T, typename U>
  requires std::common_with<T *, U *>
inline constexpr decltype(auto) operator<=>(own_ptr<T> lhs,
                                            own_ptr<U> rhs) noexcept {
    return lhs.get() <=> rhs.get();
}
END_APE_NAMESPACE

#endif //end  APE_ESTL_UTILITY_H