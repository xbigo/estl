#pragma once
#ifndef APE_ESTL_ERROR_CODE_H
#define APE_ESTL_ERROR_CODE_H

#include <ape/estl/utility.hpp>
#include <ape/estl/concepts.hpp>
#include <ape/estl/exception.hpp>

#include <system_error>
BEGIN_APE_NAMESPACE

APE_DEFINE_EXCEPTION(error_exception, std::system_error);

using error_code = std::error_code;
using error_condition = std::error_condition;
using error_category = std::error_category;

using error_code_ptr = not_own<error_code>;

inline void clear_error(error_code_ptr ec) noexcept
{
    if (ec)
        ec->clear();
}

inline bool has_error(error_code_ptr ec) noexcept
{
    return ec && *ec;
}

template <typename ExceptionType, enum_type E>
inline void set_or_throw_error(error_code_ptr ec, E e)
{
    if (ec)
    {
        *ec = make_error_code(e);
    }
    else if (e != E{})
        APE_THROW(ExceptionType, make_error_code(e));
}

template <typename ExceptionType>
inline void set_or_throw_error(error_code_ptr ec, error_code e)
{
    if (ec)
    {
        *ec = e;
    }
    else if (e)
        APE_THROW(ExceptionType, e);
}

template <typename ExceptionType, enum_type E, typename ... Messages>
inline bool transfer_error(error_code_ptr ec, E e,
                           Messages && ...msg)
{
    if (ec)
        *ec = make_error_code(e);
    else if (e != E{})
        APE_THROW(ExceptionType, make_error_code(e)) << (pack_init{} << ... << msg);
    return static_cast<bool>(e);
}

template <typename ExceptionType, typename ... Messages>
inline bool transfer_error(error_code_ptr ec, error_code e,
                           Messages && ...msg)
{

    if (ec)
        *ec = e;
    else if (e)
        APE_THROW(ExceptionType, e) << (pack_init{} << ... << msg);
    return static_cast<bool>(e);
}

END_APE_NAMESPACE
#endif //end APE_ESTL_ERROR_CODE_H