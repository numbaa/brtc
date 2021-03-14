#include "rtp.h"

namespace brtc {

constexpr size_t kFixedHeaderSize = 12;

//template <typename T>
//void read_bigendian(const uint8_t* data, T& value)
//{
//    value = 0;
//    constexpr size_t size = sizeof(T);
//    for (size_t i = 0; i < sizeof(T); i++) {
//        value |= data[size - i - 1] << (i * 8);
//    }
//}
//
//template <typename T>
//void write_bigendian(uint8_t* data, const T value)
//{
//    constexpr size_t size = sizeof(T);
//    for (size_t i = 0; i < sizeof(T); i++) {
//        data[i] = value >> ((size - i - 1) * 8);
//    }
//}

//  0                   1                   2                   3
//  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |V=2|P|X|  CC   |M|     PT      |       sequence number         |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                           timestamp                           |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |           synchronization source (SSRC) identifier            |
// +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// |            Contributing source (CSRC) identifiers             |
// |                             ....                              |
// +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// |  header eXtension profile id  |       length in 32bits        |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                          Extensions                           |
// |                             ....                              |
// +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// |                           Payload                             |
// |             ....              :  padding...                   |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |               padding         | Padding size  |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

bool RtpPacket::marker() const
{
    return buffer_[1] & 0b1000'0000;
}

uint8_t RtpPacket::payload_type() const
{
    return buffer_[1] & 0b0111'1111;
}

uint16_t RtpPacket::sequence_number() const
{
    uint16_t seq = 0;
    buffer_.read_big_endian_at(2, seq);
    return seq;
}

uint32_t RtpPacket::timestamp() const
{
    uint32_t t = 0;
    buffer_.read_big_endian_at(4, t);
    return t;
}

uint32_t RtpPacket::ssrc() const
{
    uint32_t s;
    buffer_.read_big_endian_at(8, s);
    return s;
}

std::vector<uint32_t> RtpPacket::csrcs() const
{
    const size_t num_csrc = csrcs_size();
    std::vector<uint32_t> results(num_csrc);
    for (size_t i = 0; i < num_csrc; i++) {
        buffer_.read_big_endian_at(kFixedHeaderSize + i * 4, results[i]);
    }
    return std::move(results);
}

size_t RtpPacket::csrcs_size() const
{
    return buffer_[0] & 0x0F;
}

size_t RtpPacket::headers_size() const
{
    return kFixedHeaderSize + csrcs_size() * sizeof(uint32_t) + extensions_size();
}

size_t RtpPacket::payload_size() const
{
    return buffer_.size() - headers_size() - padding_size();
}

size_t RtpPacket::padding_size() const
{
    size_t size = buffer_.size();
    return buffer_[size - 1];
}

size_t RtpPacket::extensions_size() const
{
    return extensions_size_;
}

const bco::Buffer RtpPacket::payload() const
{
    return buffer_.subbuf(headers_size(), payload_size());
}

size_t RtpPacket::size() const
{
    return buffer_.size();
}

const bco::Buffer RtpPacket::data() const
{
    return buffer_;
}

void RtpPacket::set_marker(bool marker)
{
    if (marker) {
        buffer_[0] |= 0b1000'0000;
    } else {
        buffer_[0] &= 0b0111'1111;
    }
}

void RtpPacket::set_payload_type(uint8_t pt)
{
    uint8_t payload_type = buffer_[1] & 0b1000'0000;
    buffer_[1] = payload_type | pt;
}

void RtpPacket::set_sequence_number(uint16_t seq)
{
    buffer_.write_big_endian_at(2, seq);
}

void RtpPacket::set_timestamp(uint32_t timestamp)
{
    buffer_.write_big_endian_at(2, timestamp);
}

void RtpPacket::set_ssrc(uint32_t ssrc)
{
    buffer_.write_big_endian_at(4, ssrc);
}

void RtpPacket::set_csrcs(std::span<uint32_t> csrcs)
{
    if (csrcs_size() != 0)
        return;
    uint8_t cc = csrcs.size();
    buffer_[0] = buffer_[0] | cc;
    for (size_t i = 0; i < csrcs.size(); i++) {
        buffer_.write_big_endian_at(kFixedHeaderSize + i * sizeof(uint32_t), csrcs[i]);
    }
}

void RtpPacket::parse()
{
}

} // namespace brtc