# Copyright 2019 SKA South Africa
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

from typing import Iterator, Any, List, Tuple, Sequence, Union, Text, Optional, overload
import socket

import spead2
from spead2 import _PybindStr

class RawItem(object):
    @property
    def id(self) -> int: ...
    @property
    def is_immediate(self) -> bool: ...
    @property
    def immediate_value(self) -> int: ...

class _HeapBase(object):
    @property
    def cnt(self) -> int: ...
    @property
    def flavour(self) -> spead2.Flavour: ...
    def get_items(self) -> List[RawItem]: ...
    def is_start_of_stream(self) -> bool: ...
    def is_end_of_stream(self) -> bool: ...

class Heap(_HeapBase):
    def get_descriptors(self) -> List[spead2.RawDescriptor]: ...

class IncompleteHeap(_HeapBase):
    @property
    def heap_length(self) -> int: ...
    @property
    def received_length(self) -> int: ...
    @property
    def payload_ranges(self) -> List[Tuple[int, int]]: ...

class StreamStats(object):
    heaps: int
    incomplete_heaps_evicted: int
    incomplete_Heaps_flushed: int
    packets: int
    batches: int
    worker_blocked: int
    max_batch: int
    single_packet_heaps: int
    search_dist: int
    def __add__(self, other: StreamStats) -> StreamStats: ...
    def __iadd__(self, other: StreamStats) -> None: ...

class Ringbuffer(object):
    def size(self) -> int: ...
    def capacity(self) -> int: ...

# We make a dummy _Stream base class because mypy objects to the async stream
# type overloading get as a coroutine.
class _Stream(object):
    DEFAULT_UDP_IBV_MAX_SIZE: int = ...
    DEFAULT_UDP_IBV_BUFFER_SIZE: int = ...
    DEFAULT_UDP_IBV_MAX_POLL: int = ...
    DEFAULT_MAX_HEAPS: int = ...
    DEFAULT_RING_HEAPS: int = ...
    DEFAULT_UDP_MAX_SIZE: int = ...
    DEFAULT_UDP_BUFFER_SIZE: int = ...
    DEFAULT_TCP_MAX_SIZE: int = ...
    DEFAULT_TCP_BUFFER_SIZE: int = ...

    def __init__(self, thread_pool: spead2.ThreadPool, bug_compat: int = ...,
                 max_heaps: int = ..., ring_heaps: int = ...,
                 contiguous_only: bool = ...,
                 incomplete_keep_payload_ranges: bool = ...) -> None: ...
    def __iter__(self) -> Iterator[Heap]: ...
    def get_nowait(self) -> Heap: ...
    def set_memory_allocator(self, allocator: spead2.MemoryAllocator) -> None: ...
    def set_memory_pool(self, pool: spead2.MemoryPool) -> None: ...
    def set_memcpy(self, id: int) -> None: ...
    @property
    def stop_on_stop_item(self) -> bool: ...
    @stop_on_stop_item.setter
    def stop_on_stop_item(self, value: bool) -> None: ...
    def add_buffer_reader(self, buffer: Any) -> None: ...
    @overload
    def add_udp_reader(self, port: int, max_size: int = ..., buffer_size: int = ...,
                       bind_hostname: _PybindStr = ..., socket: Optional[socket.socket] = None) -> None: ...
    @overload
    def add_udp_reader(self, socket: socket.socket, max_size: int = ...) -> None: ...
    @overload
    def add_udp_reader(self, multicast_group: _PybindStr, port: int, max_size: int = ...,
                       buffer_size: int = ..., interface_address: _PybindStr = ...) -> None: ...
    @overload
    def add_udp_reader(self, multicast_group: _PybindStr, port: int, max_size: int = ...,
                       buffer_size: int = ..., interface_index: int = ...) -> None: ...
    @overload
    def add_tcp_reader(self, port: int, max_size: int = ..., buffer_size: int = ...,
                       bind_hostname: _PybindStr = ...) -> None: ...
    @overload
    def add_tcp_reader(self, acceptor: socket.socket, max_size: int = ...) -> None: ...
    @overload
    def add_udp_ibv_reader(self, multicast_group: _PybindStr, port: int,
                           interface_address: _PybindStr,
                           max_size: int = ..., buffer_size: int = ...,
                           comp_vector: int = ..., max_poll: int = ...) -> None: ...
    @overload
    def add_udp_ibv_reader(self, endpoints: Sequence[Tuple[_PybindStr, int]],
                           interface_address: _PybindStr,
                           max_size: int = ..., buffer_size: int = ...,
                           comp_vector: int = ..., max_poll: int = ...) -> None: ...
    def add_udp_pcap_file_reader(self, filename: _PybindStr) -> None: ...
    def add_inproc_reader(self, queue: spead2.InprocQueue) -> None: ...
    def stop(self) -> None: ...
    @property
    def fd(self) -> int: ...
    @property
    def stats(self) -> StreamStats: ...
    @property
    def ringbuffer(self) -> Ringbuffer: ...

class Stream(_Stream):
    def get(self) -> Heap: ...
