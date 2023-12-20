#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

#include <cstdint>
#include <string>

//! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
//! possibly overlapping) into an in-order byte stream.
class StreamReassembler {
  private:
    // Your code here -- add private members as necessary.

    ByteStream _output;           //!< The reassembled in-order byte stream
    size_t _capacity;             //!< The maximum number of bytes
    size_t _next_index;           //!< The index of the next byte expected in the stream
    size_t _unassembled_bytes;    //!< The number of bytes in the substrings stored but not yet reassembled
    bool _should_eof;             //!< Flag about telling ByteStream to end input
    std::vector<char> _stream{};  //!< The window
    std::vector<bool> _dirty{};   //!< A table to indicate whether the element is stored
    size_t next(size_t ptr) { return (ptr + 1) % _capacity; }  //!< Get the next index of the window

  public:
    //! \brief Construct a `StreamReassembler` that will store up to `capacity` bytes.
    //! \note This capacity limits both the bytes that have been reassembled,
    //! and those that have not yet been reassembled.
    StreamReassembler(const size_t capacity)
        : _output(capacity), _capacity(capacity), _next_index(0), _unassembled_bytes(0), _should_eof(false) {
        _stream.resize(capacity, 0);  // Initialize the window and make it full of 0 (empty)
        _dirty.resize(capacity, false);
    }

    //! \brief Receive a substring and write any newly contiguous bytes into the stream.
    //!
    //! The StreamReassembler will stay within the memory limits of the `capacity`.
    //! Bytes that would exceed the capacity are silently discarded.
    //!
    //! \param data the substring
    //! \param index indicates the index (place in sequence) of the first byte in `data`
    //! \param eof the last byte of `data` will be the last byte in the entire stream
    void push_substring(const std::string &data, const uint64_t index, const bool eof);

    //! \name Access the reassembled byte stream
    //!@{
    const ByteStream &stream_out() const { return _output; }
    ByteStream &stream_out() { return _output; }
    //!@}

    //! The number of bytes in the substrings stored but not yet reassembled
    //!
    //! \note If the byte at a particular index has been pushed more than once, it
    //! should only be counted once for the purpose of this function.
    size_t unassembled_bytes() const;

    //! \brief Is the internal state empty (other than the output stream)?
    //! \returns `true` if no substrings are waiting to be assembled
    bool empty() const;
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
