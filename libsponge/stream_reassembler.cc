#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // When the string is out of the `_next_index+_capacity` or the end of the string
    // is before the `_next_index`, we should do NOTHING.
    if (index >= _capacity + _next_index || _next_index > index + data.size()) {
        return;
    }

    // Here we should consider the situation that we should accept part of
    // the string, when `actual_index < _next_index`. We should set the `actual_index`
    // to be the `_next_index` to drop the previous string, and make `data_index`
    // to be the `_next_index-index`.
    size_t actual_index = index;
    size_t data_index = 0;
    if (index < _next_index) {
        actual_index = _next_index;
        // data_index is the index of start of valid data
        data_index += _next_index - index;
    }

    // Here we need to find the start index and loop index
    // this is the start index of the `_stream`, which is the `_next_index` nothing change just different name
    size_t start_index = _next_index % _capacity;
    // this is the loop index of the `_stream`, which is the index where we should start to store the data,
    // it might not be the same as the `_next_index` (start index).
    size_t loop_index = actual_index % _capacity;

    // Here we should consider the situation that the we do not have enough space to store the data
    size_t first_unacceptable_idx = _next_index + _capacity - _output.buffer_size();
    if (first_unacceptable_idx  <= actual_index) {
        return;
    }

    // if we do have enough space to store the data, we should calculate how much reamining space we have
    // size of vaild data
    size_t loop_size = data.size() - data_index;
    // loop_size = loop_size > _capacity - _output.buffer_size() ? _capacity - _output.buffer_size() : loop_size;
    loop_size = min(loop_size, first_unacceptable_idx - actual_index);

    // Corner case: when the data is empty. This is important
    // Because the below iteration does not consider
    if (data.empty()) {
        if (eof) {
            _should_eof = true;
        }
    }

    // Here, when `_dirty[index] == false` We store the byte into
    // the `_stream[index]` and set the `_dirty[index]` to be true.
    for (size_t i = loop_index, j = data_index; j < data_index + loop_size; i = next(i), j++) {
        if (!_dirty[i]) {
            _stream[i] = data[j];
            _unassembled_bytes++;
            _dirty[i] = true;
        }
        // j is the last index of the data and if we have eof is true then we should set the flag to be true
        if (j + 1 == data.size()) {
            if (eof) {
                _should_eof = true;
            }
        }

        // we can not store the data into start_index, but we are able to fill the rest of the data
        if (next(i) == start_index)
            break;
    }

    // We should calculate consecutive `_dirty[index]` from `_next_index`.
    // Pay attention, you shouldn't set the `dirty[index]` to false,
    // because we DO NOT KNOW the ByteStream's size, so we
    // need to get the bytes actually written into the `_output`.
    string send_str{};
    for (size_t i = start_index; _dirty[i]; i = next(i)) {
        send_str.push_back(_stream[i]);
        // we pushed all the data into to send_str
        if (next(i) == start_index)
            break;
    }

    // If there is need to send the bytes
    if (!send_str.empty()) {
        size_t write_num = stream_out().write(send_str);
        for (size_t i = start_index, j = 0; j < write_num; i = next(i), ++j) {
            _dirty[i] = false;
        }
        _next_index += write_num;
        _unassembled_bytes -= write_num;
    }

    if (_should_eof && empty()) {
        stream_out().end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return {_unassembled_bytes}; }

bool StreamReassembler::empty() const { return {_unassembled_bytes == 0}; }
