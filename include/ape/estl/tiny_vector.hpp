#ifndef APE_ESTL_TINY_VECTOR_H
#define APE_ESTL_TINY_VECTOR_H
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

BEGIN_APE_NAMESPACE
struct InnerSizePolicy{
	std::size_t m_size = 0;

	constexpr std::size_t get_size() const noexcept{return m_size;};
	constexpr void set_size(std::size_t n) noexcept{ m_size = n;}
};

template<std::ptrdiff_t Offset>
struct OuterSizePolicy{
	constexpr std::size_t get_size() const noexcept{
		return *reinterpret_cast<const std::size_t*>(reinterpret_cast<const char*>(this) + Offset);
	}
	constexpr void set_size(std::size_t n) noexcept{
		*reinterpret_cast<std::size_t*>(reinterpret_cast<char*>(this) + Offset) = n;
	}
};

template<typename T, std::size_t N, typename SizePolicy = InnerSizePolicy>
class tiny_vector : private SizePolicy
{
	union{ std::array<T, N> m_data; };
	public:
	typedef std::array<T, N> base_type;
	typedef T value_type;
	typedef std::size_t size_type;
	typedef std::ptrdiff_t difference_type;
	typedef value_type& reference;
	typedef const value_type& const_reference;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef typename base_type::iterator iterator;
	typedef typename base_type::const_iterator const_iterator;
	typedef typename base_type::reverse_iterator reverse_iterator;
	typedef typename base_type::const_reverse_iterator const_reverse_iterator;

	constexpr tiny_vector() noexcept{}
	~tiny_vector() noexcept{ clear(); }
	constexpr tiny_vector(const tiny_vector& rhs){
		std::uninitialized_copy(rhs.begin(), rhs.end(), m_data.begin());
		SizePolicy::set_size(rhs.size());
	}
	constexpr tiny_vector(tiny_vector&& rhs) noexcept{
		std::uninitialized_move(rhs.begin(), rhs.end(), &*m_data.begin());
		SizePolicy::set_size(rhs.size());
	}
	constexpr tiny_vector(std::initializer_list<T> ilist ){
		Expects(ilist.size() <= N);
		std::uninitialized_move(ilist.begin(), ilist.end(), m_data.begin());
		SizePolicy::set_size(ilist.size());
	}
	constexpr tiny_vector& operator=(const tiny_vector& rhs){
		if (&rhs != this){
			auto s = std::min(size(), rhs.size());
			std::copy(rhs.begin(), rhs.begin() + s, begin());
			destruct(begin() + s, end());
			std::uninitialized_copy(rhs.begin() + s, rhs.end(), end());
			SizePolicy::set_size(rhs.size());
		}
		return *this;
	}
	constexpr tiny_vector& operator=(tiny_vector&& rhs) noexcept{
		if (&rhs != this){
			auto s = std::min(size(), rhs.size());
			std::move(rhs.begin(), rhs.begin() + s, m_data.begin());
			destruct(begin() + s, end());
			std::uninitialized_move(rhs.begin() + s, rhs.end(), end());
			SizePolicy::set_size(rhs.size());
		}
		return *this;
	}
	constexpr void swap(tiny_vector& rhs) noexcept{
		auto tmp = std::move(rhs);
		rhs = std::move(*this);
		*this = std::move(tmp);
	}

	constexpr tiny_vector(size_type n, const T& t = T()){
		Expects(n <= N);
		std::uninitialized_fill_n(m_data.begin(), n, t);
		SizePolicy::set_size(n);

	}
	tiny_vector& operator=( std::initializer_list<T> ilist ){
		Expects(ilist.size() <= N);
		std::uninitialized_copy(ilist.begin(), ilist.end(), m_data.begin());
		SizePolicy::set_size(ilist.size());
		return *this;
	}
	void assign( size_type n, const T& value ){
		Expects(n <= N);
		auto s = std::min(size(), n);
		std::fill_n(m_data.begin(), s, value);
		std::uninitialized_fill_n(m_data.begin() + s, n - s, value);
		SizePolicy::set_size(n);
	}
	template< class ForwardItr, class = std::enable_if_t<!std::is_integral<ForwardItr>::value> >
		void assign( ForwardItr first, ForwardItr last ){
			size_type n = std::distance(first, last);
			Expects(n <= N);

			auto s = std::min(size(), n);
			auto itr = m_data.begin();
			for(size_type i = 0; i < s; ++i)
				*itr++ = *first++;
			std::uninitialized_copy(first, last, itr);
			SizePolicy::set_size(n);
		}

	void assign( std::initializer_list<T> ilist ){
		assign(ilist.begin(), ilist.end());
	}

	constexpr reference at( size_type pos ){
		if (pos >= size()) throw std::out_of_range ("tiny_vector::at");
		return m_data[pos];
	}
	constexpr const_reference at( size_type pos ) const{
		if (pos >= size()) throw std::out_of_range ("tiny_vector::at");
		return m_data[pos];
	}
	constexpr reference operator[]( size_type pos ) noexcept{
		Expects(pos < size());
		return m_data[pos];
	}
	constexpr const_reference operator[]( size_type pos ) const noexcept{
		Expects(pos < size());
		return m_data[pos];
	}
	constexpr reference front() noexcept{
		Expects(!empty());
		return m_data[0];
	}
	constexpr const_reference front() const noexcept{
		Expects(!empty());
		return m_data[0];
	}
	constexpr reference back() noexcept{
		Expects(!empty());
		return m_data[size() - 1];
	}
	constexpr const_reference back() const noexcept{
		Expects(!empty());
		return m_data[size() - 1];
	}
	constexpr T* data() noexcept{ return &*m_data.begin(); }
	constexpr const T* data() const noexcept{ return &*m_data.begin(); }
	constexpr iterator begin() noexcept { return m_data.begin();}
	constexpr const_iterator begin() const noexcept{ return m_data.begin();}
	constexpr const_iterator cbegin() const noexcept{ return m_data.cbegin();}
	constexpr iterator end() noexcept { return m_data.begin() + size();}
	constexpr const_iterator end() const noexcept{ return m_data.begin() + size();}
	constexpr const_iterator cend() const noexcept { return m_data.cbegin() + size();}

	constexpr reverse_iterator rbegin() noexcept{ return m_data.rbegin() + (N- size());}
	constexpr const_reverse_iterator  rbegin() const noexcept { return m_data.rbegin() + (N - size());}
	constexpr const_reverse_iterator crbegin() const noexcept { return m_data.crbegin() + (N - size());}
	constexpr reverse_iterator rend() noexcept { return m_data.rend();}
	constexpr const_reverse_iterator rend() const noexcept { return m_data.rend();}
	constexpr const_reverse_iterator crend() const noexcept { return m_data.crend();}

	constexpr bool full() const noexcept{ return size() == N; }
	constexpr bool empty() const noexcept{ return size() == 0; }
	constexpr size_type size() const noexcept{ return SizePolicy::get_size(); }
	constexpr size_type max_size() const noexcept{ return N; }
	constexpr size_type capacity() const noexcept{ return N; }
	//Modifiler
	constexpr void fill( const T& value ){ assign(N, value); }
	constexpr void clear() noexcept{
		destruct(begin(), end());
		SizePolicy::set_size(0);
	}

	constexpr iterator insert( const_iterator pos_, const T& value ){
		return emplace(pos_, value);
	}
	constexpr iterator insert( const_iterator pos_, T&& value ){
		Expects(!full() && pos_ <= cend());
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
		Expects(size() + count <= N && pos_ <= cend());
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
			Expects(size() + count <= N && pos_ <= cend());
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
		Expects(!full() && pos_ <= cend());
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
		Expects(pos_ < end() && pos_ >= begin());
		auto pos = begin() + (pos_ - cbegin());	// cast to iterator
		std::move(pos + 1, end(), pos);
		destruct(back());
		SizePolicy::set_size(size() - 1);
		return pos;
	}
	constexpr iterator erase( const_iterator first, const_iterator last ) noexcept{
		Expects(first <= end() && first >= begin());
		Expects(last <= end() && last >= begin());
		auto pos1 = begin() + (first - cbegin());	// cast to iterator
		auto pos2 = begin() + (last - cbegin());	// cast to iterator
		std::move(pos2, end(), pos1);
		destruct(pos2, end());

		SizePolicy::set_size(size() - (last - first));
		return pos1;
	}

	template< typename... Args > constexpr reference emplace_back( Args&&... args ){
		Expects(!full());
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
		Expects(!empty());
		destruct(back());
		SizePolicy::set_size(size() - 1);
	}

	void resize( size_type count ){
		Expects(count <= N);
		destruct(std::min(begin() + count, end()), end());
		for (auto i = end(); i < begin() + count; ++i)
			default_construct(*i);
		SizePolicy::set_size(count);
	}
	void resize( size_type count, const value_type& value ){
		Expects(count <= N);
		for (auto i = begin() + count; i < end(); ++i)
			destruct(*i);
		std::fill(end(), std::max(end(), begin() + count), value);
		SizePolicy::set_size(count);
	}
};

#ifndef CPP_20
template< class T, std::size_t N, class SP1, class SP2 >
constexpr bool operator==( const tiny_vector<T,N, SP1>& lhs, const tiny_vector<T,N,SP2>& rhs ){
	return lhs.size() == rhs.size()
		&& std::equal(std::begin(lhs), std::end(lhs), std::begin(rhs));
}

template< class T, std::size_t N, class SP1, class SP2 >
constexpr bool operator!=( const tiny_vector<T,N,SP1>& lhs, const tiny_vector<T,N,SP2>& rhs ){
	return !(lhs == rhs);
}

template< class T, std::size_t N, class SP1, class SP2 >
constexpr bool operator<( const tiny_vector<T,N,SP1>& lhs, const tiny_vector<T,N,SP2>& rhs ){
	return std::lexicographical_compare(std::begin(lhs), std::end(lhs), std::begin(rhs), std::end(rhs));
}

template< class T, std::size_t N, class SP1, class SP2 >
constexpr bool operator<=( const tiny_vector<T,N,SP1>& lhs, const tiny_vector<T,N,SP2>& rhs ){
	return !(rhs < lhs);
}

template< class T, std::size_t N, class SP1, class SP2 >
constexpr bool operator>( const tiny_vector<T,N,SP1>& lhs, const tiny_vector<T,N,SP2>& rhs ){
	return rhs < lhs;
}

template< class T, std::size_t N, class SP1, class SP2 >
constexpr bool operator>=( const tiny_vector<T,N,SP1>& lhs, const tiny_vector<T,N,SP2>& rhs ){
	return !(lhs < rhs);
}

#else //CPP_20
template< class T, std::size_t N, class SP1, class SP2 >
constexpr std::weak_ordering operator<=>( const tiny_vector<T,N,SP1>& lhs, const tiny_vector<T,N,SP2>& rhs ){
	//TODO:
}
#endif	//CPP_20

END_APE_NAMESPACE

#endif //END APE_ESTL_TINY_VECTOR_H
