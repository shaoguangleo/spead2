# Copyright 2018-2020 SKA South Africa
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""Test that data can be passed through the various async transports."""

import socket
import asyncio

from nose.tools import assert_equal

import spead2
import spead2.send
import spead2.recv.asyncio
import spead2.send.asyncio

from . import test_passthrough


class BaseTestPassthroughAsync(test_passthrough.BaseTestPassthrough):
    def transmit_item_groups(self, item_groups, memcpy, allocator, new_order='='):
        self.loop = asyncio.new_event_loop()
        ret = self.loop.run_until_complete(
            self.transmit_item_groups_async(item_groups, memcpy, allocator, new_order))
        self.loop.close()
        return ret

    async def transmit_item_groups_async(self, item_groups, memcpy, allocator, new_order='='):
        if self.requires_ipv6:
            self.check_ipv6()
        recv_config = spead2.recv.StreamConfig(memcpy=memcpy)
        if allocator is not None:
            recv_config.memory_allocator = allocator
        receivers = [
            spead2.recv.asyncio.Stream(spead2.ThreadPool(), recv_config, loop=self.loop)
            for i in range(len(item_groups))
        ]
        await self.prepare_receivers(receivers)
        sender = await self.prepare_senders(spead2.ThreadPool(), len(item_groups))
        gens = [
            spead2.send.HeapGenerator(item_group)
            for item_group in item_groups
        ]
        if len(item_groups) != 1:
            # Use reversed order so that if everything is actually going
            # through the same transport it will get picked up.
            for i, gen in reversed(list(enumerate(gens))):
                await sender.async_send_heap(gen.get_heap(), substream_index=i)
            for i, gen in enumerate(gens):
                await sender.async_send_heap(gen.get_end(), substream_index=i)
        else:
            # This is a separate code path to give coverage of the case where
            # the substream index is implicit.
            await sender.async_send_heap(gens[0].get_heap())
            await sender.async_send_heap(gens[0].get_end())
        await sender.async_flush()
        received_item_groups = []
        for receiver in receivers:
            ig = spead2.ItemGroup()
            for heap in receiver:
                ig.update(heap, new_order)
            received_item_groups.append(ig)
        return received_item_groups

    async def prepare_receivers(self, receivers):
        """Generate receivers to use in the test."""
        assert_equal(1, len(receivers))
        await self.prepare_receiver(receivers[0])

    async def prepare_senders(self, thread_pool, n):
        """Generate a sender to use in the test, with `n` substreams."""
        assert_equal(1, n)
        return await self.prepare_sender(thread_pool)


class BaseTestPassthroughSubstreamsAsync(
        test_passthrough.BaseTestPassthroughSubstreams, BaseTestPassthroughAsync):
    async def prepare_receivers(self, receivers):
        raise NotImplementedError()

    async def prepare_senders(self, thread_pool, n):
        raise NotImplementedError()


class TestPassthroughUdp(BaseTestPassthroughSubstreamsAsync):
    async def prepare_receivers(self, receivers):
        for i, receiver in enumerate(receivers):
            receiver.add_udp_reader(8888 + i, bind_hostname="localhost")

    async def prepare_senders(self, thread_pool, n):
        if n == 1:
            return spead2.send.asyncio.UdpStream(
                thread_pool, "localhost", 8888,
                spead2.send.StreamConfig(rate=1e7),
                buffer_size=0, loop=self.loop)
        else:
            return spead2.send.asyncio.UdpStream(
                thread_pool,
                [("localhost", 8888 + i) for i in range(n)],
                spead2.send.StreamConfig(rate=1e7),
                buffer_size=0, loop=self.loop)


class TestPassthroughUdpCustomSocket(BaseTestPassthroughSubstreamsAsync):
    async def prepare_receivers(self, receivers):
        self._ports = []
        for receiver in receivers:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
            sock.bind(('127.0.0.1', 0))
            self._ports.append(sock.getsockname()[1])
            receiver.add_udp_reader(sock)
            sock.close()

    async def prepare_senders(self, thread_pool, n):
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        if n == 1:
            stream = spead2.send.asyncio.UdpStream(
                thread_pool, sock, '127.0.0.1', self._ports[0],
                spead2.send.StreamConfig(rate=1e7), loop=self.loop)
        else:
            stream = spead2.send.asyncio.UdpStream(
                thread_pool, sock,
                [('127.0.0.1', port) for port in self._ports],
                spead2.send.StreamConfig(rate=1e7), loop=self.loop)
        sock.close()
        return stream


class TestPassthroughTcp(BaseTestPassthroughAsync):
    async def prepare_receiver(self, receiver):
        receiver.add_tcp_reader(8888, bind_hostname="127.0.0.1")

    async def prepare_sender(self, thread_pool):
        sender = await spead2.send.asyncio.TcpStream.connect(
            thread_pool, "127.0.0.1", 8888, loop=self.loop)
        return sender


class TestPassthroughTcpCustomSocket(BaseTestPassthroughAsync):
    async def prepare_receiver(self, receiver):
        sock = socket.socket()
        # Prevent second iteration of the test from failing
        sock.bind(('127.0.0.1', 0))
        self._port = sock.getsockname()[1]
        sock.listen(1)
        receiver.add_tcp_reader(sock)
        sock.close()

    async def prepare_sender(self, thread_pool):
        sock = socket.socket()
        sock.setblocking(False)
        await self.loop.sock_connect(sock, ('127.0.0.1', self._port))
        sender = spead2.send.asyncio.TcpStream(
            thread_pool, sock, loop=self.loop)
        sock.close()
        return sender


class TestPassthroughInproc(BaseTestPassthroughSubstreamsAsync):
    async def prepare_receivers(self, receivers):
        assert len(receivers) == len(self._queues)
        for receiver, queue in zip(receivers, self._queues):
            receiver.add_inproc_reader(queue)

    async def prepare_senders(self, thread_pool, n):
        assert n == len(self._queues)
        if n == 1:
            return spead2.send.asyncio.InprocStream(thread_pool, self._queues[0], loop=self.loop)
        else:
            return spead2.send.asyncio.InprocStream(thread_pool, self._queues, loop=self.loop)

    async def transmit_item_groups_async(self, item_groups, memcpy, allocator, new_order='='):
        self._queues = [spead2.InprocQueue() for ig in item_groups]
        ret = await super(TestPassthroughInproc, self).transmit_item_groups_async(
            item_groups, memcpy, allocator, new_order)
        for queue in self._queues:
            queue.stop()
        return ret
