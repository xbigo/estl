#pragma once
#ifndef APE_ESTL_IO_H
#define APE_ESTL_IO_H
#include <ape/config.hpp>
#include <ape/estl/backports/span.hpp>
#include <ape/estl/error_code.hpp>
#include <any>
#include <cstddef>

BEGIN_APE_NAMESPACE
namespace io
{
    using mutable_buffer = std::span<std::byte>;
    using const_buffer = std::span<const std::byte>;

    inline constexpr long_size_t unknown_size = long_size_t(-1);
    inline constexpr bool is_valid_range(long_offset_range h) noexcept
    {
        return (h.begin >= 0) && (h.end == unknown_size || (h.end >= 0 && h.begin <= h.end));
    }
    inline constexpr bool is_valid_range(long_offset_range h, long_size_t fsize) noexcept
    {
        return is_valid_range(h) && (h.end == unknown_size || (long_size_t(h.end) <= fsize));
    }
    inline constexpr bool in_size_t_range(long_size_t n) noexcept
    {
        return (n & ~long_size_t(size_t(-1))) == 0;
    }
    inline constexpr bool in_ptrdiff_range(long_offset_t n) noexcept
    {
        std::ptrdiff_t v(static_cast<ptrdiff_t>(n));
        return v == n;
    }
    inline constexpr std::size_t narrow_cast(long_size_t n) noexcept
    {
        APE_Expects(in_size_t_range(n));
        return std::size_t(n);
    }
    inline constexpr std::ptrdiff_t narrow_cast(long_offset_t n) noexcept
    {
        APE_Expects(in_ptrdiff_range(n));
        return std::ptrdiff_t(n);
    }

    // read && reader
    template <typename Device>
        requires requires(Device &device, mutable_buffer buf, error_code_ptr err) {
            {
                device.read(buf, err)
            } -> std::convertible_to<mutable_buffer>;
        }
    decltype(auto) read(Device &device, mutable_buffer buf, error_code_ptr err = {})
    {
        return device.read(buf, err);
    }
    // return: read in data
    template <typename Device>
    concept reader = requires(Device &device, mutable_buffer buf, error_code_ptr err) {
        {
            read(device, buf, err)
        } -> std::convertible_to<mutable_buffer>;
        {
            read(device, buf)
        } -> std::convertible_to<mutable_buffer>;
    };

    // write && writer
    template <typename Device>
        requires requires(Device &device, const_buffer buf, error_code_ptr err) {
            {
                device.write(buf, err)
            } -> std::convertible_to<const_buffer>;
        }
    decltype(auto) write(Device &device, const_buffer buf, error_code_ptr err = {})
    {
        return device.write(buf, err);
    }
    // return: not written data
    template <typename Device>
    concept writer = requires(Device &device, const_buffer buf, error_code_ptr err) {
        {
            write(device, buf, err)
        } -> std::convertible_to<const_buffer>;
        {
            write(device, buf)
        } -> std::convertible_to<const_buffer>;
    };

    // sync && syncer
    template <typename Device>
        requires requires(Device &device, error_code_ptr err) {
            device.sync(err);
        }
    decltype(auto) sync(Device &device, error_code_ptr err = {})
    {
        return device.sync(err);
    }
    template <typename Device>
    concept syncer = requires(Device &device, error_code_ptr err) {
        sync(device, err);
        sync(device);
    };

    // truncate && truncater
    template <typename Device>
        requires requires(Device &device, long_size_t size, error_code_ptr err) {
            {
                device.truncate(size, err)
            } -> std::convertible_to<long_size_t>;
        }
    decltype(auto) truncate(Device &device, long_size_t size, error_code_ptr err = {})
    {
        return device.truncate(size, err);
    }
    template <typename Device>
    concept truncater = requires(Device &device, long_size_t size, error_code_ptr err) {
        {
            truncate(device, size, err)
        } -> std::convertible_to<long_size_t>;
        {
            truncate(device, size)
        } -> std::convertible_to<long_size_t>;
    };

    // size && sizer
    template <typename Device>
        requires requires(Device &device, error_code_ptr err) {
            {
                device.size(err)
            } -> std::convertible_to<long_size_t>;
        }
    decltype(auto) size(Device &device, error_code_ptr err = {})
    {
        return device.size(err);
    }
    template <typename Device>
    concept sizer = requires(Device &device, error_code_ptr err) {
        {
            size(device, err)
        } -> std::convertible_to<long_size_t>;
        {
            size(device)
        } -> std::convertible_to<long_size_t>;
    };

    // offset && sequence
    template <typename Device>
        requires requires(Device &device, error_code_ptr err) {
            {
                device.offset(err)
            } -> std::convertible_to<long_size_t>;
        }
    decltype(auto) offset(Device &device, error_code_ptr err = {})
    {
        return device.offset(err);
    }
    template <typename Device>
    concept sequence = requires(Device &device, error_code_ptr err) {
        {
            offset(device, err)
        } -> std::convertible_to<long_size_t>;
        {
            offset(device)
        } -> std::convertible_to<long_size_t>;
    };

    // seek && random
    template <typename Device>
        requires requires(Device &device, long_size_t off, error_code_ptr err) {
            {
                device.seek(off, err)
            } -> std::convertible_to<long_size_t>;
        }
    decltype(auto) seek(Device &device, long_size_t off, error_code_ptr err = {})
    {
        return device.seek(off, err);
    }
    template <typename Device>
    concept random = requires(Device &device, long_size_t off, error_code_ptr err) {
        {
            seek(device, off, err)
        } -> std::convertible_to<long_size_t>;
        {
            seek(device, off)
        } -> std::convertible_to<long_size_t>;
    } && sequence<Device>;

    // seek_forward && forward
    template <random Device>
    decltype(auto) seek_forward(Device &device, long_size_t off, error_code_ptr err = {})
    {
        return seek(device, off, err);
    }
    template <typename Device>
        requires requires(Device &device, long_size_t off, error_code_ptr err) {
            {
                device.seek_forward(off, err)
            } -> std::convertible_to<long_size_t>;
        }
    decltype(auto) seek_forward(Device &device, long_size_t off, error_code_ptr err = {})
    {
        return device.seek_forward(off, err);
    }
    template <typename Device>
    concept forward = requires(Device &device, long_size_t off, error_code_ptr err) {
        {
            seek_forward(device, off, err)
        } -> std::convertible_to<long_size_t>;
        {
            seek_forward(device, off)
        } -> std::convertible_to<long_size_t>;
    } && sequence<Device>;

    // read_view
    template <typename Device>
        requires requires(Device &device) {
            {
                device.address()
            } -> std::convertible_to<const_buffer>;
        }
    decltype(auto) address(Device &device)
    {
        return device.address();
    }
    template <typename Device>
        requires requires(Device &device) {
            {
                device->address()
            } -> std::convertible_to<const_buffer>;
        }
    decltype(auto) address(Device &device)
    {
        return device->address();
    }
    template <typename Device>
    concept read_view = requires(Device &device) {
        {
            address(device)
        } -> std::convertible_to<const_buffer>;
    } || requires(Device &device) {
        {
            address(*device)
        } -> std::convertible_to<const_buffer>;
    };

    // view_rd && read_map
    template <typename Device>
        requires requires(Device &device, long_offset_range rng, error_code_ptr err) {
            {
                device.view_rd(rng, err)
            } -> read_view;
        }
    decltype(auto) view_rd(Device &device, long_offset_range rng, error_code_ptr err = {})
    {
        return device.view_rd(rng, err);
    }
    template <typename Device>
    concept read_map = requires(Device &device, long_offset_range rng, error_code_ptr err) {
        {
            view_rd(device, rng, err)
        } -> read_view;
        {
            view_rd(device, rng)
        } -> read_view;
    } && sizer<Device>;

    // write_view
    template <typename Device>
        requires requires(Device &device) {
            {
                device.address()
            } -> std::convertible_to<mutable_buffer>;
        }
    decltype(auto) address(Device &device)
    {
        return device.address();
    }
    template <typename Device>
        requires requires(Device &device) {
            {
                device->address()
            } -> std::convertible_to<mutable_buffer>;
        }
    decltype(auto) address(Device &device)
    {
        return device->address();
    }
    template <typename Device>
    concept write_view = requires(Device &device) {
        {
            address(device)
        } -> std::convertible_to<mutable_buffer>;
    } || requires(Device &device) {
        {
            address(*device)
        } -> std::convertible_to<mutable_buffer>;
    };

    // view_wr && write_map
    template <typename Device>
        requires requires(Device &device, long_offset_range rng, error_code_ptr err) {
            {
                device.view_wr(rng, err)
            } -> write_view;
        }
    decltype(auto) view_wr(Device &device, long_offset_range rng, error_code_ptr err = {})
    {
        return device.view_wr(rng, err);
    }
    template <typename Device>
    concept write_map = requires(Device &device, long_offset_range rng, error_code_ptr err) {
        {
            view_wr(device, rng, err)
        } -> write_view;
        {
            view_wr(device, rng)
        } -> write_view;
    } && sizer<Device>;

    // options
    template <typename Device>
        requires requires(Device &device, int id, const std::any &optdata, error_code_ptr err) {
            {
                device.getopt(id, optdata, err)
            } -> std::convertible_to<std::any>;
        }
    decltype(auto) getopt(Device &device, int id, const std::any &optdata, error_code_ptr err = {})
    {
        return device.getopt(id, optdata, err);
    }
    template <typename Device>
        requires requires(Device &device, int id, const std::any &optdata, error_code_ptr err) {
            {
                device.setopt(id, optdata, optdata, err)
            } -> std::convertible_to<std::any>;
        }
    decltype(auto) setopt(Device &device, int id, const std::any &optdata,
                          const std::any &indata, error_code_ptr err = {})
    {
        return device.setopt(id, optdata, indata, err);
    }
    template <typename Device>
    concept options = requires(Device &device, int id, const std::any &optdata, error_code_ptr err) {
        {
            getopt(device, id, optdata, err)
        } -> std::convertible_to<std::any>;
        {
            getopt(device, id, optdata)
        } -> std::convertible_to<std::any>;
        {
            setopt(device, id, optdata, optdata, err)
        } -> std::convertible_to<std::any>;
        {
            setopt(device, id, optdata, optdata)
        } -> std::convertible_to<std::any>;
    };

} // end namespace io;

END_APE_NAMESPACE

#endif // end  APE_ESTL_IO_H