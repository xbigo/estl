#pragma once
#ifndef APE_ESTL_IO_ADAPTOR_H
#define APE_ESTL_IO_ADAPTOR_H
#include <ape/estl/io/iocore.hpp>
#include <vector>

BEGIN_APE_NAMESPACE
namespace io
{
    template<typename MultiplexDevice>
    struct offset_tracker
    {
        MultiplexDevice *self;
        offset_tracker(MultiplexDevice *me) : self(me)
        {
            seek(self->m_device, self->pos);
        }
        ~offset_tracker()
        {
            self->pos = offset(self->m_device);
        }
    };

    template <random Device>
    class multiplex_device
    {
        Device &m_device;
        long_size_t pos{0};
        friend struct offset_tracker<multiplex_device<Device>>;
    public:
        multiplex_device(Device &d, long_size_t p = 0)
        : m_device(d), pos(p)
        {
        }

        long_size_t offset(error_code_ptr ec = {}) const noexcept
        {
            clear_error(ec);
            return pos;
        }

        bool is_eof(error_code_ptr ec = {}) const noexcept
        {
            offset_tracker tracker{this};
            return is_eof(m_device, ec);
        }

        long_size_t seek(long_size_t offset, error_code_ptr ec = {})
        {
            offset_tracker tracker{this};

            return seek(m_device, offset, ec);
        }

        long_size_t size(error_code_ptr ec = {}) const noexcept
        {
            return size(m_device, ec);
        }

        Device& underlying() noexcept
        {
            return m_device;
        }
        const Device& underlying() const noexcept
        {
            return m_device;
        }

        mutable_buffer read(mutable_buffer buf, error_code_ptr ec = {})
        {
            offset_tracker tracker{this};

            return read(m_device, buf, ec);
        }

        auto view_rd(long_offset_range h, error_code_ptr ec = {})
        {
            return view_rd(m_device, h, ec);
        }

        const_buffer write(const_buffer buf, error_code_ptr ec = {})
        {
            offset_tracker tracker{this};

            return write(m_device, buf, ec);
        }

        auto view_wr(long_offset_range h, error_code_ptr ec = {})
        {
            return view_wr(m_device, h, ec);
        }

        void sync(error_code_ptr err = {})
        {
            sync(m_device, err);
        }

        long_size_t truncate(long_size_t size, error_code_ptr err = {})
        {
            offset_tracker tracker{this};

            return truncate(m_device, size, err);
        }
    };

    template <forward Device>
    class shift_device
    {
        Device &m_device;
        long_size_t m_shift{0};

    public:

        shift_device(Device &d, long_size_t p = 0) : m_device(d), m_shift(p)
        {
        }

        long_size_t offset(error_code_ptr ec = {}) const noexcept
        {
            clear_error(ec);
            auto off = offset(m_device, ec);
            return off < m_shift ? 0 : (off - m_shift);
        }

        bool is_eof(error_code_ptr ec = {}) const noexcept
        {
            return is_eof(m_device, ec);
        }

        long_size_t seek(long_size_t offset, error_code_ptr ec = {})
        {
            seek(m_device, offset + m_shift, ec) - m_shift;
        }

        long_size_t size(error_code_ptr ec = {}) const noexcept
        {
            auto s = size(m_device, ec);
            return s < m_shift ? 0 : (s - m_shift);
        }

        Device& underlying() noexcept
        {
            return m_device;
        }
        const Device& underlying() const noexcept
        {
            return m_device;
        }

        long_size_t shift() const noexcept {
            return m_shift;
        }

        mutable_buffer read(mutable_buffer buf, error_code_ptr ec = {})
        {
            if (offset(m_device, ec) < m_shift)
                seek_forward(m_device, m_shift, ec);
            return read(m_device, buf, ec);
        }

        auto view_rd(long_offset_range h, error_code_ptr ec = {})
        {
            return view_rd(m_device, {h.begin + m_shift, h.end + m_shift}, ec);
        }

        const_buffer write(const_buffer buf, error_code_ptr ec = {})
        {
            if (offset(m_device, ec) < m_shift)
                seek_forward(m_device, m_shift, ec);
            return write(m_device, buf, ec);
        }

        auto view_wr(long_offset_range h, error_code_ptr ec = {})
        {
            long_offset_range shift_h{h.begin + m_shift, h.end + m_shift};
            return view_wr(m_device, shift_h, ec);
        }

        void sync(error_code_ptr err = {})
        {
            sync(m_device, err);
        }

        long_size_t truncate(long_size_t size, error_code_ptr err = {})
        {
            return truncate(m_device, size + m_shift, err) - m_shift;
        }
    };

    template <random Device>
    class sub_device
    {
        Device &m_device;
        long_offset_range m_section{};

    public:
        sub_device(Device &d, long_offset_range s = {0, unknown_offset}) : m_device(d), m_section(s)
        {
            APE_Expects(m_section.begin <= m_section.end);
        }

        long_size_t offset(error_code_ptr ec = {}) const noexcept
        {
            clear_error(ec);
            auto off = std::clamp(offset(m_device, ec), m_section.begin, m_section.end);
            return off - m_section.begin;;
        }

        bool is_eof(error_code_ptr ec = {}) const noexcept
        {
            return offset(ec) == size(m_section);
        }

        long_size_t seek(long_size_t offset, error_code_ptr ec = {})
        {
            seek(m_device, offset + m_section.begin, ec) - m_section.begin;
        }

        long_size_t size(error_code_ptr ec = {}) const noexcept
        {
            clear_error(ec);
            return size(m_section);
        }

        Device& underlying() noexcept
        {
            return m_device;
        }
        const Device& underlying() const noexcept
        {
            return m_device;
        }

        long_offset_range section() const noexcept
        {
            return m_section;
        }

        mutable_buffer read(mutable_buffer buf, error_code_ptr ec = {})
        {
            auto off = offset(m_device, ec);
            if (off < m_section.begin)
                seek_forward(m_device, m_section.begin, ec);
            if (m_section.end < off)
                return buf.first(0);

            auto avaliable_size = m_section.end - off;
            auto buf_size = std::min(avaliable_size, buf.size());
            return read(m_device, buf.first(buf_size), ec);
        }

        auto view_rd(long_offset_range h, error_code_ptr ec = {})
        {
            auto first = h.begin + m_section.begin;
            auto last = h.end + m_section.begin;
            return view_rd(m_device, {first, std::min(last, m_section.end)}, ec);
        }

        const_buffer write(const_buffer buf, error_code_ptr ec = {})
        {
            auto off = offset(m_device, ec);
            if (off < m_section.begin)
                seek_forward(m_device, m_section.begin, ec);
            if (m_section.end < off)
                return buf;

            auto avaliable_size = m_section.end - off;
            auto buf_size = std::min(avaliable_size, buf.size());
            auto res = write(m_device, buf.first(buf_size), ec);
            return {res.begin(), buf.end()};
        }

        auto view_wr(long_offset_range h, error_code_ptr ec = {})
        {
            auto first = h.begin + m_section.begin;
            auto last = h.end + m_section.begin;
            return view_wr(m_device, {first, std::min(last, m_section.end)}, ec);
        }

        void sync(error_code_ptr err = {})
        {
            sync(m_device, err);
        }

        long_size_t truncate(long_size_t size_, error_code_ptr err = {})
        {
            if (size_ != size(m_section)){
                set_error_or_throw<io_exception>(err, std::errc::invalid_argument);
                return {};
            }
            clear_error(err);
            return size(m_section) ;
        }
    };

    struct cache_rd_view
    {
        constexpr const_buffer address() const noexcept { return data; }
        std::vector<std::byte> data;
    };

    template<reader Device>
    class reader_to_view
    {
        Device &m_device;
        public:
        explicit reader_to_view(Device& d) noexcept : m_device(d){}

        cache_rd_view view_rd(long_offset_range rng, error_code_ptr err = {}){
            if (!in_size_t_range(size(rng)))
            {
                set_error_or_throw<io_exception>(err, std::errc::value_too_large);
                return {};
            }

            cache_rd_view res;
            res.data.resize(size(rng));
            res.data.resize(read(m_device, mutable_buffer(res.data), err).size());
            return res;
        }

    };

    template<writer Device>
    requires random<Device>
    class cache_wr_view
    {
        Device &m_device;
        std::vector<std::byte> data;
        long_offset_t pos;
    public:
        cache_wr_view(Device&d, std::vector<std::byte> data, long_offset_t p)
            : m_device(d)
            , data(std::move(data))
            , pos(p)
        {
        }

        constexpr mutable_buffer address() noexcept
        {
            return data;
        }
        ~cache_wr_view() {
            seek(m_device, pos);
            write(m_device, data);
        }
    };

    template<writer Device>
    class writer_to_view
    {
        Device &m_device;
        public:
        explicit writer_to_view(Device& d) noexcept : m_device(d){}

        cache_rd_view view_wr(long_offset_range rng, error_code_ptr err = {}){
            if (!in_size_t_range(size(rng)))
            {
                set_error_or_throw<io_exception>(err, std::errc::value_too_large);
                return {};
            }

            cache_wr_view res(m_device, {}, rng.begin);
            res.data.resize(size(rng));
            return res;
        }

    };


}

END_APE_NAMESPACE
#endif // end APE_ESTL_IO_ADAPTOR_H