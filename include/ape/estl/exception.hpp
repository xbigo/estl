#pragma once
#ifndef APE_ESTL_EXCEPTION_H
#define APE_ESTL_EXCEPTION_H
#include <ape/config.hpp>
#include <string>
#include <sstream>

BEGIN_APE_NAMESPACE
namespace detail
{
    template <typename Base>
    class context_exception : public Base
    {
    public:
        using Base::Base;
        
        context_exception(context_exception&&) = default;

        /// just for nothow compatible
        virtual ~context_exception() {}

        /// \return exception information, return as  char string
        virtual const char *what() const noexcept
        {
            return this->m_msg.c_str();
        }

        /// helper overloaded for APE_THROW
		[[noreturn]] void operator=(std::ostream& sstr) && {
            m_msg = Base::what();
            if (!m_msg.empty())
                m_msg.push_back('\n');
            m_msg += static_cast<std::stringstream&>(sstr).str();
            throw std::move(*this);
        }

    private:
        std::string m_msg;
    };
    
}

struct pack_init
{
    std::stringstream stros;
    template<typename T>
    pack_init& operator<<(const T&rhs) {
        stros << rhs;
        return *this;
    }
    friend std::ostream& operator<<(std::ostream& os, const pack_init & pack_){
        return os << pack_.stros.rdbuf();
    }

};


END_APE_NAMESPACE

#define APE_DEFINE_EXCEPTION(type, base) \
    struct type : public base            \
    {                                    \
        using base_type = base;          \
        using base_type::base_type;      \
        virtual ~type() {}               \
    }

/// throw exception of type ex
/// usage: APE_THROW(exception_type, arguments_of_exception_type...) << Additional_infomation;
/// \param ex type of exception
#define APE_THROW(ex, ...) if (std::stringstream sstr_; true) \
::ape::detail::context_exception<ex>{__VA_ARGS__} = \
sstr_  << __FILE__ << '(' << __LINE__ << ')'

#endif // end APE_ESTL_EXCEPTION_H