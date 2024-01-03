#pragma once
#ifndef APE_ESTL_CONCEPTS_H
#define APE_ESTL_CONCEPTS_H
#include <ape/config.hpp>
#include <type_traits>
#include <concepts>
#include <iterator> //For std::indirectly_swappable

BEGIN_APE_NAMESPACE
namespace detail{
    template<typename T>
    concept indirectly_writable_ = std::indirectly_writable<T, std::iter_value_t<T>>;
}

//Extension
template<typename T> concept complete_type = sizeof(T) != 0;

template <typename T>
concept boolean_testable =
    std::convertible_to<T, bool> && requires(T &&t) {
        { !std::forward<T>(t) } -> std::convertible_to<bool>;
    };

template<typename T>
concept writable_forward_iterator = std::forward_iterator<T> && detail::indirectly_writable_<T>;
template<typename T>
concept writable_bidirectional_iterator = std::bidirectional_iterator<T> && detail::indirectly_writable_<T>;
template<typename T>
concept writable_random_access_iterator = std::random_access_iterator<T> && detail::indirectly_writable_<T>;
template<typename T>
concept writable_contiguous_iterator = std::contiguous_iterator<T> && detail::indirectly_writable_<T>;

template<typename U, typename T>
concept exclude_type = !std::same_as<T, std::remove_cvref_t<U>>;

template<typename From, typename To> 
concept ptr_convertible_to  = std::convertible_to<From*, To*>;

template<typename To, typename From> 
concept ptr_convertible_from  = std::convertible_to<From*, To*>;

template<typename E>
concept enum_type = std::is_enum_v<E>;

template<typename T>
concept has_member_size = requires(const T& t) {
    {std::size(t)} -> std::convertible_to<std::size_t>;
};

template<typename T, typename U>
concept permissive_same_as = 
    std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;



END_APE_NAMESPACE

#endif // end APE_ESTL_CONCEPTS_H