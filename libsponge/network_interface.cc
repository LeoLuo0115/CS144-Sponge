#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] ip the IP address to send the ARP request to all other hosts in the Local Area Network
EthernetFrame NetworkInterface::broadcast_frame(uint32_t ip) {
    ARPMessage arp_msg;
    arp_msg.opcode = ARPMessage::OPCODE_REQUEST;
    arp_msg.sender_ethernet_address = _ethernet_address;
    arp_msg.sender_ip_address = _ip_address.ipv4_numeric();
    arp_msg.target_ethernet_address = {};
    arp_msg.target_ip_address = ip;

    EthernetHeader header;
    header.src = _ethernet_address;
    header.dst = ETHERNET_BROADCAST;
    // we have two tpes of Ethernet frames, one for IPv4 and one for ARP, we only send APR here, but can receive both
    header.type = header.TYPE_ARP;

    EthernetFrame frame;
    frame.header() = header;
    frame.payload() = arp_msg.serialize();

    return frame;
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();

    auto it = _arp_table.find(next_hop_ip);
    // if not found in ARP table, send ARP request to all other hosts in the Local Area Network
    if (it == _arp_table.end()) {
        // first store the datagram and next hop address in the cache
        _dgram_cache.push_back({next_hop, dgram});

        // if we have already sent an ARP request to that ip address, we don't need to send another one
        if (waiting_msg.find(next_hop_ip) != waiting_msg.end()) {
            return;
        } else {
            // send ARP request to all other hosts in the Local Area Network
            // create the boradcast frame
            EthernetFrame frame = broadcast_frame(next_hop_ip);
            // send the frame
            _frames_out.push(frame);
            // record the time when we send the frame
            waiting_msg[next_hop_ip] = _time;
        }
    } else {
        // if found in ARP table, send the datagram
        EthernetFrame frame;
        frame.header().src = _ethernet_address;
        frame.header().dst = it->second.first;
        frame.header().type = frame.header().TYPE_IPv4;
        frame.payload() = dgram.serialize();
        _frames_out.push(frame);
    }
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    EthernetHeader header = frame.header();
    // only accept the frame whose destination is broadcast address or the ethernet address is itself
    if (frame.header().dst != ETHERNET_BROADCAST && frame.header().dst != _ethernet_address) {
        return {};
    }

    // if the frame is datagram type, then extract the InternetDatagram from the frame
    if (header.type == header.TYPE_IPv4) {
        InternetDatagram ip_datagram;
        ParseResult res = ip_datagram.parse(frame.payload());
        if (res == ParseResult::NoError) {
            return ip_datagram;
        } else {
            return {};
        }
    }
    // if the frame is ARP type, then extract the ARPMessage from the frame and update the ARP table
    else {
        ARPMessage arp_msg;
        ParseResult res = arp_msg.parse(frame.payload());
        if (res == ParseResult::NoError) {
            // get the sender's ethernet address and ip address
            EthernetAddress eth_addr = arp_msg.sender_ethernet_address;
            uint32_t ip_addr = arp_msg.sender_ip_address;

            // if it is an ARP request requesting our IP address, send the appropriate ARP reply
            if ((arp_msg.opcode == ARPMessage::OPCODE_REQUEST) &&
                (arp_msg.target_ip_address == _ip_address.ipv4_numeric())) {
                // set the frame header
                EthernetHeader header_send;
                header_send.type = header_send.TYPE_ARP;
                header_send.dst = arp_msg.sender_ethernet_address;
                header_send.src = _ethernet_address;

                // set the ARP message
                ARPMessage arp_msg_send;
                arp_msg_send.opcode = arp_msg_send.OPCODE_REPLY;
                arp_msg_send.sender_ethernet_address = _ethernet_address;
                arp_msg_send.sender_ip_address = _ip_address.ipv4_numeric();
                arp_msg_send.target_ethernet_address = arp_msg.sender_ethernet_address;
                arp_msg_send.target_ip_address = arp_msg.sender_ip_address;

                // set the frame
                EthernetFrame frame_send;
                frame_send.header() = header_send;
                frame_send.payload() = arp_msg_send.serialize();

                // send the frame
                _frames_out.push(frame_send);
            }
            // OPCODE is reply update the our ARP table
            _arp_table[ip_addr] = {eth_addr, _time};

            // we have revceived the ARP reply, so we can send the datagram in the cache
            for (auto it = _dgram_cache.begin(); it != _dgram_cache.end();) {
                Address addr_cache = it->first;
                InternetDatagram dgram_cache = it->second;
                // if the ip address of the next hop is the same as the sender's ip address, send the datagram
                if (addr_cache.ipv4_numeric() == ip_addr) {
                    send_datagram(dgram_cache, addr_cache);
                    // remove the datagram from the cache
                    _dgram_cache.erase(it++);
                } else {
                    it++;
                }
            }

            // remove the waiting message from the waiting_msg
            waiting_msg.erase(ip_addr);
        }
    }
    return {};
}
//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    // time have elapsed
    _time += ms_since_last_tick;
    // update the ARP table, TTL = 30s
    for (auto it = _arp_table.begin(); it != _arp_table.end();) {
        // if the time have elapsed more than 30s, remove the entry from the ARP table
        if (_time - (it->second).second >= 30 * 1000) {
            _arp_table.erase(it++);
        } else {
            it++;
        }
    }
    // update the waiting message, TTL = 5s
    for (auto it = waiting_msg.begin(); it != waiting_msg.end(); it++) {
        // if the time have elapsed more than 5s, send the ARP request again
        if (_time - it->second >= 5 * 1000) {
            // IP in the waiting message means it never received the ARP reply
            // so we boradcast the ARP request again after 5s
            // create the boradcast frame
            EthernetFrame frame = broadcast_frame(it->first);
            // send the frame
            _frames_out.push(frame);
            // update the time
            it->second = _time;
        }
    }
}
