#ifndef APE_ESTL_BASIC_REF_VECTOR_H
#define APE_ESTL_BASIC_REF_VECTOR_H
#include <concepts>
#include <algorithm> // for std::min/max
#include <memory> // for std::addressof
#include <array>
#include <span>
#include <initializer_list>

#include <gsl/gsl_assert>

#include <ape/estl/memory.hpp>
#include <ape/estl/concepts.hpp>
#include <ape/estl/type_utility.hpp>

BEGIN_APE_NAMESPACE

// Concepts for size and buffer policy
template<typename Policy> concept size_policy =
    std::semiregular<Policy>
    && std::destructible<Policy>
    && requires{
        typename Policy::size_store_type;
        typename Policy::size_type;
        typename Policy::need_init;
        requires requires(Policy& p
            , const Policy& cp
            , typename Policy::size_type n)
            {
                {cp.get_size()} noexcept -> std::same_as<typename Policy::size_type>;
                {p.set_size(n)};
            };
    };

    template <typename Policy>
    concept buffer_policy =
        std::regular<Policy> && std::destructible<Policy> && requires
    {
        typename Policy::const_iterator;
        typename Policy::const_pointer;
        typename Policy::const_reference;
        typename Policy::const_reverse_iterator;
        typename Policy::difference_type;
        typename Policy::iterator;
        typename Policy::pointer;
        typename Policy::reference;
        typename Policy::reverse_iterator;
        typename Policy::size_type;
        typename Policy::value_type;
        requires requires(Policy & p, const Policy &cp, std::size_t n)
        {
            {cp.size() } noexcept -> std::same_as<typename Policy::size_type>;

            {cp.begin()} noexcept -> std::same_as<typename Policy::const_iterator>;
            {p.begin()} noexcept -> std::same_as<typename Policy::iterator>;
            {cp.data()} noexcept -> std::same_as<typename Policy::const_pointer>;
            {p.data()} noexcept -> std::same_as<typename Policy::pointer>;
            {cp.rbegin()} noexcept -> std::same_as<typename Policy::const_reverse_iterator>;
            {p.rbegin()} noexcept -> std::same_as<typename Policy::reverse_iterator>;
        };
    };

template<typename SizeType = std::size_t, typename APISizeType = std::size_t>
struct separated_policy{
    using size_store_type = SizeType;
    using size_type = APISizeType;
    using need_init = std::false_type;

    size_store_type m_size{0};
    separated_policy()  = default;
    constexpr size_type get_size() const noexcept{
        return size_type(m_size);
        };
    constexpr void set_size(size_type n) noexcept{
        m_size = size_store_type(n);
        }
};

template<typename SizeType = std::size_t, typename APISizeType = std::size_t>
struct separated_indirect_policy{
    using size_store_type = SizeType;
    using size_type = APISizeType;
    using need_init = std::true_type;

    size_store_type* m_size;
    explicit separated_indirect_policy(size_store_type* p = nullptr) : m_size(p){}
    constexpr size_type get_size() const noexcept{
        APE_Expects(m_size != nullptr);
        return size_type(*m_size);
        };
    constexpr void set_size(size_type n) noexcept{
        APE_Expects(m_size != nullptr);
        *m_size = size_store_type(n);
        }
};

namespace detail_{
    template<buffer_policy BufferPolicy, size_policy SizePolicy>
    struct ref_storage : SizePolicy,  BufferPolicy {
        ref_storage() = default;
        ~ref_storage() = default;
        struct tag_t{};

        template<typename Arg1, typename ...Args>
        requires exclude_type<Arg1, ref_storage>
            && exclude_type<Arg1, tag_t>
        ref_storage(Arg1&& p1, Args&& ...p2)
        : ref_storage(tag_t{}, bool_type_t<is_std_array<BufferPolicy>>{}
            , typename SizePolicy::need_init{},
            std::forward<Arg1>(p1), std::forward<Args>(p2)...)
        {}

        template<typename Arg1, typename ...Args>
        ref_storage(tag_t, std::false_type, std::true_type, Arg1&& p1, Args&& ...p2)
            : SizePolicy(std::forward<Arg1>(p1))
            , BufferPolicy(std::forward<Args>(p2)...)
            {}

        template<typename Arg1, typename ...Args>
        ref_storage(tag_t, std::false_type, std::false_type, Arg1&& p1, Args&& ...p2)
            : SizePolicy()
            , BufferPolicy(std::forward<Arg1>(p1), std::forward<Args>(p2)...)
            {}

        template <typename Arg1, typename Type>
        ref_storage(tag_t, std::true_type, std::true_type, Arg1&& p1, std::initializer_list<Type> il)
            : ref_storage(tag_t{}, std::make_index_sequence<std_array_size<BufferPolicy>::value>{},
                 std::forward<Arg1>(p1), il) {}

        template <typename Arg1, typename ... Args>
        ref_storage(tag_t, std::true_type, std::true_type, Arg1&& p1, Args&& ...args)
            : SizePolicy(std::forward<Arg1>(p1))
            , BufferPolicy{std::forward<Args>(args)...} {
                set_size(sizeof...(Args));
            }

        template <typename Type>
        ref_storage(tag_t, std::true_type, std::false_type, std::initializer_list<Type> il)
            : ref_storage(tag_t{}, std::make_index_sequence<std_array_size<BufferPolicy>::value>{}, il) {}

        template <typename ...Args>
        ref_storage(tag_t, std::true_type, std::false_type, Args&&... args)
            : SizePolicy()
            , BufferPolicy{std::forward<Args>(args)...} {
                set_size(sizeof...(Args));
            }

        template <typename Arg1, typename Type, std::size_t... Is>
        ref_storage(tag_t, std::index_sequence<Is...>, Arg1&& p1, std::initializer_list<Type> il)
            : SizePolicy(std::forward<Arg1>(p1))
            , BufferPolicy{*(il.begin() + Is)...} {
                set_size(il.size());
            }
        template <typename Type, std::size_t... Is>
        ref_storage(tag_t, std::index_sequence<Is...>, std::initializer_list<Type> il)
            : SizePolicy()
            , BufferPolicy{*(il.begin() + Is)...} {
                set_size(il.size());
            }

        using SizePolicy::get_size;
        using SizePolicy::set_size;
        using SizePolicy::size_type;
        using SizePolicy::size_store_type;

        constexpr SizePolicy &get_sizer_() noexcept { return *this; }
        constexpr const SizePolicy &get_sizer_() const noexcept { return *this; }
        constexpr BufferPolicy &get_buffer_() noexcept { return *this; }
        constexpr const BufferPolicy &get_buffer_() const noexcept { return *this; }
    };
}

// A ref_vector can be bind to array or memory via span,
// and the the bound target can't be changed in whole lifetime, even in swap.
// except full_swap, it will exchange all underly data;
template<buffer_policy BufferPolicy, size_policy SizePolicy = separated_policy<>>
class basic_ref_vector : private detail_::ref_storage<BufferPolicy, SizePolicy>
{
    using base = detail_::ref_storage<BufferPolicy, SizePolicy>;
    using buffer = BufferPolicy;
    constexpr base& get_base_() noexcept{ return *this;}
    constexpr const base& get_base_() const noexcept{ return *this;}
    public:

    using typename base::value_type;
    using typename base::size_type;
    using typename base::difference_type;
    using typename base::reference;
    using typename base::const_reference;
    using typename base::pointer;
    using typename base::const_pointer;
    using typename base::iterator;
    using typename base::const_iterator;
    using typename base::reverse_iterator;
    using typename base::const_reverse_iterator;
    private: //helpers

    public:
    constexpr basic_ref_vector() noexcept = default;
    constexpr ~basic_ref_vector() noexcept = default;

    constexpr basic_ref_vector(const basic_ref_vector& rhs)=default;
    constexpr basic_ref_vector(basic_ref_vector&& rhs) noexcept = default;
    constexpr basic_ref_vector(std::initializer_list<value_type> ilist )
        : base(ilist){
        APE_Expects(ilist.size() <= capacity());
    }
    template <exclude_type<basic_ref_vector> Arg1, typename ...Args>
    constexpr basic_ref_vector(Arg1&& p1, Args&& ... args)
        : base(std::forward<Arg1>(p1), std::forward<Args>(args)...)
        {}
    template <std::forward_iterator Iter>
    constexpr basic_ref_vector(Iter first, Iter last)
        : base{}
        {
            auto length = std::distance(first, last);
            std::copy(first, last, begin());
            this->set_size(length);
        }

    basic_ref_vector& operator=( std::initializer_list<value_type> ilist ){
        APE_Expects(ilist.size() <=  capacity());
        assign(ilist);
        return *this;
    }
    constexpr basic_ref_vector& operator=(const basic_ref_vector& rhs){
        APE_Expects(rhs.size() <=  capacity());
        [[likely]]
        if (this != &rhs){
            assign(rhs.begin(), rhs.end());
        }
        return *this;
    }
    constexpr basic_ref_vector& operator=(basic_ref_vector&& rhs) noexcept{
        APE_Expects(rhs.size() <=  capacity());
        [[likely]]
        if (this != &rhs){
            move_assign(rhs.begin(), rhs.end());
        }
        return *this;
    }
    constexpr void swap(basic_ref_vector& rhs) noexcept{
        APE_Expects(rhs.size() <=  capacity());
        APE_Expects(size() <= rhs.capacity());
        auto this_size = size();
        auto that_size = rhs.size();
        auto bigger_size = std::max(this_size, that_size);
        std::swap_ranges(begin(), begin() + bigger_size, rhs.begin());
        this->get_sizer_().set_size(that_size);
        rhs.get_sizer_().set_size(this_size);
    }

    constexpr void full_swap(basic_ref_vector& rhs) noexcept {
        std::swap<base>(*this, rhs);
    }

    void assign( size_type n, const_reference value ){
        APE_Expects(n <= capacity());
        std::fill_n(begin(), n, value);
        this->get_sizer_().set_size(n);
    }
    template< std::forward_iterator ForwardItr>
        void assign( ForwardItr first, ForwardItr last ){
            size_type n = std::distance(first, last);
            APE_Expects(n <= capacity());
            std::copy(first, last, begin());
            this->get_sizer_().set_size(n);
        }

    void assign( std::initializer_list<value_type> ilist ){
        assign(ilist.begin(), ilist.end());
    }
    template< std::forward_iterator ForwardItr>
        void move_assign( ForwardItr first, ForwardItr last ){
            size_type n = std::distance(first, last);
            APE_Expects(n <= capacity());
            std::move(first, last, begin());
            this->get_sizer_().set_size(n);
        }

    constexpr reference at( size_type pos ){
        [[unlikely]]
        if (pos >= size()) throw std::out_of_range ("basic_ref_vector::at");
        return (*this)[pos];
    }
    constexpr const_reference at( size_type pos ) const{
        [[unlikely]]
        if (pos >= size()) throw std::out_of_range ("basic_ref_vector::at");
        return (*this)[pos];
    }
    constexpr reference operator[]( size_type pos ) noexcept{
        APE_Expects(pos < size());
        return *(begin() + pos);
    }
    constexpr const_reference operator[]( size_type pos ) const noexcept{
        APE_Expects(pos < size());
        return *(begin() + pos);
    }
    constexpr reference front() noexcept{
        APE_Expects(!empty());
        return *begin();
    }
    constexpr const_reference front() const noexcept{
        APE_Expects(!empty());
        return *begin();
    }
    constexpr reference back() noexcept{
        APE_Expects(!empty());
        return *(begin() + (size() - 1));
    }
    constexpr const_reference back() const noexcept{
        APE_Expects(!empty());
        return *(begin() + (size() - 1));
    }
    constexpr void sweep() noexcept(std::is_nothrow_default_constructible_v<value_type>){
        std::fill(end(), begin() + capacity(), value_type{});
        }
    constexpr pointer data() noexcept{ return buffer::data(); }
    constexpr const_pointer data() const noexcept{ return buffer::data(); }
    constexpr iterator begin() noexcept { return buffer::begin();}
    constexpr const_iterator begin() const noexcept{ return buffer::begin();}
    constexpr const_iterator cbegin() const noexcept{ return buffer::begin();}
    constexpr iterator end() noexcept { return buffer::begin() + size();}
    constexpr const_iterator end() const noexcept{ return buffer::begin() + size();}
    constexpr const_iterator cend() const noexcept { return buffer::begin() + size();}

    constexpr reverse_iterator rbegin() noexcept{ return buffer::rbegin() + (capacity()- size());}
    constexpr const_reverse_iterator  rbegin() const noexcept { return buffer::rbegin() + (capacity() - size());}
    constexpr const_reverse_iterator crbegin() const noexcept { return buffer::rbegin() + (capacity() - size());}
    constexpr reverse_iterator rend() noexcept { return buffer::rend();}
    constexpr const_reverse_iterator rend() const noexcept { return buffer::rend();}
    constexpr const_reverse_iterator crend() const noexcept { return buffer::rend();}

    constexpr bool full() const noexcept{ return size() == capacity(); }
    constexpr bool empty() const noexcept{ return size() == 0; }
    constexpr size_type size() const noexcept{ return this->get_sizer_().get_size(); }
    constexpr size_type max_size() const noexcept{ return capacity(); }
    constexpr size_type capacity() const noexcept{ return buffer::size(); }
    //Modifiler
    constexpr void fill( const_reference value ){ assign(capacity(), value); }
    constexpr void clear(bool do_sweep = false) noexcept{
        this->get_sizer_().set_size(0);
        [[unlikely]]
        if (do_sweep)
            sweep();
    }

    constexpr iterator insert( const_iterator pos_, const_reference value ){
        return emplace(pos_, value);
    }
    constexpr iterator insert( const_iterator pos_, value_type&& value ){
        APE_Expects(!full() && pos_ <= cend());
        iterator pos = begin() + (pos_ - cbegin()); // cast to mutable iterator
        std::move_backward(pos, end(), end() + 1);
        *pos =  std::move(value);

        this->get_sizer_().set_size(size() + 1);
        return pos;
    }
    constexpr iterator insert( const_iterator pos_, size_type count, const_reference value ){
        APE_Expects(size() + count <= capacity() && pos_ <= cend());
        iterator pos = begin() + (pos_ - cbegin()); // cast to mutable iterator
        std::move_backward(pos, end(), end() + count);
        std::fill_n(pos, count, value);
        this->get_sizer_().set_size(size() + count);
        return pos;
    }

    template< std::forward_iterator ForwardItr>
        constexpr iterator insert( const_iterator pos_, ForwardItr first, ForwardItr last ){
            auto count = std::distance(first, last);
            APE_Expects(size() + count <= capacity() && pos_ <= cend());
            auto pos = begin() + (pos_ - cbegin());    // cast to iterator
            std::move_backward(pos, end(), end() + count);
            std::copy(first, last, pos);
            this->get_sizer_().set_size(size() + count);
            return pos;
        }
    constexpr iterator insert( const_iterator pos, std::initializer_list<value_type> ilist ){
        return insert(pos, ilist.begin(), ilist.end());
    }
    template< typename... Args > constexpr iterator emplace( const_iterator pos_, Args&&... args ){
        APE_Expects(!full() && pos_ <= cend());
        auto pos = begin() + (pos_ - cbegin());    // cast to iterator
        std::move_backward(pos, end(), end() + 1);
        *pos = value_type{std::forward<Args>(args)...};
        this->get_sizer_().set_size(size() + 1);
        return pos;
    }

    constexpr iterator erase( const_iterator pos_ ) noexcept{
        APE_Expects(pos_ < end() && pos_ >= begin());
        auto pos = begin() + (pos_ - cbegin());    // cast to iterator
        std::move(pos + 1, end(), pos);
        this->get_sizer_().set_size( size() - 1);
        return pos;
    }
    constexpr iterator erase( const_iterator first, const_iterator last ) noexcept{
        APE_Expects(first <= end() && first >= begin());
        APE_Expects(last <= end() && last >= begin());
        auto pos1 = begin() + (first - cbegin());    // cast to iterator
        auto pos2 = begin() + (last - cbegin());    // cast to iterator
        std::move(pos2, end(), pos1);
        this->get_sizer_().set_size(size() - (last - first));
        return pos1;
    }
    template< typename... Args > constexpr reference emplace_back( Args&&... args ){
        APE_Expects(!full());
        emplace(end(), std::forward<Args>(args)...);
        return back();
    }
    constexpr void push_back( const_reference value ){
        emplace_back(value);
    }
    constexpr void push_back( value_type&& value ){
        emplace_back(std::move(value));
    }
    constexpr void pop_back(){
        APE_Expects(!empty());
        this->get_sizer_().set_size(size() - 1);
    }

    void resize( size_type count, const_reference value = {}){
        APE_Expects(count <= capacity());
        if (count > size())
            std::fill_n(end(), count - size(), value);
        this->get_sizer_().set_size(count);
    }
    //template <buffer_policy BPolicy, buffer_policy SPolicy>
    //friend constexpr auto operator<=>(const basic_ref_vector<BPolicy, SPolicy> &lhs, const basic_ref_vector<BPolicy, SPolicy> &rhs) noexcept
    friend constexpr auto operator<=>(const basic_ref_vector &lhs, const basic_ref_vector &rhs) noexcept
    {
        return std::lexicographical_compare_three_way(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }
    friend constexpr bool operator==(const basic_ref_vector &lhs, const basic_ref_vector &rhs) noexcept{
        return (lhs <=> rhs) == 0;
    }
};


template<typename T, std::size_t N, size_policy SizePolicy = separated_policy<>>
using array_ref_vector = basic_ref_vector<std::array<T, N>, SizePolicy>;

template<typename T, std::size_t N = std::dynamic_extent, size_policy SizePolicy = separated_policy<>>
using span_ref_vector = basic_ref_vector<std::span<T, N>, SizePolicy>;

END_APE_NAMESPACE

#endif //END APE_ESTL_BASIC_REF_VECTOR_H
