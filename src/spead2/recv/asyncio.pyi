# Copyright 2019, 2023 National Research Foundation (SARAO)
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

from collections.abc import AsyncIterator

import spead2
import spead2.recv

class Stream(spead2.recv._RingStream):
    async def get(self) -> spead2.recv.Heap: ...
    def __aiter__(self) -> AsyncIterator[spead2.recv.Heap]: ...

class ChunkRingbuffer(spead2.recv._ChunkRingbuffer):
    async def get(self) -> spead2.recv.Chunk: ...
    async def put(self, chunk: spead2.recv.Chunk): ...
    def __aiter__(self) -> AsyncIterator[spead2.recv.Chunk]: ...
