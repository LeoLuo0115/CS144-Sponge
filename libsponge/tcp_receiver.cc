#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    const TCPHeader &header = seg.header();
    if (!_isn.has_value()) {
        if (!header.syn)
            return;  // SYN not set, ignore this segment
        _isn = header.seqno;
    }

    // checkpoint is first unassembled byte index
    uint64_t checkpoint = _reassembler.stream_out().bytes_written();
    // abs_seqno is absolute seqno of the first byte of the segment
    uint64_t abs_seq = unwrap(header.seqno, _isn.value(), checkpoint);
    // if SYN is set, then the stream index for syn is 0 - 1 = -1
    // we can not have negative stream index, so we add 1 to abs_seq
    if (header.syn) {
        abs_seq += 1;
    }
    uint64_t stream_index = abs_seq - 1;
    _reassembler.push_substring(seg.payload().copy(), stream_index, header.fin);
}

// ackno() returns the next ackno that the receiver is expecting
// ackno() type is optional<WrappingInt32> (sequence number)
optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!_isn.has_value()) {
        return {};
    }
    // return seqno after the last byte of the stream, the seqno after FIN
    if (_reassembler.stream_out().input_ended()) {
        // reassembler.stream_out().bytes_written() + 1 transfer stream index to absolute seqno
        // wrap() transfer absolute seqno to seqno
        // +1 is move to seqno after FIN
        return WrappingInt32(wrap(_reassembler.stream_out().bytes_written() + 1, _isn.value())) + 1;
    } else {
        return WrappingInt32(wrap(_reassembler.stream_out().bytes_written() + 1, _isn.value()));
    }
}

size_t TCPReceiver::window_size() const { return stream_out().remaining_capacity(); }
