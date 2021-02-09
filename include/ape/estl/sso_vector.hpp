#ifndef APE_ESTL_SSO_VECTOR_H
#define APE_ESTL_SSO_VECTOR_H
#include <algorithm> // for std::min/max
#include <memory> // for std::addressof
#include <array>
#include <vector>
#include <initializer_list>
#ifdef CPP_20
#include <compare>
#endif

#include <gsl/gsl_assert>

#include <ape/estl/memory.hpp>
#include <ape/estl/tiny_vector.hpp>

BEGIN_APE_NAMESPACE

template<typename T, std::size_t N, class Allocator = std::allocator<T>>
class sso_vector : public Allocator
{
	typedef tiny_vector<T, N> small_type;
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
			default_construct(m_large_buffer);
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

	public:

	typedef T value_type;
	typedef std::size_t size_type;
	typedef std::ptrdiff_t difference_type;
	typedef value_type& reference;
	typedef const value_type& const_reference;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef value_type* iterator;
	typedef const value_type* const_iterator;
	typedef std::reverse_iterator<iterator>         reverse_iterator;
	typedef std::reverse_iterator<const_iterator>   const_reverse_iterator;

	constexpr sso_vector() noexcept{
		default_construct(m_small_buffer);
	}

	~sso_vector() noexcept{ 
		if (m_storage_type == small) 
			destruct(m_small_buffer);
		else
			destruct(m_large_buffer);
	}

	constexpr sso_vector(const sso_vector& rhs)	
		: m_storage_type(rhs.m_storage_type)
	{
		if (m_storage_type == small)
			emplace_construct(m_small_buffer, rhs.m_small_buffer);
		else
			emplace_construct(m_large_buffer, rhs.m_large_buffer);
	}

	constexpr sso_vector(sso_vector&& rhs) noexcept
		: m_storage_type(rhs.m_storage_type)
	{
		if (m_storage_type == small)
			emplace_construct(m_small_buffer, std::move(rhs.m_small_buffer));
		else
			emplace_construct(m_large_buffer, std::move(rhs.m_large_buffer));
	}

	constexpr sso_vector(std::initializer_list<T> ilist )
		: m_storage_type(ilist.size() <= N ? small : large)
	{
		if (ilist.size() <= N)
			emplace_construct(m_small_buffer, ilist);
		else {
			emplace_construct(m_large_buffer, ilist);
		}
	}

	constexpr sso_vector(size_type n, const T& t = T())
	 	: m_storage_type(n <= N ? small : large)
	{
		if (n <= N)
			emplace_construct(m_small_buffer, n, t);
		else 
			emplace_construct(m_large_buffer, n, t);
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

	constexpr sso_vector& operator=(sso_vector&& rhs) noexcept{
		if (&rhs != this){
			switch_storage_(rhs.m_storage_type);

			if (rhs.m_storage_type == small)
				m_small_buffer = std::move(rhs.m_small_buffer);
			else
				m_large_buffer = std::move(rhs.m_large_buffer);
		}
		return *this;
	}
	constexpr void swap(sso_vector& rhs) noexcept{
		auto tmp = std::move(rhs);
		rhs = std::move(*this);
		*this = std::move(tmp);
	}
	
	sso_vector& operator=( std::initializer_list<T> ilist ){
		switch_storage_(ilist.size() <= N ? small : large);

		if (m_storage_type == small)
			m_small_buffer = ilist;
		else
			m_large_buffer = ilist;

		return *this;
	}
	void assign( size_type n, const T& value ){
		switch_storage_(n <= N ? small : large);

		if (m_storage_type == small)
			m_small_buffer.assign(n, value);
		else
			m_large_buffer.assign(n, value);
	}
	template< class ForwardItr, class = std::enable_if_t<!std::is_integral<ForwardItr>::value> >
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
	constexpr T* data() noexcept{ 
		return is_small() ? m_small_buffer.data() : m_large_buffer.data();
	}
	constexpr const T* data() const noexcept{ 
		return is_small() ? m_small_buffer.data() : m_large_buffer.data();
	}
	constexpr iterator begin() noexcept { 
		return data();
	}
	constexpr const_iterator begin() const noexcept{ 
		return data();
	}
	constexpr const_iterator cbegin() const noexcept{ 
		return data();
	}
	constexpr iterator end() noexcept { 
		return data() + size();
	}
	constexpr const_iterator end() const noexcept{ 
		return data() + size();
	}
	constexpr const_iterator cend() const noexcept { 
		return data() + size();
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
	constexpr size_type max_size() const noexcept{ 
		return Allocator().max_size();
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
		auto idx = (pos_ - cbegin());
		if (is_small()){
			if (m_small_buffer.size() + count <= N)
				return m_small_buffer.insert(pos_, count, value);
			expand_to_large_storage_();
		}
		auto pos = m_large_buffer.begin() + idx;
		auto ret = m_large_buffer.insert(pos, count, value);
		return m_large_buffer.data() + (ret - m_large_buffer.begin());
	}

	template< class ForwardItr, class = std::enable_if_t<!std::is_integral<ForwardItr>::value> >
		constexpr iterator insert( const_iterator pos_, ForwardItr first, ForwardItr last ){
		auto count = std::distance(first, last);
		auto idx = (pos_ - cbegin());
		if (is_small())
		{
			if (m_small_buffer.size() + count <= N)
				return m_small_buffer.insert(pos_, first, last);
			expand_to_large_storage_();
		}
		auto pos = m_large_buffer.begin() + idx;
		auto ret = m_large_buffer.insert(pos, first, last);
		return m_large_buffer.data() + (ret - m_large_buffer.begin());
	}

	constexpr iterator insert( const_iterator pos, std::initializer_list<T> ilist ){
		return insert(pos, ilist.begin(), ilist.end());
	}

	template< typename... Args > constexpr iterator emplace(const_iterator pos_, Args&&... args ){
		auto idx = (pos_ - cbegin());
		if (is_small())
		{
			if (m_small_buffer.size() < N)
				return m_small_buffer.emplace(pos_, std::move(args)...);
			expand_to_large_storage_();
		}
		auto pos = m_large_buffer.begin() + idx;
		auto ret = m_large_buffer.emplace(pos, std::move(args)...);
		return m_large_buffer.data() + (ret - m_large_buffer.begin());
	}

	constexpr iterator erase( const_iterator pos_ ) noexcept{
		if (is_small())
			return m_small_buffer.erase(pos_);

		auto pos = m_large_buffer.begin() + (pos_ - cbegin());
		auto ret = m_large_buffer.erase(pos);
		return m_large_buffer.data() + (ret - m_large_buffer.begin());
	}

	constexpr iterator erase( const_iterator first, const_iterator last ) noexcept{
		if (is_small())
			return m_small_buffer.erase(first, last);

		auto from = m_large_buffer.begin() + (first - cbegin());
		auto to = m_large_buffer.begin() + (last - cbegin());
		auto ret = m_large_buffer.erase(from, to);
		return m_large_buffer.data() + (ret - m_large_buffer.begin());
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
	constexpr void pop_back(){
		if (is_small())
			m_small_buffer.pop_back();
		m_large_buffer.pop_back();
	}

	void resize( size_type count ){
		if (count <= N && is_small()){
				m_small_buffer.resize(count);
		}
		else {
			expand_to_large_storage_();
			m_large_buffer.resize(count);
		}
	}
	void resize( size_type count, const value_type& value ){
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
constexpr bool operator==( const sso_vector<T,N, SP1>& lhs, const sso_vector<T,N,SP2>& rhs ){
	return lhs.size() == rhs.size()
		&& std::equal(std::begin(lhs), std::end(lhs), std::begin(rhs));
}

template< class T, std::size_t N, class SP1, class SP2 >
constexpr bool operator!=( const sso_vector<T,N,SP1>& lhs, const sso_vector<T,N,SP2>& rhs ){
	return !(lhs == rhs);
}

template< class T, std::size_t N, class SP1, class SP2 >
constexpr bool operator<( const sso_vector<T,N,SP1>& lhs, const sso_vector<T,N,SP2>& rhs ){
	return std::lexicographical_compare(std::begin(lhs), std::end(lhs), std::begin(rhs), std::end(rhs));
}

template< class T, std::size_t N, class SP1, class SP2 >
constexpr bool operator<=( const sso_vector<T,N,SP1>& lhs, const sso_vector<T,N,SP2>& rhs ){
	return !(rhs < lhs);
}

template< class T, std::size_t N, class SP1, class SP2 >
constexpr bool operator>( const sso_vector<T,N,SP1>& lhs, const sso_vector<T,N,SP2>& rhs ){
	return rhs < lhs;
}

template< class T, std::size_t N, class SP1, class SP2 >
constexpr bool operator>=( const sso_vector<T,N,SP1>& lhs, const sso_vector<T,N,SP2>& rhs ){
	return !(lhs < rhs);
}

END_APE_NAMESPACE

#endif //END APE_ESTL_SSO_VECTOR_H

