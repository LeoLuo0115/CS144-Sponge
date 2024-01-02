#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _retransmission_timer{retx_timeout} {}

uint64_t TCPSender::bytes_in_flight() const { return _next_seqno - _receiver_ack; }

void TCPSender::fill_window() {
    // Special case: we have already sent the `FIN`.
    if (end) {
        return;
    }

    TCPSegment segment{};

    // Special case: when the `_receiver_window_size` equals 0
    uint64_t window_size = _receiver_window_size == 0 ? 1 : _receiver_window_size;

    // Special case : TCP connection
    // we need to send out SYN, and intial _receiver_window_size is 1
    if (_next_seqno == 0) {
        segment.header().syn = true;
        segment.header().seqno = _isn + _next_seqno;
        _next_seqno += 1;
    }
    // TCP connection establish, and ready to read from its input ByteStream
    // and sends as many bytes as possible in the form of TCPSegments,
    else {
        uint64_t length =
            std::min(std::min(window_size - bytes_in_flight(), stream_in().buffer_size()), TCPConfig::MAX_PAYLOAD_SIZE);

        // segment's payload is stored as a reference-counted read-only string (a Buffer object).
        segment.payload() = Buffer{std::move(stream_in().read(length))};

        // keep track of each segment seqno (32bits) and _next_seqno (64bits / absoult seqno)
        segment.header().seqno = _isn + _next_seqno;
        _next_seqno += length;

        // When the `stream_in` is end of file,  we need to set the `fin` to `true`.
        // Pay attention, we should check whether there is an enough window size
        if (stream_in().eof() && window_not_full(window_size)) {
            segment.header().fin = true;
            end = true;
            _next_seqno++;
        }

        // Do nothing: either receiver does not have space or bytestream does not have input yet
        if (length == 0 && !end)
            return;
    }

    // Now we can actually push our segment to the queue
    _segments_out.push(segment);
    // keep a copy of the segment
    _outstanding_segments.push_back(segment);
    // stater the RTO alarm
    _retransmission_timer.start_timer();
    // if we have engough window size, we can send out the segment again
    if (window_not_full(window_size)) {
        fill_window();
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
// The TCPSender should look through its collection of outstanding segments and remove any that have now been fully
// acknowledged ackno is greater than all of the sequence numbers in the segment
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    // DUMMY_CODE(ackno, window_size);

    uint64_t abs_ackno = unwrap(ackno, _isn, next_seqno_absolute());
    // wrong ackno, ackno can not be greater than next_seqno, or less than _receiver_ack
    // ackno is 左闭右开区间，所以 abs_ackno 可以 == _next_seqno
    // ackno == _next_seqno 时，说明 receiver 已经收到了所有的数据
    if (abs_ackno > _next_seqno || abs_ackno < _receiver_ack) {
        return;
    }

    _receiver_window_size = window_size;
    bool is_ack_update = false;

    // 2. 把 ackno 之前的 segment 都删掉
    for (auto it = _outstanding_segments.begin(); it != _outstanding_segments.end();) {
        uint64_t abs_seqno = unwrap(it->header().seqno, _isn, next_seqno_absolute());
        if (abs_seqno + it->length_in_sequence_space() <= abs_ackno) {
            _receiver_ack = abs_seqno + it->length_in_sequence_space();
            it = _outstanding_segments.erase(it);
            is_ack_update = true;
        } else {
            it++;
        }
    }

    // When there is no outstanding segments, we should stop the timer
    if (_outstanding_segments.empty()) {
        _retransmission_timer.stop_timer();
    }

    // _retransmission_timer 记录的是在RTO时间内有没有收到合法的ACK,如果没有收到合法的ACK，那么就需要重传

    // When the receiver gives the sender an ackno that acknowledges
    // the successful receipt of new data
    if (is_ack_update) {
        _retransmission_timer.reset_timer();
        _consecutive_retransmissions = 0;
    }

    // if we have engough window size, we can send out the segment again
    if (window_not_full(window_size)) {
        fill_window();
    }
}

/*
下面简单的概括一下timer使用的时机:

当发送一个新的segment的时候，如果timer没有开启，那么需要开启timer。

当在RTO内收到一个合法的ACK,有两种情况:
如果sender没发完segments那么需要重启timer,重启的意思是timer从0开始计时。
如果sender已经发完所有的segments了那么需要关闭timer

当超时的情况发生,也是两种情况:
window_size = 0 : 重启timer,重传segments。
window_size != 0 : double RTO, 重启timer,重传segments。
*/

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    // DUMMY_CODE(ms_since_last_tick);
    if (_retransmission_timer.timer_expired(ms_since_last_tick)) {
        if (_receiver_window_size == 0) {
            _retransmission_timer.reset_timer();
        } else {
            _retransmission_timer.handle_expired();
        }
        _consecutive_retransmissions++;
        segments_out().push(_outstanding_segments.front());
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment empty{};
    empty.header().seqno = _isn + _next_seqno;
    segments_out().push(empty);
}
