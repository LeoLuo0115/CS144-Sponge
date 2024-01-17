# Stanford CS144 Fall2021
CS144 Sponge version 2021, Lab 0-7 initial code is under the corresponding branch, you can complete the experiment by cloning this project and then rolling back.

See labs_pdf_21 folder for supporting experiments.

## Finished
- Lab 0: Networking Warmup
    - sponge/apps/webget.cc
    - sponge/libsponge/byte_stream.cc
    - sponge/libsponge/byte_stream.hh
- Lab 1: Stitching substrings into a byte stream
    - sponge/libsponge/stream_reassembler.cc
    - sponge/libsponge/stream_reassembler.hh
- Lab 2: the TCP receiver
    - sponge/libsponge/wrapping_integers.cc
    - sponge/libsponge/wrapping_integers.hh
    - sponge/libsponge/tcp_receiver.cc
    - sponge/libsponge/tcp_receiver.hh
- Lab 3: the TCP sender
    - sponge/libsponge/tcp_sender.cc
    - sponge/libsponge/tcp_sender.hh
- Lab 4: the TCP connection
    - sponge/libsponge/tcp_connection.cc
    - sponge/libsponge/tcp_sender.hh
- Lab 5: the network interface
    - sponge/libsponge/network_interface.cc
    - sponge/libsponge/network_interface.hh
    -  I can pass all the tests on my WSL, except CS144 version of webget (FullStackSocket) because of timeout. (WSL + ubuntu may be the issue)
- Lab 6: the IP router
    - sponge/libsponge/router.cc
    - sponge/libsponge/router.hh



## Sponge quickstart
For build prereqs, see [the CS144 VM setup instructions](https://web.stanford.edu/class/cs144/vm_howto).

To set up your build directory:

	$ mkdir -p <path/to/sponge>/build
	$ cd <path/to/sponge>/build
	$ cmake ..

**Note:** all further commands listed below should be run from the `build` dir.

To build:

    $ make

You can use the `-j` switch to build in parallel, e.g.,

    $ make -j$(nproc)

To test (after building; make sure you've got the [build prereqs](https://web.stanford.edu/class/cs144/vm_howto) installed!)

    $ make check_labN *(replacing N with a checkpoint number)*

The first time you run `make check_lab...`, it will run `sudo` to configure two
[TUN](https://www.kernel.org/doc/Documentation/networking/tuntap.txt) devices for use during
testing.

### build options

You can specify a different compiler when you run cmake:

    $ CC=clang CXX=clang++ cmake ..

You can also specify `CLANG_TIDY=` or `CLANG_FORMAT=` (see "other useful targets", below).

Sponge's build system supports several different build targets. By default, cmake chooses the `Release`
target, which enables the usual optimizations. The `Debug` target enables debugging and reduces the
level of optimization. To choose the `Debug` target:

    $ cmake .. -DCMAKE_BUILD_TYPE=Debug

The following targets are supported:

- `Release` - optimizations
- `Debug` - debug symbols and `-Og`
- `RelASan` - release build with [ASan](https://en.wikipedia.org/wiki/AddressSanitizer) and
  [UBSan](https://developers.redhat.com/blog/2014/10/16/gcc-undefined-behavior-sanitizer-ubsan/)
- `RelTSan` - release build with
  [ThreadSan](https://developer.mozilla.org/en-US/docs/Mozilla/Projects/Thread_Sanitizer)
- `DebugASan` - debug build with ASan and UBSan
- `DebugTSan` - debug build with ThreadSan

Of course, you can combine all of the above, e.g.,

    $ CLANG_TIDY=clang-tidy-6.0 CXX=clang++-6.0 .. -DCMAKE_BUILD_TYPE=Debug

**Note:** if you want to change `CC`, `CXX`, `CLANG_TIDY`, or `CLANG_FORMAT`, you need to remove
`build/CMakeCache.txt` and re-run cmake. (This isn't necessary for `CMAKE_BUILD_TYPE`.)

### other useful targets

To generate documentation (you'll need `doxygen`; output will be in `build/doc/`):

    $ make doc

To format (you'll need `clang-format`):

    $ make format

To see all available targets,

    $ make help
