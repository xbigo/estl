#pragma once
#ifndef APE_ESTL_BUFFER_H
#define APE_ESTL_BUFFER_H
#include <ape/config.hpp>
#include <ape/estl/type_utility.hpp>
#include <ape/estl/backports/span.hpp>
//STL
#include <algorithm>
#include <memory>
#include <array>
#include <cstddef> // std::byte
#include <stdexcept>
#include <gsl/gsl_assert>

BEGIN_APE_NAMESPACE
    /// class buffer intend to be used locally, for performance sensitive scenario.

    template<typename T, typename Allocator>
    struct buffer_storage
    {
        using allocator_type = Allocator;
        using real_allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
        using pointer = typename real_allocator_type::pointer;

        struct buffer_imp_data{
            pointer start;
            pointer finish;
            pointer end_of_storage;
            constexpr buffer_imp_data() noexcept
            : start(), finish(), end_of_storage()
            {}
            constexpr buffer_imp_data(buffer_imp_data&& rhs) noexcept
            : start(rhs.start), finish(rhs.finish), end_of_storage(rhs.end_of_storage)
            {
                rhs.start = rhs.finish = rhs.end_of_storage = pointer();
            }
            constexpr buffer_imp_data(const buffer_imp_data&) noexcept = default;

            constexpr void swap(buffer_imp_data& rhs){
                /// gnuc++ doesn't use std::swap, and said that "(std::swap) it loses information used by TBAA"
                /// But I think, as a standard function, std::swap should not fool the compiler
                std::swap(rhs.start, start);
                std::swap(rhs.finish, finish);
                std::swap(rhs.end_of_storage, end_of_storage);
            }
            constexpr void reset_(){
                start = nullptr;
                finish = nullptr;
                end_of_storage = nullptr;
            }
        };
        struct buffer_imp : real_allocator_type, buffer_imp_data
        {
            constexpr buffer_imp() noexcept(noexcept(real_allocator_type())){}
            constexpr buffer_imp(const real_allocator_type& al) noexcept(noexcept(real_allocator_type(al)))
                : real_allocator_type(al){}
            constexpr buffer_imp(buffer_imp&& rhs) noexcept
                : real_allocator_type(std::move(rhs)), buffer_imp_data(std::move(rhs)){}
            constexpr explicit buffer_imp(real_allocator_type&& al) noexcept : real_allocator_type(std::move(al)){}
            constexpr buffer_imp(real_allocator_type&& al, buffer_imp&& rhs) noexcept
                : real_allocator_type(std::move(al)), buffer_imp_data(std::move(rhs)){}
        };
        [[nodiscard]] real_allocator_type& get_real_allocator_() noexcept{
            return this->m_imp;
        }
        [[nodiscard]] const real_allocator_type& get_real_allocator_() const noexcept{
            return this->m_imp;
        }
        [[nodiscard]] allocator_type get_allocator() const noexcept{
            return allocator_type(get_real_allocator_());
        }
        [[nodiscard]] pointer do_allocate_(std::size_t n){
            using tr = std::allocator_traits<real_allocator_type>;
            return n == 0? pointer() : tr::allocate(m_imp, n);
        }
        void do_deallocate_(pointer p, std::size_t n) noexcept {
            using tr = std::allocator_traits<real_allocator_type>;
            if (p)
                tr::deallocate(m_imp, p, n);
        }
        buffer_storage() = default;
        buffer_storage(buffer_storage&&) = default;
        buffer_storage(const allocator_type& al) : m_imp(al){}
        buffer_storage(std::size_t n, const allocator_type& al) : m_imp(al){
            create_storage_(n);
        }
        buffer_storage(real_allocator_type&& al, buffer_storage&& rhs) noexcept
                : m_imp(std::move(al), std::move(rhs.m_imp))
        {
        }

        ~buffer_storage(){
            do_deallocate_(m_imp.start, m_imp.end_of_storage - m_imp.start)
        }

        void create_storage_(size_t n){
            this->m_imp.start = this->do_allocate_(n);
            this->m_imp.finish = this->m_imp.start;
            this->m_imp.end_of_storage = this->m_imp.start + n;
        }
        buffer_imp m_imp;
    };
    /// this class is designed for performance, and the resize does not construct the elements.
    /// As a result, element type can only be of POD types.
    template<typename T, typename Allocator = std::allocator<T> >
    class buffer : public buffer_storage<T, Allocator>
    {
        static_assert(std::is_standard_layout_v<T> && std::is_trivial_v<T>, "T must be both trival and standard-layout");
        static_assert(std::is_same_v<remove_cv_t<T>, T>,
                      "ape::buffer must have a non-const, non-volatile value_type");
        static_assert(std::is_same_v<typename Allocator::value_type, T>,
                      "ape::buffer must have the same value_type as its allocator");

        using base_type = buffer_storage<T, Allocator>;
        using real_allocator_type = typename base_type::real_allocator_type;
        using real_allocator_traits_ = std::allocator_traits<real_allocator_type>;
    public:
        using value_type = T;                                                   ///< type of stored elements
        using allocator_type = Allocator;                               ///< type of allocator
        using size_type = typename real_allocator_type::size_type;                   ///< type of size
        using difference_type = typename allocareal_allocator_typetor_type::difference_type;       ///< type of difference size

        using reference = typename real_allocator_type::reference;                   ///< type of element reference
        using const_reference = typename real_allocator_type::const_reference;       ///< type of element const reference
        using pointer = typename real_allocator_type::pointer;                       ///< type of element pointer
        using const_pointer = typename real_allocator_type::const_pointer;           ///< type of element const reference

        using iterator = pointer;                                               ///< type of iterator
        using const_iterator = const_pointer ;                                   ///< type of const iterator
    private:
        // the 3rd parameter: true for is_always_equal allocator
        buffer(buffer&& rhs, const allocator_type& al, std::true_type) noexcept
        : base_type(al, std::move(rhs))
        { }

        buffer(buffer&& rhs, const allocator_type& al, std::false_type)
        : base_type(al)
        {
            if (rhs.get_allocator() == al)
                this->m_imp.swap(rhs.m_imp);
            else if (!rhs.empty())
            {
                this->create_storage_(rhs.size());
                this->m_imp.finish = std::copy(rhs.begin(), rhs.end(), this->m_imp.start);
                rhs.clear();
            }
        }
        [[nodiscard]] static size_type max_size_(const real_allocator_type &al) noexcept
        {
            const size_t diffmax = std::numeric_limits<std::ptrdiff_t>::max / sizeof(T);
            const size_t allocmax = real_allocator_traits_::max_size(al);
            return (std::min)(diffmax, allocmax);
        }
        [[nodiscard]] static size_type check_init_len_(size_type n, const allocator_type &al)
        {
            if (n > max_size_(real_allocator_type(al)))
                 throw std::length_error(
                    "cannot create ape::buffer larger than max_size()");
            return n;
        }

        template <typename Iterator>
        void range_initialize_(Iterator first, Iterator last)
        {
            if constexpr (is_forward_iterator_v<Iterator>){
                const size_type n = std::distance(first, last);
                this->m_imp.start = this->do_allocate(check_init_len_(n, this->get_real_allocator()));
                this->m_imp.end_of_storage = this->m_imp.start + n;
                this->m_imp.finish = std::copy(first, last,
                                                this->m_imp.start);
            }
            else if constexpr (is_input_iterator_v<Iterator>) {
                try
                {
                    for (; first != last; ++first)
                        emplace_back(*first);
                }
                catch(...)
                {
                    clear();
                    throw;
                }
            }
            else {
                static_assert(false, "parameter type is not iterator in function range_initialize_");
            }
        }
        void clear_reserve_(size_type n)
        {
            pointer start = this->do_allocate_(n);
            do_deallocate_(this->m_imp.start, capacity());

            this->m_imp.start = start;
            this->m_imp.finish = start;
            this->m_imp.end_of_storage = start + n;
        }

        [[nodiscard]] iterator to_mutable_(const_iterator pos) noexcept
        {
            return begin() + (pos - cbegin());
        }

        [[nodiscard]] size_type new_cap_(size_type n) const noexcept
        {
            return
                n < capacity()
                ? capacity()
                : (n < 2 * capacity())
                 ? 2 * capacity()
                 : n + n / 2;
        }
    public:

        /// default constructor
        /// \post empty()
        constexpr buffer() noexcept(noexcept(Allocator())) = default;

        /// construct an empty buffer object with the given allocator
        /// \post empty()
        constexpr explicit buffer(const Allocator& al) noexcept
            : base_type(al)
        {}

        constexpr buffer( size_type count,
                  const T& value,
                  const Allocator& al = Allocator())
            : base_type(check_init_len_(count, al), al)
        {
            this->m_imp.finish = std::fill_n(this->m_impl.start, n, value);
        }

        // construct a buffer with count of uninitialized elements
        constexpr explicit buffer( size_type count,
                           const Allocator& alloc = Allocator() )
            : base_type(check_init_len_(count, al), al)
        {
            this->m_imp.finish = this->m_impl.start + n;
        }

        template< class InputIt, class = std::enable_if_t<std::is_convertible_v<std::iterator_category_t<InputIt>,
                   std::input_iterator_tag> > >
        constexpr buffer( InputIt first, InputIt last,
                  const Allocator& alloc = Allocator() )
            : base_type(alloc)
        {
             range_initialize_(first, last);
        }


        constexpr buffer( const buffer& rhs, const Allocator& alloc )
            : base_type(rhs.size(), alloc)
        {
            this->m_imp.finish = std::copy(rhs.begin(), rhs.end(), this->m_imp.start);
        }
        constexpr buffer(const buffer& rhs)
            : buffer(rhs, real_allocator_traits_::select_on_container_copy_construction(rhs.get_real_allocator()))
        {
        }

        constexpr buffer(buffer&&) noexcept = default;
        constexpr buffer(buffer &&other, const Allocator &alloc) noexcept(typename real_allocator_traits::is_always_equal::value)
            : buffer(std::move(other), alloc, real_allocator_traits::is_always_equal{})
        {}

        constexpr buffer( std::initializer_list<T> init,
                  const Allocator& alloc = Allocator() )
            : base_type(alloc)
        {
            range_initialize_(init.begin(), init.end());
        }

        template<typename Range, typename=stl::enable_if_t<stl::is_container_v<Range>> >
        buffer(const Range& r, const Allocator& alloc)
            : base_type(alloc)
        {
            range_initialize_(std::begin(r), std::end(r));
        }
        /// ctor that contructs a buffer object fill with a copy a range of objects. If no heap is
        /// specified, the global heap would be utilized in the newly constructed object, otherwise, the heap specified is used
        /// \tparam Range the range type that specify a range of objects
        /// \param r the range of objects to copy from
        /// \param h the heap to be utilized in the newly constructed buffer object
        /// \post size() == r.size()
        template<typename Range, typename=stl::enable_if_t<stl::is_container_v<Range>> >
        buffer(const Range& r) : buffer(r, r.get_allocator())
        {}

        /// copy assignment operator
        buffer& operator=(const buffer& rhs)
        {
            if (this != &rhs){
                if constexpr (!real_allocator_traits_::is_always_equal::value
                    && real_allocator_traits_::propagate_on_container_copy_assignment::value)
                {
                    if (this->get_real_allocator() != rhs.get_real_allocator()){
                        // replacement allocator cannot free existing storage
                        do_deallocate_(this->m_imp.start, capacity());
                        this->m_imp.reset_();
                        this->get_real_allocator() = rhs.get_real_allocator();
                    }
                }

                APE_Expects(this->get_real_allocator() = rhs.get_real_allocator());
                if (rhs.size() > capacity())
                    clear_reserve_(rhs.size());
                this->m_imp.finish = std::copy(rhs.begin(), rhs.end(), this->m_imp.start);
            }
            return *this;
        }

        /// move assignment operator
        buffer& operator=(buffer&& rhs)
        {
            buffer(std::move(rhs)).swap(*this);
            return *this;
        }

        /// change the content of the buffer object to be count number of ch object
        /// \param count the number of object that the buffer object will have
        /// \param ch the object that the buffer object will contain
        /// \return reference to the buffer object itself
        buffer& assign(size_type count, const T& ch)
        {
            if (capacity() < count())
                clear_reserve_(count);
            this->m_imp.finish = std::fill_n(this->m_imp.start, count, ch);
            return *this;
        }

        /// change the content of the buffer object to be a copy of a range of objects
        /// \tparam Range the range type that specify a range of objects
        /// \param r the range of objects to copy from
        /// \return reference to the buffer object itself
        template<typename Range>
        stl::enable_if_t<stl::is_container<Range>::value, buffer&> assign(const Range& r)
        {
            if (capacity() < std::size(r))
                clear_reserve_(r.size());
            this->m_imp.finish = std::copy(std::begin(r), std::end(r), this->m_imp.start);
            return *this;
        }

        //memory

        /// get the capacity of the buffer object
        /// \return the capacity
        [[nodiscard]] size_type capacity() const noexcept    { return this->m_imp.end_of_storage - this->m_imp.start;}

        /// get the size of the buffer object
        /// \return the size
        [[nodiscard]] size_type size() const noexcept { return this->m_imp.finish - this->m_imp.start;}

        /// reserve space for storing exactly n objects. This method does nothing if the capacity is already
        /// large enough for n object. Otherwise, space for storing n objects will be allocated, old objects
        /// will be copied to the newly allocated space and the old space will be freed.
        /// \param n number of objects to allocate space for
        void reserve(size_type n)
        {
            if (n > capacity())
            {
                pointer start = this->do_allocate_(n);
                pointer finish = std::copy(begin(), end(), start);
                do_deallocate_(this->m_imp.start, capacity());

                this->m_imp.start = start;
                this->m_imp.finish = finish;
                this->m_imp.end_of_storage = start + n;
            }
        }

        /// resize the buffer to store n objects. It will reserve space large enough for storing n objects.
        /// This method does nothing if the capacity is already large enough for n object.
        /// Otherwise, space large enough for storing n objects will be allocated, old objects
        /// will be copied to the newly allocated space and old space will be freed.
        /// \param n new size of the buffer object
        /// \note if resize() extended the buffer, the new elements will not be initialized.
        void resize(size_type n)
        {
            reserve(n);
            this->m_imp.finish = this->m_imp.start + n;
        }

        /// resize the buffer to store n objects. If the new size if larger then the original, the increased
        /// space will be filled with the specified object ch. Otherwise, it does nothing.
        /// \param n new size of buffer
        /// \param ch the object to fill the extra space, if any
        void resize(size_type n, T ch)
        {
            if (n > capacity())
                reserve(n);

            if (size() < n)
                this->m_imp.finish = std::fill(this->m_imp.finish, n - size(), ch);
            else
                this->m_imp.finish = this->m_imp.start + n;
        }


        //insert

        /// insert a range of objects at the specified position
        /// \tparam Range the range type that specify a range of objects
        /// \param pos the position in the buffer to insert the new objects
        /// \param r the range of objects to copy from
        /// \pre pos is between [begin(), end()]
        /// \return reference to the buffer object itself
        template<typename Range>
        stl::enable_if_t<stl::is_container<Range>::value, buffer&> insert(const_iterator pos, const Range& r)
        {
            APE_Expects(pos >= begin() && pos < end());
            auto n = std::size(r);

            if (size() + n > capacity())
            {
                size_type ncap = new_cap_(size() + n);
                pointer start = this->do_allocate_(ncap);
                pointer finish = std::copy(cbegin(), pos, start);
                finish = std::copy(std::begin(r), std::end(r), finish);
                finish = std::copy(pos, cend(), finish);

                do_deallocate_(this->m_imp.start, capacity());
                this->m_imp.start = start;
                this->m_imp.finish = finish;
                this->m_imp.end_of_storage = start + ncap;
            }
            else
            {
                std::copy_backward(pos, cend(),  end() + n);
                this->m_imp.finish += =n;
                std::copy(std::begin(r), std::end(r),  to_mutable_(pos));
            }
            return *this;
        }

        /// insert specified number of a particular object at a specific position
        /// \param pos the position in the buffer to insert the new objects
        /// \param n number of objects to insert
        /// \param ch the specific object to insert
        /// \return reference to the buffer object itself
        buffer& insert(const_iterator pos, size_type n, T ch)
        {
            APE_Expects(pos >= begin() && pos < end());
            if (size() + n > capacity())
            {
                size_type ncap = new_cap_(size() + n );

                pointer start = this->do_allocate_(ncap);
                pointer finish = std::copy(begin(), pos, start);
                finish = std::fill_n(pos, n, ch);
                finish = std::copy(pos, cend(), finish);

                do_deallocate_(this->m_imp.start, capacity());
                this->m_imp.start = start;
                this->m_imp.finish = finish;
                this->m_imp.end_of_storage = start + ncap;
            }
            else
            {
                std::copy_backward(pos, cend(),  end() + n);
                this->m_imp.finish += =n;
                std::fill_n(to_mutable_(pos), n, ch);
            }
            return *this;
        }

        //append

        /// append count number of object ch at the end of the buffer object
        /// \param count number of objects to insert
        /// \param ch the specific object to insert
        /// \return reference to the buffer object itself
        buffer& append(size_type count, T ch)
        {
            return insert(end(), count, ch);
        }

        /// append a range of objects at the end of the buffer object
        /// \tparam Range the range type that specify a range of objects
        /// \param r the range of objects to insert
        /// \return reference to the buffer object itself
        template<typename Range>
        stl::enable_if_t<stl::is_container<Range>::value, buffer&> append(const Range& r)
        {
            return insert(end(), r);
        }

        /// append a single object to the end of the buffer object
        /// \param ch the object to append
        /// \return reference to the buffer object itself
        buffer& push_back(T ch)
        {
            if (size() < capacity()) {
                *this->m_imp.finish++ = ch;
            }
            else
                append(1, ch);
            return *this;
        }

        //erase

        /// remove the object referenced by p from the buffer object
        /// \pre p >= begin() && p < end()
        /// \param p the iterator that points to the object to remove
        /// \return the iterator that point to the original position inside the buffer
        iterator erase(const_iterator p) noexcept
        {
            APE_Expects(p >= begin() && p < end());
            this->m_imp.finish = std::copy(p + 1, end(), to_mutable_(p));
            return to_mutable_(p);
        }

        /// remove a range of objects from the buffer object
        /// \pre the specified range must be within the allocated space of the buffer
        /// \param first the begin of the range to be erased
        /// \param last the end of the range to erased
        /// \return the beginning position of the range of objects to remove
        iterator erase(const_iterator first, const_iterator last) noexcept
        {
            APE_Expects(first <= last);
            APE_Expects(first >= begin() && last <= end());

            std::copy(last, cend(), to_mutable_(first));
            this->m_imp.finish -= last - first;

            return to_mutable_(first);
        }

        /// replace one range of objects by another range of objects
        /// \tparam Range the range type that specify a range of objects
        /// \param first the begin of the range to be replaced
        /// \param last the end of the range to replaced
        /// \pre [first, last) belongs to this buffer
        /// \return reference to the buffer object itself
        template<typename Range>
        stl::enable_if_t<stl::is_container<Range>::value, buffer&> replace(
                const_iterator first, const_iterator last,
                const Range& f)
        {
            APE_Expects(first <= last);
            APE_Expects(first >= begin() && last <= end());

            difference_type len1 = std::distance(first, last);
            difference_type len2 = std::distance(f.begin(), f.end());
            if (size() - len1 + len2 < capacity())
            {
                if (len1 < len2) //insert
                    std::copy_backward(last, cend(), end() - len1 + len2);
                else if (len1 > len2)
                    std::copy(last, cend(), end() - len1 + len2);
                std::copy(f.begin(), f.end(), to_mutable_(first));
                this->m_imp.finish += len2 - len1;
            }
            else //insert
            {
                size_type ncap = new_cap_(size() - len1 + len2);
                pointer start = this->do_allocate_(ncap);
                pointer finish = std::copy(begin(), first, start);
                finish = std::copy(f.begin(), f.end(), finish);
                finish = std::copy(last, cend(), finish);

                do_deallocate_(this->m_imp.start, capacity());
                this->m_imp.start = start;
                this->m_imp.finish = finish;
                this->m_imp.end_of_storage = start + ncap;
            }
             return *this;
        }

        /// reduce buffer to a sub range of objects
        /// \param r the sub range of objects to reduce to
        /// \return reference to the buffer object itself
        buffer& sub(const_iterator first, const_iterator last) noexcept
        {
            APE_Expects(first <= last);
            APE_Expects(first >= begin() && last <= end());

            if (!empty())
                this->m_imp.finish = std::copy(r.begin(), r.end(), this->m_imp.start);

            return *this;
        }

        //methods

        /// clear all the content of the buffer
        void clear() noexcept
        {
            this->m_imp.finish = this->m_imp.start;
        }

        /// swap the content of two buffers
        void swap(buffer& rhs) noexcept
        {
            std::swap(this->m_imp, rhs.m_imp);
        }


        //queries

        /// get capacity
        [[nodiscard]] constexpr size_type capacity() const noexcept {
            return this->m_imp.end_of_storage - this->m_imp.start;
        }

        /// get size
        [[nodiscard]] constexpr size_type size() const noexcept {
            return this->m_imp.finish - this->m_imp.start;
        }

        /// get size
        [[nodiscard]] constexpr size_type size_bytes() const noexcept {
            return sizeof(T) * size();
        }

        /// check whether the buffer is empty
        /// \return if or not the buffer is empty
        [[nodiscard]] constexpr bool empty() const noexcept
        {
            return this->m_imp.finish == this->m_imp.start;
        }

        //iterator

        /// \return iterator to the beginning position of buffer storage
        [[nodiscard]] constexpr iterator begin() noexcept
        {
            return this->m_imp.start;
        }

        /// \return iterator to the end position of buffer storage
        [[nodiscard]] constexpr iterator end() noexcept
        {
            return this->m_imp.finish;
        }

        /// \return const iterator to the beginning position of buffer storage
        [[nodiscard]] constexpr const_iterator begin() const noexcept
        {
            return this->m_imp.start;
        }

        /// \return const iterator to the end position of buffer storage
        [[nodiscard]] constexpr const_iterator end() const noexcept
        {
            return this->m_imp.finish;
        }

        /// \return const iterator to the beginning position of buffer storage
        [[nodiscard]] constexpr const_iterator cbegin() const noexcept
        {
            return this->m_imp.start;
        }

        /// \return const iterator to the end position of buffer storage
        [[nodiscard]] constexpr const_iterator cend() const noexcept
        {
            return this->m_imp.finish;
        }

        //content access

        /// \return the pointer to the buffer storage
        [[nodiscard]] constexpr pointer data() noexcept
        {
            return this->m_imp.start;
        }

        //content access

        /// \return the const pointer to the buffer storage
        /// \note it returns nullptr if capacity() == 0;
        [[nodiscard]] constexpr const_pointer data() const noexcept
        {
            return this->m_imp.start;
        }

        /// access object in a specific position within a buffer
        /// \param n the index of the object to access
        /// \return reference to the requested object
        [[nodiscard]] constexpr reference operator[](size_type n) noexcept
        {
            APE_Expects(n < size());
            return this->m_imp.start[n];
        }

        /// access object in a specific position within a const buffer
        /// \param n the index of the object to access
        /// \return const reference to the requested object
        [[nodiscard]] constexpr const_reference operator[](size_type n) const noexcept
        {
            APE_Expects(n < size());
            return this->m_imp.start[n];
        }

        /// check if two buffer objects are the same. Two buffers are considered same if
        /// they contain the same number of objects and each object is identical
        /// \param rhs buffer object to compare with
        /// \return whether two buffer objects are same or not
        [[nodiscard]] constexpr bool operator == (const buffer<T, Allocator>& rhs) const noexcept {
            return size() == rhs.size()
                && std::equal(cbegin(), cend(), rhs.cbegin());
        }

        /// check if two buffer objects are different
        /// \see operator==(const buffer<T>& rhs)
        /// \param rhs buffer object to compare with
        [[nodiscard]] constexpr bool operator != (const buffer<T>& rhs) const noexcept {
            return !(*this == rhs);
        }

        /// operator <
        /// \param rhs buffer object to compare with
        /// \return whether the lhs buffer is lexicographically less than the rhs buffer
        [[nodiscard]] constexpr bool operator < (const buffer<T>& rhs) const noexcept {
            return std::lexicographical_compare(cbegin(), cend(), rhs.cbegin(), rhs.cend());
        }

        /// operator <=
        /// \param rhs buffer object to compare with
        /// \return whether the lhs buffer is lexicographically less than or equal to the rhs buffer
        [[nodiscard]] constexpr bool operator <= (const buffer<T>& rhs) const noexcept {
            return !(*this > rhs);
        }

        /// operator >
        /// \param rhs buffer object to compare with
        /// \return whether the lhs buffer is lexicographically greater than the rhs buffer
        [[nodiscard]] constexpr bool operator > (const buffer<T>& rhs) const noexcept {
            return rhs < *this;
        }

        /// operator >=
        /// \param rhs buffer object to compare with
        /// \return whether the lhs buffer is lexicographically greater than or equal to the rhs buffer
        [[nodiscard]] constexpr bool operator >= (const buffer<T>& rhs) const noexcept {
            return !(*this < rhs);
        }

    };

    using byte_buffer =  buffer<std::byte>;

END_APE_NAMESPACE

#endif //end APE_ESTL_BUFFER_H
