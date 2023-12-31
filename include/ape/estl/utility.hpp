#pragma once
#ifndef APE_ESTL_UTILITY_H
#define APE_ESTL_UTILITY_H
#include <ape/config.hpp>
#include <concepts>
#include <gsl/gsl_assert>

BEGIN_APE_NAMESPACE

template<typename From, typename To> 
concept ptr_convertible_to  = std::convertible_to<From*, To*>;

template<typename To, typename From> 
concept ptr_convertible_from  = std::convertible_to<From*, To*>;

template<typename T> 
class not_own{
    T* ptr{nullptr};
public:
    constexpr not_own() noexcept = default;
#if 0
    constexpr not_own(const not_own&) noexcept = default;
    constexpr not_own(not_own&&) noexcept = default;
    constexpr not_own& operator=(const not_own&) noexcept = default;
    constexpr not_own& operator=(not_own&&) noexcept = default;
#endif
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
        APE_Expects(ptr);
        return ptr;
    }
    
    template<ptr_convertible_from<T> U>
    constexpr U* get() const noexcept {
        APE_Expects(ptr);
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


END_APE_NAMESPACE

#endif //end  APE_ESTL_UTILITY_H