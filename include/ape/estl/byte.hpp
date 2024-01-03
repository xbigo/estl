#pragma once
#ifndef APE_ESTL_BYTE_H
#define APE_ESTL_BYTE_H
#include <ape/config.hpp>
#include <string>
#include <cstddef>
#include <type_traits>

BEGIN_APE_NAMESPACE

template <typename T>
constexpr inline std::byte *cast_to_byte_ptr(T *s) noexcept
{
    return reinterpret_cast<std::byte *>(s);
}

template <typename T>
constexpr inline const std::byte *cast_to_byte_ptr(const T *s) noexcept
{
    return reinterpret_cast<const std::byte *>(s);
}

template <typename T>
constexpr inline T *cast_from_byte_ptr(std::byte *s) noexcept
{
    return reinterpret_cast<T *>(s);
}

template <typename T>
constexpr inline const T *cast_from_byte_ptr(const std::byte *s) noexcept
{
    return reinterpret_cast<const T *>(s);
}

inline std::byte *copy_mem(std::byte *dest, const std::byte *src, size_t n) noexcept
{
    auto ret = std::char_traits<char>::copy(cast_from_byte_ptr<char>(dest), cast_from_byte_ptr<char>(src), n);
    return cast_to_byte_ptr(ret + n);
}

inline std::byte *copy_mem(char *dest, const char *src, size_t n) noexcept
{
    auto ret = std::char_traits<char>::copy(dest, src, n);
    return cast_to_byte_ptr(ret + n);
}

inline std::byte *copy_mem(void *dest, const void *src, size_t n) noexcept
{
    auto ret = std::char_traits<char>::copy(static_cast<char *>(dest), static_cast<const char *>(src), n);
    return cast_to_byte_ptr(ret + n);
}

template<typename T> requires std::is_trivially_copyable_v<T>
inline T *copy_mem(T *dest, const T *src, size_t n) noexcept
{
    auto ret = copy_mem(cast_from_byte_ptr<char>(dest), cast_from_byte_ptr<char>(src), n);
    return cast_from_byte_ptr(ret);
}

END_APE_NAMESPACE
#endif //end APE_ESTL_BYTE_H