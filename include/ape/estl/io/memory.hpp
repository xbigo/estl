#pragma once
#ifndef APE_ESTL_IO_MEMORY_H
#define APE_ESTL_IO_MEMORY_H
#include <ape/estl/io/iocore.hpp>
#include <type_traits>
#include <algorithm>
#include <vector>

BEGIN_APE_NAMESPACE
namespace io
{
    class buffer_rd_view
    {
    public:
        constexpr const_buffer address() const noexcept { return m_data; }
        constexpr buffer_rd_view() noexcept = default;
        constexpr explicit buffer_rd_view(const_buffer data) noexcept : m_data(data) {}

    private:
        const_buffer m_data;
    };

    class buffer_wr_view
    {
    public:
        constexpr mutable_buffer address() const noexcept { return m_data; }
        constexpr buffer_wr_view() noexcept = default;
        constexpr explicit buffer_wr_view(mutable_buffer data) noexcept : m_data(data) {}

    private:
        mutable_buffer m_data;
    };

    template <typename Represent>
    concept read_represent_func = requires(const std::remove_cvref_t<Represent> &rep, std::size_t p) {
        {
            *rep.data().begin()
        } -> permissive_same_as<std::byte>;
        {
            *rep.data().end()
        } -> permissive_same_as<std::byte>;
        {
            rep.pos()
        } -> std::convertible_to<long_size_t>;
    } && requires(std::remove_cvref_t<Represent> &rep, std::size_t p) {
        rep.pos(p);
    };

    template <typename Represent>
    concept read_represent_data = requires(const std::remove_cvref_t<Represent> &rep) {
        {
            *rep.data.begin()
        } -> permissive_same_as<std::byte>;
        {
            *rep.data.end()
        } -> permissive_same_as<std::byte>;
        {
            rep.pos
        } -> std::convertible_to<long_size_t>;
    } && requires(std::remove_cvref_t<Represent> &rep, std::size_t p) {
        rep.pos = p;
    };

    template <typename Represent>
    concept read_represent = read_represent_data<Represent> || read_represent_func<Represent>;

    template <typename Represent>
    concept write_represent_func =
        requires(std::remove_cvref_t<Represent> &rep, std::size_t size) {
            *rep.data().begin() = std::byte{};
        } && read_represent<Represent>;

    template <typename Represent>
    concept write_represent_data =
        requires(std::remove_cvref_t<Represent> &rep, std::size_t size) {
            *rep.data.begin() = std::byte{};
        } && read_represent<Represent>;

    template <typename Represent>
    concept write_represent = write_represent_data<Represent> || write_represent_func<Represent>;

    template <typename Represent>
    concept truncatable_represent =
        requires(std::remove_cvref_t<Represent> &rep, std::size_t size) {
            rep.data().resize(size);
        } ||
        requires(std::remove_cvref_t<Represent> &rep, std::size_t size) {
            rep.data.resize(size);
        };

    template <read_represent_func Rep>
    decltype(auto) get_data_part(Rep &&rep) noexcept
    {
        return rep.data();
    }
    template <read_represent_data Rep>
    auto &&get_data_part(Rep &&rep) noexcept
    {
        return rep.data;
    }
    template <read_represent_func Rep>
    auto &&get_pos_part(Rep &&rep) noexcept
    {
        return rep.pos();
    }
    template <read_represent_data Rep>
    auto &&get_pos_part(Rep &&rep) noexcept
    {
        return rep.pos;
    }

    template <read_represent_func Rep>
    auto &&set_pos_part(Rep &rep, std::size_t p) noexcept
    {
        return rep.pos(p);
    }
    template <read_represent_data Rep>
    auto &&set_pos_part(Rep &rep, std::size_t p) noexcept
    {
        return rep.pos = p;
    }

    namespace impl
    {
        template <typename Represent>
        decltype(auto) get_pos_iter(Represent &rep) noexcept
        {
            return get_data_part(rep).begin() + get_pos_part(rep);
        }

        template <typename Represent>
        std::size_t get_size(const Represent &rep) noexcept
        {
            if constexpr (has_member_size<Represent>)
            {
                return get_data_part(rep).size();
            }
            else
            {
                return get_data_part(rep).end() - get_data_part(rep).begin();
            }
        }

        template <typename Represent>
        bool get_is_eof(const Represent &rep) noexcept
        {
            return get_size(rep) <= get_pos_part(rep);
        }

        template <typename Represent>
        std::size_t get_readable_size(const Represent &rep) noexcept
        {
            return get_is_eof(rep) ? 0 : (get_size(rep) - get_pos_part(rep));
        }

        template <read_represent Represent>
        mutable_buffer read(Represent &rep, mutable_buffer buf, error_code_ptr ec) noexcept
        {
            clear_error(ec);

            if (buf.empty() || get_is_eof(rep))
                return buf;

            auto n = std::min(get_readable_size(rep), buf.size());
            auto ditr = std::copy_n(get_pos_iter(rep), n, buf.begin());
            set_pos_part(rep, get_pos_part(rep) + n);

            return {buf.begin(), ditr};
        }

        template <read_represent Represent>
        long_size_t offset(const Represent &rep, error_code_ptr ec = {}) noexcept
        { // sequence
            clear_error(ec);
            return get_pos_part(rep);
        }

        template <read_represent Represent>
        bool is_eof(const Represent &rep, error_code_ptr ec = {}) noexcept
        { // is_eofer
            clear_error(ec);
            return get_is_eof(rep);
        }

        template <read_represent Represent>
        long_size_t seek(Represent &rep, long_size_t offset, error_code_ptr ec = {})
        { // random
            if (!in_size_t_range(offset))
            {
                set_error_or_throw<io_exception>(ec, std::errc::value_too_large);
                return get_pos_part(rep);
            }
            clear_error(ec);
            return set_pos_part(rep, narrow_cast(offset));
        }

        template <read_represent Represent>
        long_size_t size(const Represent &rep, error_code_ptr ec = {}) noexcept
        { // sizer
            clear_error(ec);
            return get_size(rep);
        }

        template <read_represent Represent>
        buffer_rd_view view_rd(const Represent &rep, long_offset_range h, error_code_ptr ec = {})
        { // read_map
            APE_Expects(is_valid_range(h));

            if (h.end != unknown_size && h.end > get_size(rep))
            {
                set_error_or_throw<io_exception>(ec, std::errc::invalid_argument);
                return buffer_rd_view();
            }

            clear_error(ec);

            if (h.end == unknown_size)
                h.end = get_size(rep);
            return buffer_rd_view({get_data_part(rep).begin() + narrow_cast(h.begin),
                                   get_data_part(rep).begin() + narrow_cast(h.end)});
        }

        template <write_represent Represent>
        long_size_t do_truncate(Represent &rep, long_size_t size, error_code_ptr err)
        {
            if constexpr (!truncatable_represent<Represent>)
            {
                set_error_or_throw<io_exception>(err, std::errc::function_not_supported);
                return {};
            }

            std::size_t new_size = narrow_cast(size);
            try
            {
                get_data_part(rep).resize(new_size);
            }
            catch (...)
            {
                set_error_or_throw<io_exception>(err, std::errc::not_enough_memory);
                return {};
            }
            return new_size;
        }

        template <truncatable_represent Represent>
        long_size_t truncate(Represent &rep, long_size_t new_size, error_code_ptr err = {})
        {
            if (!in_size_t_range(new_size))
            {
                set_error_or_throw<io_exception>(err, std::errc::value_too_large);
                return {};
            }
            return do_truncate(rep, new_size, err);
        }

        template <write_represent Represent>
        const_buffer write(Represent &rep, const_buffer buf, error_code_ptr err)
        { // write
            auto new_pos = get_pos_part(rep) + buf.size();
            if (get_size(rep) < new_pos)
            {
                do_truncate(rep, new_pos, err);
                if (has_error(err))
                    return {};
            }

            std::copy(buf.begin(), buf.end(), get_pos_iter(rep));

            set_pos_part(rep, new_pos);

            clear_error(err);
            return {buf.end(), buf.end()};
        }

        template <write_represent Represent>
        buffer_wr_view view_wr(Represent &rep, long_offset_range h, error_code_ptr err = {})
        { // write_map
            APE_Expects(is_valid_range(h));

            if (h.end == unknown_size)
                h.end = get_size(rep);

            if (!in_size_t_range(h.end))
            {
                set_error_or_throw<io_exception>(err, std::errc::value_too_large);
                return {};
            }

            if (h.end > get_size(rep))
            {
                do_truncate(rep, h.end, err);
                if (has_error(err))
                    return {};
            }

            clear_error(err);
            return buffer_wr_view({get_data_part(rep).begin() + narrow_cast(h.begin),
                                   get_data_part(rep).begin() + narrow_cast(h.end)});
        }

        template <write_represent Represent>
        void sync(Represent &rep, error_code_ptr err = {})
        {
            unused(rep);

            clear_error(err);
        }
    }

    struct const_buffer_represent
    {
        const_buffer &data;
        std::size_t &pos;
    };

    struct mutable_buffer_represent
    {
        mutable_buffer &data;
        std::size_t &pos;
    };

    struct vector_buffer_represent
    {
        std::vector<std::byte> data;
        std::size_t pos{0};
    };

    // imp [ sequence, forward, random ] [ is_eofer, sizer]
    template <typename Represent = vector_buffer_represent>
        requires read_represent<Represent> || write_represent<Represent> || truncatable_represent<Represent>
    struct memory_device
    {
        explicit memory_device(Represent rep = {}) noexcept(std::is_nothrow_move_constructible_v<Represent>)
            : m_rep{std::move(rep)} {}

        long_size_t offset(error_code_ptr ec = {}) const noexcept
        { // sequence
            return impl::offset(m_rep, ec);
        }

        bool is_eof(error_code_ptr ec = {}) const noexcept
        { // is_eofer
            return impl::is_eof(m_rep, ec);
        }

        long_size_t seek(long_size_t offset, error_code_ptr ec = {})
        { // random
            return impl::seek(m_rep, offset, ec);
        }

        long_size_t size(error_code_ptr ec = {}) const noexcept
        { // sizer
            return impl::size(m_rep, ec);
        }

        std::add_lvalue_reference_t<Represent> underlying() noexcept
        {
            return m_rep;
        }
        const std::add_lvalue_reference_t<Represent> underlying() const noexcept
        {
            return m_rep;
        }

        mutable_buffer read(mutable_buffer buf, error_code_ptr ec = {})
        { // reader
            return impl::read(this->m_rep, buf, ec);
        }

        buffer_rd_view view_rd(long_offset_range h, error_code_ptr ec = {})
        { // read_map
            return impl::view_rd(this->m_rep, h, ec);
        }

        const_buffer write(const_buffer buf, error_code_ptr ec = {})
        {
            return impl::write(m_rep, buf, ec);
        }

        buffer_wr_view view_wr(long_offset_range h, error_code_ptr ec = {})
        { // read_map
            return impl::view_wr(m_rep, h, ec);
        }

        void sync(error_code_ptr err = {})
        {
            impl::sync(m_rep, err);
        }

        long_size_t truncate(long_size_t size, error_code_ptr err = {})
        {
            return impl::truncate(m_rep, size, err);
        }

    protected:
        Represent m_rep;
    };

    struct pseudo_common
    {
        long_size_t offset(error_code_ptr ec = {}) const noexcept
        { // sequence
            clear_error(ec);
            return m_pos;
        }

        bool is_eof(error_code_ptr ec = {}) const noexcept
        { // is_eofer
            clear_error(ec);
            return false;
        }

        long_size_t seek(long_size_t offset, error_code_ptr ec = {})
        { // random
            clear_error(ec);
            return m_pos = offset;
        }

        long_size_t size(error_code_ptr ec = {}) const noexcept
        { // sizer
            clear_error(ec);
            return unknown_size;
        }

    protected:
        long_size_t m_pos = 0;
    };

    class zero : public pseudo_common
    {
    public:
        mutable_buffer read(mutable_buffer buf, error_code_ptr ec = {})
        { // reader
            clear_error(ec);
            std::fill(buf.begin(), buf.end(), std::byte{});
            return buf;
        }
    };

    class fill : public pseudo_common
    {
        const_buffer m_pattern;

    public:
        explicit fill(const_buffer pattern) : m_pattern(pattern)
        {
            APE_Expects(!pattern.empty());
        }

        mutable_buffer read(mutable_buffer buf, error_code_ptr ec = {})
        {
            clear_error(ec);

            auto tail_n = m_pos % m_pattern.size();
            auto ditr = std::copy_n(m_pattern.end() - tail_n, tail_n, buf.begin());

            auto pattern_n = (buf.size() - tail_n) / m_pattern.size();
            for (; pattern_n > 0; --pattern_n)
                ditr = std::copy(m_pattern.begin(), m_pattern.end(), ditr);

            auto head_n = (buf.size() - tail_n) % m_pattern.size();
            ditr = std::copy_n(m_pattern.begin(), head_n, ditr);

            m_pos += buf.size();
            return buf;
        }
    };

    struct null
    {
        const_buffer write(const_buffer r, error_code_ptr ec = {})
        {
            clear_error(ec);
            m_pos += r.size();
            return {r.end(), r.end()};
        }

        void sync(error_code_ptr ec = {}) { clear_error(ec); }

        long_size_t truncate(long_size_t size, error_code_ptr ec = {})
        {
            clear_error(ec);
            return m_pos = size;
        }

        long_size_t offset(error_code_ptr ec = {}) const
        { // sequence
            clear_error(ec);
            return m_pos;
        }

        long_size_t seek(long_size_t offset, error_code_ptr ec = {})
        { // random
            clear_error(ec);
            return m_pos = offset;
        }

        long_size_t size(error_code_ptr ec = {}) const
        { // sizer
            clear_error(ec);
            return 0;
        }

    private:
        long_size_t m_pos = 0;
    };

    struct empty
    {

        mutable_buffer read(mutable_buffer buf, error_code_ptr ec = {})
        {
            clear_error(ec);
            return {buf.begin(), buf.begin()};
        }

        long_size_t offset(error_code_ptr ec = {}) const
        { // sequence
            clear_error(ec);
            return 0;
        }

        long_size_t seek(long_size_t /*offset*/, error_code_ptr ec = {})
        { // random
            clear_error(ec);
            return 0;
        }

        long_size_t size(error_code_ptr ec = {}) const
        { // sizer
            clear_error(ec);
            return 0;
        }
    };
}

END_APE_NAMESPACE
#endif // end APE_ESTL_IO_MEMORY_H