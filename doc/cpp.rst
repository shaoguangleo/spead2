C++ API for spead2
==================

The C++ API is at a lower level than the Python API. In particular, item
values are treated as uninterpreted binary blobs. The protocol is directly
tied to numpy's type system, so it is not practical to implement this in C++.
The C++ API is thus best suited to situations which require the maximum
possible performance and where the data formats can be fixed in advance.

There is also no equivalent to the :py:class:`spead2.ItemGroup` and
:py:class:`spead2.send.HeapGenerator` classes. The user is responsible for
maintaining previously seen descriptors (if they are desired) and tracking
which descriptors and items need to be inserted into heaps.

.. toctree::
   :maxdepth: 2

   cpp-recv
   cpp-send