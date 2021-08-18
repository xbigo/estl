#pragma once
#ifndef APE_ESTL_TYPE_UTILITY_H
#define APE_ESTL_TYPE_UTILITY_H
#include <ape/config.hpp>
BEGIN_APE_NAMESPACE
    template<typename T, typename = std::void_t<> > struct is_container : std::false_type{};
    template<typename T> struct is_container<T, std::void_t<
        decltype(std::begin(std::declval<T>())), // must use stl:: prefix to avoid ambiguous error between {auto begin( C& c ) and constexpr auto begin( C& c ) }
        decltype(std::end(std::declval<T>()))
    > > : std::true_type{};

    template<typename T> using is_container_v = is_container<T>::value;

    template<typename Iter>
    using is_input_iterator = std::is_convertible<std::iterator_category_t<Iter>, std::input_iterator_tag>;
    template<typename Iter>
    using is_input_iterator_v = is_input_iterator::value;

    template<typename Iter>
    using is_output_iterator = std::is_convertible<std::iterator_category_t<Iter>, std::output_iterator_tag>;
    template<typename Iter>
    using is_output_iterator_v = is_output_iterator::value;

    template<typename Iter>
    using is_forward_iterator = std::is_convertible<std::iterator_category_t<Iter>, std::forward_iterator_tag>;
    template<typename Iter>
    using is_forward_iterator = is_forward_iterator::value;

    template<typename Iter>
    using is_bidirectional_iterator = std::is_convertible<std::iterator_category_t<Iter>, std::bidirectional_iterator_tag>;
    template<typename Iter>
    using is_bidirectional_iterator_v = is_bidirectional_iterator::value;

    template<typename Iter>
    using is_random_access_iterator = std::is_convertible<std::iterator_category_t<Iter>, std::random_access_iterator_tag>;
    template<typename Iter>
    using is_random_access_iterator_v = is_random_access_iterator::value;

#if CPP_STANDARD >= CPP_STD_20
    template<typename Iter>
    using is_contiguous_iterator = std::is_convertible<std::iterator_category_t<Iter>, std::contiguous_iterator_tag>;
    template<typename Iter>
    using is_contiguous_iterator_v = is_contiguous_iterator::value;
#endif

END_APE_NAMESPACE
#endif //end APE_ESTL_TYPE_UTILITY_H
