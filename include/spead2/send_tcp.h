/*
 * TCP sender for SPEAD protocol
 *
 * ICRAR - International Centre for Radio Astronomy Research
 * (c) UWA - The University of Western Australia, 2018
 * Copyright by UWA (in the framework of the ICRAR)
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SPEAD2_SEND_TCP_H
#define SPEAD2_SEND_TCP_H

#include <boost/asio.hpp>
#include <vector>
#include <utility>
#include <initializer_list>
#include <spead2/send_packet.h>
#include <spead2/send_stream.h>
#include <spead2/common_endian.h>
#include <spead2/common_socket.h>

namespace spead2
{
namespace send
{

namespace detail
{

boost::asio::ip::tcp::socket make_socket(
    const io_service_ref &io_service,
    const std::vector<boost::asio::ip::tcp::endpoint> &endpoints,
    std::size_t buffer_size,
    const boost::asio::ip::address &interface_address);

} // namespace detail

class tcp_stream : public stream_impl<tcp_stream>
{
private:
    friend class stream_impl<tcp_stream>;

    /// The underlying TCP socket
    boost::asio::ip::tcp::socket socket;
    /// Whether the underlying socket is already connected or not
    std::atomic<bool> connected{false};

    void async_send_packets();

public:
    /// Socket send buffer size, if none is explicitly passed to the constructor
    static constexpr std::size_t default_buffer_size = 208 * 1024;

    /**
     * Constructor. A callback is provided to indicate when the connection is
     * established.
     *
     * @warning The callback may be called before the constructor returns. The
     * implementation of the callback needs to be prepared to handle this case.
     *
     * @param io_service   I/O service for sending data
     * @param connect_handler  Callback when connection is established. It is called
     *                     with a @c boost::system::error_code to indicate whether
     *                     connection was successful.
     * @param endpoints    Destination host and port (must contain exactly one element)
     * @param config       Stream configuration
     * @param buffer_size  Socket buffer size (0 for OS default)
     * @param interface_address   Source address
     *                            @verbatim embed:rst:leading-asterisks
     *                            (see tips on :ref:`routing`)
     *                            @endverbatim
     */
    template<typename ConnectHandler>
    tcp_stream(
        io_service_ref io_service,
        ConnectHandler &&connect_handler,
        const std::vector<boost::asio::ip::tcp::endpoint> &endpoints,
        const stream_config &config = stream_config(),
        std::size_t buffer_size = default_buffer_size,
        const boost::asio::ip::address &interface_address = boost::asio::ip::address())
        : stream_impl(std::move(io_service), config, 1),
        socket(detail::make_socket(get_io_service(), endpoints, buffer_size, interface_address))
    {
        socket.async_connect(endpoints[0],
            [this, connect_handler] (const boost::system::error_code &ec)
            {
                if (!ec)
                    connected.store(true);
                connect_handler(ec);
            });
    }

    /**
     * Backwards-compatibility constructor.
     */
    template<typename ConnectHandler>
    SPEAD2_DEPRECATED("use a vector of endpoints")
    tcp_stream(
        io_service_ref io_service,
        ConnectHandler &&connect_handler,
        const boost::asio::ip::tcp::endpoint &endpoint,
        const stream_config &config = stream_config(),
        std::size_t buffer_size = default_buffer_size,
        const boost::asio::ip::address &interface_address = boost::asio::ip::address())
        : tcp_stream(
            std::move(io_service),
            std::forward<ConnectHandler>(connect_handler),
            std::vector<boost::asio::ip::tcp::endpoint>{endpoint},
            config, buffer_size, interface_address)
    {
    }

    /* Force an initializer list to forward to the vector version (without this,
     * a singleton initializer list forwards to the scalar version).
     */
    template<typename ConnectHandler>
    tcp_stream(
        io_service_ref io_service,
        ConnectHandler &&connect_handler,
        std::initializer_list<boost::asio::ip::tcp::endpoint> endpoints,
        const stream_config &config = stream_config(),
        std::size_t buffer_size = default_buffer_size,
        const boost::asio::ip::address &interface_address = boost::asio::ip::address())
        : tcp_stream(
            std::move(io_service),
            std::forward<ConnectHandler>(connect_handler),
            std::vector<boost::asio::ip::tcp::endpoint>(endpoints),
            config, buffer_size, interface_address)
    {
    }

    /**
     * Constructor using an existing socket. The socket must be connected.
     */
    tcp_stream(
        io_service_ref io_service,
        boost::asio::ip::tcp::socket &&socket,
        const stream_config &config = stream_config());

    virtual std::size_t get_num_substreams() const override final { return 1; }

    virtual ~tcp_stream();
};

} // namespace send
} // namespace spead2

#endif // SPEAD2_SEND_TCP_H
