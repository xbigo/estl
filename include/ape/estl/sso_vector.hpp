#ifndef APE_ESTL_SSO_VECTOR_H
#define APE_ESTL_SSO_VECTOR_H
#include <vector>

#include <gsl/gsl_assert>

#include <ape/estl/tiny_vector.hpp>

BEGIN_APE_NAMESPACE

template<typename T, std::size_t N, class Allocator = std::allocator<T>>
class sso_vector : public Allocator
{
    typedef array_ref_vector<T, N> small_type;
    typedef std::vector<T, Allocator> large_type;
    enum storage_type_{
        small, large
    };

    storage_type_ m_storage_type = small;

    union{
        small_type m_small_buffer;
        large_type m_large_buffer;
    };

    constexpr void switch_storage_(storage_type_ intend){
        if (intend == m_storage_type)
            return;
        if (m_storage_type == small){
            destruct(m_small_buffer);
            emplace_construct(m_large_buffer, get_allocator());
        }
        else{
            destruct(m_large_buffer);
            default_construct(m_small_buffer);
        }
        m_storage_type = intend;
    }
    constexpr bool shrink_to_small_storage_(){
        if (is_small()) return true;
        if (size() > N) return false;

        large_type tmp{std::move(m_large_buffer)};
        destruct(m_large_buffer);
        default_construct(m_small_buffer);
        m_storage_type = small;
        m_small_buffer.assign(tmp.begin(), tmp.end());

        return true;
    }
    constexpr void expand_to_large_storage_(){
        if (!is_small()) return;

        large_type tmp(m_small_buffer.begin(), m_small_buffer.end());
        destruct(m_small_buffer);
        default_construct(m_large_buffer);
        m_large_buffer.swap(tmp);
        m_storage_type = large;
    }
    using allocator_traits_ = std::allocator_traits<Allocator>;
    using allocator_traits = extra_allocator_traits<allocator_traits_>;
    using base = Allocator;
public:
    using value_type = T;
    using size_type = std::size_t ;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = const value_type& ;
    using pointer = T*;
    using const_pointer = const T*;
    using iterator = value_type*;
    using const_iterator = const value_type*;
    using reverse_iterator =  std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using allocator_type = Allocator ;
private:
    constexpr iterator iter_from_large_(typename large_type::iterator ret) noexcept{
        return m_large_buffer.data() + (ret - m_large_buffer.begin());
    }
    constexpr iterator iter_from_small_(typename small_type::iterator ret) noexcept{
        return m_small_buffer.data() + (ret - m_small_buffer.begin());
    }
    constexpr typename large_type::const_iterator iter_to_large_(const_iterator itr) noexcept{
        return m_large_buffer.begin() + (itr - cbegin());
    }
    constexpr typename small_type::const_iterator iter_to_small_(const_iterator itr) noexcept{
        return m_small_buffer.begin() + (itr - cbegin());
    }
public:
    constexpr sso_vector() noexcept(noexcept(allocator_type())){
        default_construct(m_small_buffer);
    }

    constexpr ~sso_vector() noexcept{
        if (m_storage_type == small)
            destruct(m_small_buffer);
        else
            destruct(m_large_buffer);
    }

    constexpr explicit sso_vector( const allocator_type& alloc ) noexcept
    : base(alloc)
    { }

    constexpr sso_vector(const sso_vector& rhs)
        : base(allocator_traits::select_on_copy(rhs.get_allocator()))
        , m_storage_type(rhs.m_storage_type)
    {
        if (m_storage_type == small)
            emplace_construct(m_small_buffer, rhs.m_small_buffer);
        else
            emplace_construct(m_large_buffer, rhs.m_large_buffer);
    }
    constexpr sso_vector(const sso_vector& rhs, const allocator_type& alloc)
        : base(alloc)
        , m_storage_type(rhs.m_storage_type)
    {
        if (m_storage_type == small)
            emplace_construct(m_small_buffer, rhs.m_small_buffer);
        else
            emplace_construct(m_large_buffer, rhs.m_large_buffer, alloc);
    }

    constexpr sso_vector(sso_vector&& rhs) noexcept
        : base(rhs.get_allocator())
        , m_storage_type(rhs.m_storage_type)
    {
        if (m_storage_type == small)
            emplace_construct(m_small_buffer, std::move(rhs.m_small_buffer));
        else
            emplace_construct(m_large_buffer, std::move(rhs.m_large_buffer));
    }
    constexpr sso_vector(sso_vector&& rhs, const allocator_type& alloc)
            noexcept(allocator_traits::always_equal())
        : base(alloc)
        , m_storage_type(rhs.m_storage_type)
    {
        if (m_storage_type == small)
            emplace_construct(m_small_buffer, std::move(rhs.m_small_buffer));
        else
            emplace_construct(m_large_buffer, std::move(rhs.m_large_buffer), alloc);
    }

    constexpr sso_vector(std::initializer_list<T> ilist, const allocator_type& alloc = allocator_type())
        : base(alloc)
        , m_storage_type(ilist.size() <= N ? small : large)
    {
        if (m_storage_type == small)
            emplace_construct(m_small_buffer, ilist);
        else
            emplace_construct(m_large_buffer, ilist, alloc);
    }

    constexpr sso_vector(size_type n, const T& t, const allocator_type& alloc = allocator_type())
        : base(alloc)
        , m_storage_type(n <= N ? small : large)
    {
        if (m_storage_type == small){
            default_construct(m_small_buffer);
            m_small_buffer.assign(n, t);
        }
        else
            emplace_construct(m_large_buffer, n, t, alloc);
    }
    constexpr explicit sso_vector(size_type n, const allocator_type& alloc = allocator_type())
    : sso_vector(n, T{}, alloc)
    {}

    template <std::forward_iterator Iter>
    constexpr sso_vector(Iter first, Iter last,
                     const allocator_type &alloc = allocator_type())
        : base(alloc)
    {
        auto length = std::distance(first, last);
        m_storage_type = (length <= N ? small : large);
        if (m_storage_type == small)
            emplace_construct(m_small_buffer, first, last);
        else
            emplace_construct(m_large_buffer, first, last, alloc);
    }

    constexpr sso_vector& operator=(const sso_vector& rhs){
        if (&rhs != this){
            switch_storage_(rhs.m_storage_type);

            if (m_storage_type == small)
                m_small_buffer = rhs.m_small_buffer;
            else
                m_large_buffer = rhs.m_large_buffer;
        }
        return *this;
    }

    constexpr sso_vector& operator=(sso_vector&& rhs) noexcept(allocator_traits::nothrow_move()){
        if (&rhs != this){
            switch_storage_(rhs.m_storage_type);

            if (rhs.m_storage_type == small)
                m_small_buffer = std::move(rhs.m_small_buffer);
            else
                m_large_buffer = std::move(rhs.m_large_buffer);
        }
        return *this;
    }

    sso_vector& operator=( std::initializer_list<T> ilist ){
        switch_storage_(ilist.size() <= N ? small : large);

        if (m_storage_type == small)
            m_small_buffer = ilist;
        else
            m_large_buffer = ilist;

        return *this;
    }

    constexpr void swap(sso_vector& rhs) noexcept{
        APE_Expects(allocator_traits::propagate_on_swap() || get_allocator() == rhs.get_allocator());
        auto tmp = std::move(rhs);
        rhs = std::move(*this);
        *this = std::move(tmp);
    }
    void assign( size_type n, const T& value ){
        switch_storage_(n <= N ? small : large);

        if (m_storage_type == small)
            m_small_buffer.assign(n, value);
        else
            m_large_buffer.assign(n, value);
    }
    template< std::forward_iterator ForwardItr>
        void assign( ForwardItr first, ForwardItr last ){

        size_type n = std::distance(first, last);
        switch_storage_(n <= N ? small : large);

        if (m_storage_type == small)
            m_small_buffer.assign(first, last);
        else
            m_large_buffer.assign(first, last);
    }

    void assign(std::initializer_list<T> ilist)
    {
        (*this) = ilist;
    }

    constexpr reference at( size_type pos ){
        if (pos >= size()) throw std::out_of_range ("sso_vector::at");
        return is_small() ? m_small_buffer[pos] : m_large_buffer[pos];
    }
    constexpr const_reference at( size_type pos ) const{
        if (pos >= size()) throw std::out_of_range ("sso_vector::at");
        return is_small() ? m_small_buffer[pos] : m_large_buffer[pos];
    }
    constexpr reference operator[]( size_type pos ) noexcept{
        APE_Expects(pos < size(), "index out of range sso_vector");
        return is_small() ? m_small_buffer[pos] : m_large_buffer[pos];
    }
    constexpr const_reference operator[]( size_type pos ) const noexcept{
        APE_Expects(pos < size(), "index out of range sso_vector");
        return is_small() ? m_small_buffer[pos] : m_large_buffer[pos];
    }
    constexpr reference front() noexcept{
        APE_Expects(!empty(), "get front from empty sso_vector");
        return is_small() ? m_small_buffer.front() : m_large_buffer.front();
    }
    constexpr const_reference front() const noexcept{
        APE_Expects(!empty(), "get front from empty sso_vector");
        return is_small() ? m_small_buffer.front() : m_large_buffer.front();
    }
    constexpr reference back() noexcept{
        APE_Expects(!empty(), "get back from empty sso_vector");
        return is_small() ? m_small_buffer.back() : m_large_buffer.back();
    }
    constexpr const_reference back() const noexcept{
        APE_Expects(!empty(), "get back from empty sso_vector");
        return is_small() ? m_small_buffer.back() : m_large_buffer.back();
    }
    constexpr pointer data() noexcept{
        return is_small() ? m_small_buffer.data() : m_large_buffer.data();
    }
    constexpr const_pointer  data() const noexcept{
        return is_small() ? m_small_buffer.data() : m_large_buffer.data();
    }
    constexpr iterator begin() noexcept {
        return data();
    }
    constexpr const_iterator begin() const noexcept{
        return data();
    }
    constexpr const_iterator cbegin() const noexcept{
        return begin();
    }
    constexpr iterator end() noexcept {
        return data() + size();
    }
    constexpr const_iterator end() const noexcept{
        return data() + size();
    }
    constexpr const_iterator cend() const noexcept {
        return end();
    }
    constexpr reverse_iterator rbegin() noexcept{
        return reverse_iterator(end());
    }
    constexpr const_reverse_iterator  rbegin() const noexcept {
        return const_reverse_iterator(cend());
    }
    constexpr const_reverse_iterator crbegin() const noexcept {
        return const_reverse_iterator(cend());
    }
    constexpr reverse_iterator rend() noexcept {
        return reverse_iterator(begin());
    }
    constexpr const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator(cbegin());
    }
    constexpr const_reverse_iterator crend() const noexcept {
        return const_reverse_iterator(cbegin());
    }

    constexpr bool is_small() const noexcept{ return m_storage_type == small;}
    constexpr size_type small_capacity() const noexcept{ return N; }
    constexpr bool empty() const noexcept{ return size() == 0; }
    constexpr size_type size() const noexcept{ return is_small() ? m_small_buffer.size() : m_large_buffer.size(); }
    constexpr allocator_type get_allocator() const noexcept { return *this;}
    constexpr size_type max_size() const noexcept{
        return allocator_traits::max_size(get_allocator());
    }
    constexpr size_type capacity() const noexcept{
        return is_small() ? N : m_large_buffer.capacity();
    }
    //Modifiler
    constexpr void clear() noexcept{
        if (is_small())
            m_small_buffer.clear();
        else
            m_large_buffer.clear();
    }

    constexpr iterator insert( const_iterator pos_, const T& value ){
        return emplace(pos_, value);
    }

    constexpr iterator insert( const_iterator pos_, T&& value ){
        return emplace(pos_, std::move(value));
    }

    constexpr iterator insert( const_iterator pos_, size_type count, const T& value ){
        if (is_small()){
            if (m_small_buffer.size() + count <= N)
                return iter_from_small_(m_small_buffer.insert(iter_to_small_(pos_), count, value));
            expand_to_large_storage_();
        }
        auto ret = m_large_buffer.insert(iter_to_large_(pos_), count, value);
        return iter_from_large_(ret);
    }

    template< std::forward_iterator ForwardItr>
        constexpr iterator insert( const_iterator pos_, ForwardItr first, ForwardItr last ){
        auto count = std::distance(first, last);
        if (is_small())
        {
            if (m_small_buffer.size() + count <= N)
                return iter_from_small_(m_small_buffer.insert(iter_to_small_(pos_), first, last));
            expand_to_large_storage_();
        }
        auto ret = m_large_buffer.insert(iter_to_large_(pos_), first, last);
        return iter_from_large_(ret);
    }

    constexpr iterator insert( const_iterator pos, std::initializer_list<T> ilist ){
        return insert(pos, ilist.begin(), ilist.end());
    }

    template< typename... Args > constexpr iterator emplace(const_iterator pos_, Args&&... args ){
        if (is_small())
        {
            if (m_small_buffer.size() < N)
                return iter_from_small_(m_small_buffer.emplace(iter_to_small_(pos_), std::move(args)...));
            expand_to_large_storage_();
        }
        auto ret = m_large_buffer.emplace(iter_to_large_(pos_), std::move(args)...);
        return iter_from_large_(ret);
    }

    constexpr iterator erase( const_iterator pos_ ) noexcept{
        if (is_small())
            return iter_from_small_(m_small_buffer.erase(iter_to_small_(pos_)));

        auto ret = m_large_buffer.erase(iter_to_large_(pos_));
        return iter_from_large_(ret);
    }

    constexpr iterator erase( const_iterator first, const_iterator last ) noexcept{
        if (is_small())
            return iter_from_small_(m_small_buffer.erase(iter_to_small_(first), iter_to_small_(last)));

        auto ret = m_large_buffer.erase(iter_to_large_(first), iter_to_large_(first));
        return iter_from_large_(ret);
    }

    template< typename... Args > constexpr reference emplace_back( Args&&... args ){
        return *emplace(cend(), std::move(args)...);
    }
    constexpr void push_back( const T& value ){
        emplace_back(value);
    }
    constexpr void push_back( T&& value ){
        emplace_back(std::move(value));
    }
    constexpr void pop_back() noexcept{
        APE_Expects(!empty(), "pop empty sso_vector");
        if (is_small())
            m_small_buffer.pop_back();
        m_large_buffer.pop_back();
    }

    void resize( size_type count, const value_type& value = {} ){
        if (count <= N && is_small()){
                m_small_buffer.resize(count, value);
        }
        else {
            expand_to_large_storage_();
            m_large_buffer.resize(count, value);
        }
    }
};

template< class T, std::size_t N, class SP1, class SP2 >
constexpr auto operator<=>(const sso_vector<T,N, SP1>& lhs, const sso_vector<T,N,SP2>& rhs ) noexcept{
    return std::lexicographical_compare_three_way(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

END_APE_NAMESPACE

#endif //END APE_ESTL_SSO_VECTOR_H
