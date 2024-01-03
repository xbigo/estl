#pragma once
#ifndef APE_ESTL_UTILITY_H
#define APE_ESTL_UTILITY_H
#include <ape/config.hpp>
#include <ape/estl/concepts.hpp>
#include <gsl/gsl_assert>

#include <cstdint>

BEGIN_APE_NAMESPACE
using long_size_t = std::uint64_t;
using long_offset_t = std::int64_t;

struct long_offset_range{
    long_offset_t begin{0}, end{0};
};

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

template<typename ... Args>
void unused(Args&&... ){}

END_APE_NAMESPACE

#endif //end  APE_ESTL_UTILITY_H