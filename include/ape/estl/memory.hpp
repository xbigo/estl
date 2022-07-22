#ifndef APE_ESTL_MEMORY_H
#define APE_ESTL_MEMORY_H
#include <ape/config.hpp>
#include <ape/estl/concepts.hpp>
#include <memory>   // for std::addressof
#include <utility> // for std::forward
#include <iterator> //for forward_iterator
#include <array>
BEGIN_APE_NAMESPACE

template<typename T> inline constexpr
void default_construct(T& t)
    noexcept(std::is_nothrow_default_constructible_v<T>){
    new (std::addressof(t)) T();
}

template<typename T> inline constexpr
void default_construct_uninit(T& t)
    noexcept(std::is_nothrow_default_constructible_v<T>){
    new (std::addressof(t)) T;
}

template<typename T, typename ...U> inline constexpr
void emplace_construct(T& t, U&& ... args)
    noexcept(std::is_nothrow_constructible_v<T, U&&...>){
    new (std::addressof(t)) T(std::forward<U>(args)...);
}
template<typename T, std::size_t N> inline constexpr
void emplace_construct(std::array<T, N>& t, std::initializer_list<T> ilist)
    noexcept(std::is_nothrow_move_constructible_v<T>){
    return emplace_construct(t, ilist, std::make_index_sequence<N>{});
}
template<typename T, std::size_t N, std::size_t... Is> inline constexpr
void emplace_construct(std::array<T, N>& t, std::initializer_list<T> ilist, std::index_sequence<Is...>)
    noexcept(std::is_nothrow_move_constructible_v<T>){
    new (std::addressof(t)) T{std::move(*(ilist.begin() + Is))...};
}

template<complete_type T> inline constexpr
void destruct(T& t) noexcept{
    static_assert(sizeof(T) != 0, "type T is incomplete in destruct");
    t.~T();
}

template<std::forward_iterator Iter> inline constexpr
void destruct(Iter first, Iter last) noexcept{
    for(; first != last; ++first) destruct(*first);
}

template<std::bidirectional_iterator Iter> inline constexpr
void destruct_reverse(Iter first, Iter last) noexcept{
    for(; first != last; ) destruct(*--last);
}

template <typename AllocTraits>
struct extra_allocator_traits : AllocTraits
{
    using allocator_type = typename AllocTraits::allocator_type;

    static constexpr allocator_type select_on_copy(const allocator_type &a)
    {
        return AllocTraits::select_on_container_copy_construction(a);
    }
    static constexpr bool propagate_on_copy_assign() noexcept
    {
        return AllocTraits::propagate_on_container_copy_assignment::value;
    }

    static constexpr bool propagate_on_move_assign() noexcept
    {
        return AllocTraits::propagate_on_container_move_assignment::value;
    }

    static constexpr bool propagate_on_swap() noexcept
    {
        return AllocTraits::propagate_on_container_swap::value;
    }

    static constexpr bool always_equal() noexcept
    {
        return AllocTraits::is_always_equal::value;
    }

    static constexpr bool nothrow_move() noexcept
    {
        return propagate_on_move_assign() || always_equal();
    }
    static constexpr void on_swap(allocator_type &a, allocator_type &b) noexcept
    {
        using std::swap;
        if constexpr (propagate_on_swap())
            std::swap(a, b);
    }
};
END_APE_NAMESPACE
#endif