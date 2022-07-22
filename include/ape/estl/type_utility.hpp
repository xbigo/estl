#pragma once
#ifndef APE_ESTL_TYPE_UTILITY_H
#define APE_ESTL_TYPE_UTILITY_H
#include <ape/config.hpp>
#include <type_traits>
#include <array>
BEGIN_APE_NAMESPACE

struct default_t{};
inline constexpr  default_t const default_v;

template<typename T> using is_default_type = std::is_same<default_t, T>;
template<typename T> using is_default_type_v = is_default_type<T>::value;

template<typename T> struct is_std_array : std::false_type {};
template<typename T, std::size_t N> struct is_std_array<std::array<T, N>> : std::true_type {};
template<typename T> using is_std_array_v = is_std_array<T>::value;
template<typename T> constexpr bool is_std_array_f() noexcept{
    return is_std_array<T>::value;
}

template<typename T> struct element_type;
template<typename T, std::size_t N> struct element_type<std::array<T,N>> : std::type_identity<T> {};
template<typename T> using element_type_t = typename element_type<T>::type;

template<typename T> struct std_array_size;
template<typename T, std::size_t N> struct std_array_size<std::array<T,N>> : std::integral_constant<std::size_t, N> {};
template<typename T> using std_array_size_v = std_array_size<T>::value;

template<typename T> using bool_type = std::conditional<T::value, std::true_type, std::false_type>;
template<typename T> using bool_type_t = typename std::conditional<T::value, std::true_type, std::false_type>::type;

END_APE_NAMESPACE
#endif //end APE_ESTL_TYPE_UTILITY_H
