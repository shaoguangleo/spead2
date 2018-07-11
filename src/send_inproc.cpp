/* Copyright 2018 SKA South Africa
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

#include <cstddef>
#include <utility>
#include <memory>
#include <boost/asio.hpp>
#include <spead2/common_thread_pool.h>
#include <spead2/common_ringbuffer.h>
#include <spead2/common_semaphore.h>
#include <spead2/common_logging.h>
#include <spead2/send_packet.h>
#include <spead2/send_inproc.h>

namespace spead2
{
namespace send
{

namespace detail
{

packet copy_packet(const packet &in)
{
    std::size_t size = boost::asio::buffer_size(in.buffers);
    packet out;
    out.data.reset(new std::uint8_t[size]);
    boost::asio::mutable_buffer buffer(out.data.get(), size);
    boost::asio::buffer_copy(buffer, in.buffers);
    out.buffers.emplace_back(buffer);
    return out;
}

} // namespace detail

inproc_stream::inproc_stream(
    io_service_ref io_service,
    std::shared_ptr<ringbuffer<packet, semaphore_fd, semaphore_fd>> queue,
    const stream_config &config)
    : stream_impl<inproc_stream>(std::move(io_service), config),
    queue(std::move(queue)),
    space_sem_wrapper(wrap_fd(get_io_service(), this->queue->get_space_sem().get_fd()))
{
}

} // namespace send
} // namespace spead2
