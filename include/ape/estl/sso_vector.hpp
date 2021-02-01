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
	static constexpr std::size_t uninit_ = std::size_t(-1);
	size_t m_size = 0;
	typedef tiny_vector<T, N> small_type;
	typedef std::vector<T, Allocator> large_type;

	union{ 
		small_type m_small_buffer; 
		large_type m_large_buffer;
	};
	constexpr bool is_broken_() const noexcept{ return m_size == uninit_; }
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

	constexpr sso_vector() noexcept{}
	~sso_vector() noexcept{ clear(); }
	constexpr sso_vector(const sso_vector& rhs)
	{
		if (rhs.is_small())
			emplace_construct(m_small_buffer, rhs.m_small_buffer);
		else 
			emplace_construct(m_large_buffer, rhs.m_large_buffer);
		m_size = rhs.size();
	}
	constexpr sso_vector(sso_vector&& rhs) noexcept{
		if (rhs.is_small())
			emplace_construct(m_small_buffer, std::move(rhs.m_small_buffer));
		else 
			emplace_construct(m_large_buffer, std::move(rhs.m_large_buffer));
		m_size = rhs.size();
		rhs.clear();
	}
	constexpr sso_vector(std::initializer_list<T> ilist ){
		if (ilist.size() > N)
			emplace_construct(m_large_buffer, ilist);
		else
			emplace_construct(m_small_buffer, ilist);
		m_size = ilist.size();
	}
	constexpr sso_vector& operator=(const sso_vector& rhs){
		if (&rhs != this){
			clear();
			if (rhs.is_small())
				m_small_buffer = rhs.m_small_buffer;
			else {
				destruct(m_small_buffer); 
				m_size = uninit_;// base guarantee
				emplace_construct(m_large_buffer, rhs.m_large_buffer);
			}
			m_size = rhs.size();
		}
		return *this;
	}
	constexpr sso_vector& operator=(sso_vector&& rhs) noexcept{
		if (&rhs != this){
			clear();
			if (rhs.is_small())
					m_small_buffer = std::move(rhs.m_small_buffer);
			else {
				destruct(m_small_buffer); 
				m_size = uninit_;// base guarantee
				emplace_construct(m_large_buffer, std::move(rhs.m_large_buffer));
			}
			m_size = rhs.size();
			rhs.clear();
		}
		return *this;
	}
	constexpr void swap(sso_vector& rhs) noexcept{
		auto tmp = std::move(rhs);
		rhs = std::move(*this);
		*this = std::move(tmp);
	}

	constexpr sso_vector(size_type n, const T& t = T()){
		if (n <= N)
			emplace_construct(m_small_buffer, n, t);
		else 
			emplace_construct(m_large_buffer, n, t);
		m_size = n;
	}
	sso_vector& operator=( std::initializer_list<T> ilist ){
		clear();
		if (n <= N)
			emplace_construct(m_small_buffer, n, t);
		else {
			destruct(m_small_buffer); 
			m_size = uninit_;// base guarantee
			emplace_construct(m_large_buffer, n, t);
		}
		m_size = ilist.size();
		return *this;
	}
	void assign( size_type n, const T& value ){
		clear();
		if (n <= N)
			m_small_buffer.assign(n, value);
		else{
			destruct(m_small_buffer); 
			m_size = uninit_;// base guarantee
			emplace_construct(m_large_buffer, n, value);
		}
		m_size = n;
	}
	template< class ForwardItr, class = std::enable_if_t<!std::is_integral<ForwardItr>::value> >
		void assign( ForwardItr first, ForwardItr last ){
			clear();
			size_type n = std::distance(first, last);
			if (n <= N)
				m_small_buffer.assign(n, value);
			else{
				destruct(m_small_buffer); 
				m_size = uninit_;// base guarantee
				emplace_construct(m_large_buffer, n, value);
			}

			m_size = n;
		}

	void assign( std::initializer_list<T> ilist ){
		(*this) = ilist;
	}

	constexpr reference at( size_type pos ){
		if (pos >= size()) throw std::out_of_range ("sso_vector::at");
		return size() < N ? m_small_buffer[pos] : m_large_buffer[pos];
	}
	constexpr const_reference at( size_type pos ) const{
		if (pos >= size()) throw std::out_of_range ("sso_vector::at");
		return size() < N ? m_small_buffer[pos] : m_large_buffer[pos];
	}
	constexpr reference operator[]( size_type pos ) noexcept{
		APE_Expects(pos < size());
		return size() < N ? m_small_buffer[pos] : m_large_buffer[pos];
	}
	constexpr const_reference operator[]( size_type pos ) const noexcept{
		APE_Expects(pos < size());
		return size() < N ? m_small_buffer[pos] : m_large_buffer[pos];
	}
	constexpr reference front() noexcept{
		APE_Expects(!empty());
		return size() < N ? m_small_buffer.front() : m_large_buffer.front();
	}
	constexpr const_reference front() const noexcept{
		APE_Expects(!empty());
		return size() < N ? m_small_buffer.front() : m_large_buffer.front();
	}
	constexpr reference back() noexcept{
		APE_Expects(!empty());
		return size() < N ? m_small_buffer.back() : m_large_buffer.back();
	}
	constexpr const_reference back() const noexcept{
		APE_Expects(!empty());
		return size() < N ? m_small_buffer.back() : m_large_buffer.back();
	}
	constexpr T* data() noexcept{ 
		if (is_broken_()) return nullptr;
		return is_small() ? m_small_buffer.data() : m_large_buffer.data();
	}
	constexpr const T* data() const noexcept{ 
		if (is_broken_()) return nullptr;
		return is_small() ? m_small_buffer.data() : m_large_buffer.data();
	}
	constexpr iterator begin() noexcept { 
		return is_broken_() ?  nullptr : &front();
	}
	constexpr const_iterator begin() const noexcept{ 
		return is_broken_() ?  nullptr : &front();
	}
	constexpr const_iterator cbegin() const noexcept{ 
		return is_broken_() ?  nullptr : &front();
	}
	constexpr iterator end() noexcept { 
		return is_broken_() ?  nullptr : &front() + size();
	}
	constexpr const_iterator end() const noexcept{ 
		return is_broken_() ?  nullptr : &front() + size();
	}
	constexpr const_iterator cend() const noexcept { 
		return is_broken_() ?  nullptr : &front() + size();
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

	constexpr bool is_small() const noexcept{ return size() <= N; }
	constexpr size_type small_capacity() const noexcept{ return N; }
	constexpr bool empty() const noexcept{ return size() == 0; }
	constexpr size_type size() const noexcept{ return m_size == uninit_ ? 0 : m_size; }
	constexpr size_type max_size() const noexcept{ 
		
	}
	constexpr size_type capacity() const noexcept{ 
		return is_small() ? N : m_large_buffer.capacity();
	}
	//Modifiler
	constexpr void clear() noexcept{
		if (m_size > N){
				destruct(m_large_buffer);
				default_construct(m_small_buffer);
		}
		else if (m_size != uninit_)
			m_small_buffer.clear();
		if (m_size == uninit_)
			default_construct(m_small_buffer);
		m_size = 0;
	}

	constexpr iterator insert( const_iterator pos_, const T& value ){
		return emplace(pos_, value);
	}
	constexpr iterator insert( const_iterator pos_, T&& value ){
		APE_Expects(!full() && pos_ <= cend());
		auto pos = begin() + (pos_ - cbegin());	// cast to iterator
		if (pos < end()){
			emplace_construct(*end(), std::move(back()));
			std::move_backward(pos, end() - 1, end());
		}
		emplace_construct(*pos, std::move(value));

		SizePolicy::set_size(size() + 1);
		return pos;
	}
	constexpr iterator insert( const_iterator pos_, size_type count, const T& value ){
		APE_Expects(size() + count <= N && pos_ <= cend());
		auto pos = begin() + (pos_ - cbegin());	// cast to iterator
		size_type right_size = end() - pos;
		//  From: |   |/////|
		//   To1: |   |/////|\\\\|
		//   To2: |   |xxxxx|\\\\|/////|
		if (right_size < count){
			std::uninitialized_fill_n(end(), count - right_size, value);
			std::uninitialized_move(pos, end(), pos + count);
		}
		//   To1: |   |///xx//|
		//   To2: |   |xx/////|
		else if(count > 0){
			std::uninitialized_move(end() - count, end(), end());
			std::move_backward(pos, right_size - count + pos, end());
		}
		SizePolicy::set_size(count + size()); // adjust size, basic guarantee
		std::fill_n(pos, std::min(count, right_size), value);
		return pos;
	}
	template< class ForwardItr, class = std::enable_if_t<!std::is_integral<ForwardItr>::value> >
		constexpr iterator insert( const_iterator pos_, ForwardItr first, ForwardItr last ){
			auto count = std::distance(first, last);
			APE_Expects(size() + count <= N && pos_ <= cend());
			auto pos = begin() + (pos_ - cbegin());	// cast to iterator
			auto right_size = end() - pos;
			if (right_size < count){
				auto mitr = first;
				std::advance(mitr, right_size);
				std::uninitialized_copy(mitr, last, end());
				std::uninitialized_move(pos, end(), pos + count);
			}
			else if(count > 0){
				std::uninitialized_move(end() - count, end(), end());
				std::move_backward(pos, right_size - count + pos, end());
			}
			SizePolicy::set_size(count + size()); // adjust size, basic guarantee
			std::copy(first, first + std::min(count, right_size), pos);
			return pos;
		}
	constexpr iterator insert( const_iterator pos, std::initializer_list<T> ilist ){
		return insert(pos, ilist.begin(), ilist.end());
	}
	template< typename... Args > constexpr iterator emplace( const_iterator pos_, Args&&... args ){
		APE_Expects(!full() && pos_ <= cend());
		auto pos = begin() + (pos_ - cbegin());	// cast to iterator
		if (pos < end()){
			emplace_construct(*end(), std::move(back()));
			std::move_backward(pos, end() - 1, end());
		}
		emplace_construct(*pos, std::forward<Args>(args)...);

		SizePolicy::set_size(size() + 1);
		return pos;
	}

	constexpr iterator erase( const_iterator pos_ ) noexcept{
		APE_Expects(pos_ < end() && pos_ >= begin());
		auto pos = begin() + (pos_ - cbegin());	// cast to iterator
		std::move(pos + 1, end(), pos);
		destruct(back());
		SizePolicy::set_size(size() - 1);
		return pos;
	}
	constexpr iterator erase( const_iterator first, const_iterator last ) noexcept{
		APE_Expects(first <= end() && first >= begin());
		APE_Expects(last <= end() && last >= begin());
		auto pos1 = begin() + (first - cbegin());	// cast to iterator
		auto pos2 = begin() + (last - cbegin());	// cast to iterator
		std::move(pos2, end(), pos1);
		destruct(pos2, end());

		SizePolicy::set_size(size() - (last - first));
		return pos1;
	}

	template< typename... Args > constexpr reference emplace_back( Args&&... args ){
		APE_Expects(!full());
		emplace_construct(m_data[size()], std::forward<Args>(args)...);
		SizePolicy::set_size(size() + 1);
		return back();
	}
	constexpr void push_back( const T& value ){
		emplace_back(value);
	}
	constexpr void push_back( T&& value ){
		emplace_back(std::move(value));
	}
	constexpr void pop_back(){
		APE_Expects(!empty());
		destruct(back());
		SizePolicy::set_size(size() - 1);
	}

	void resize( size_type count ){
		APE_Expects(count <= N);
		destruct(std::min(begin() + count, end()), end());
		for (auto i = end(); i < begin() + count; ++i)
			default_construct(*i);
		SizePolicy::set_size(count);
	}
	void resize( size_type count, const value_type& value ){
		APE_Expects(count <= N);
		for (auto i = begin() + count; i < end(); ++i)
			destruct(*i);
		std::fill(end(), std::max(end(), begin() + count), value);
		SizePolicy::set_size(count);
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

