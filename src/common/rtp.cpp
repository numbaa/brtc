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

RtpPacket::RtpPacket(bco::Buffer buff)
{
    const bool has_padding = (buff[0] & 0x20) != 0;
    const bool has_extension = (buff[0] & 0x10) != 0;
    const uint8_t number_of_crcs = buff[0] & 0x0f;
    //主要是要获得extension_entries_
    if (has_extension) {
        uint16_t magic;
        buffer_.read_big_endian_at(kFixedHeaderSize + csrcs_size() * sizeof(uint32_t), magic);
        if (magic == 0xBEDE) {
            extension_mode_ = ExtensionMode::kOneByte;
        } else if (magic == 0x0100) {
            extension_mode_ = ExtensionMode::kTwoByte;
        } else {
            //error handling
            return;
        }
        uint16_t number_of_extension;
        buffer_.read_big_endian_at(kFixedHeaderSize + csrcs_size() * sizeof(uint32_t) + sizeof(uint16_t), number_of_extension);
        size_t extension_offset = kFixedHeaderSize + csrcs_size() * sizeof(uint32_t) + sizeof(uint32_t);
        (void)extension_offset;
        //解析出extension_entries_
        for (uint16_t i = 0; i < number_of_extension; i++) {
            //
        }
    }

}

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
    return kFixedHeaderSize + csrcs_size() * sizeof(uint32_t) + sizeof(uint32_t) + extensions_size();
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
    size_t size = 0;
    if (extension_mode_ == ExtensionMode::kOneByte) {
        for (const auto& entry : extension_entries_) {
            size += (entry.length + 2) * 8;
        }
    } else {
        for (const auto& entry : extension_entries_) {
            size += entry.length * 8 + 16;
        }
    }
    return size;
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
    uint8_t cc = static_cast<uint8_t>(csrcs.size());
    buffer_[0] = buffer_[0] | cc;
    for (size_t i = 0; i < csrcs.size(); i++) {
        buffer_.write_big_endian_at(kFixedHeaderSize + i * sizeof(uint32_t), csrcs[i]);
    }
}

void RtpPacket::set_payload(const std::span<uint8_t>& payload)
{
    auto ext = buffer_.data().back();
    auto mod = ext.size() % 4;
    if (mod != 0) {
        buffer_.push_back(std::vector<uint8_t>(4 - mod, 0));
    }
    buffer_.push_back(payload, true);
}

void RtpPacket::set_payload(std::vector<uint8_t>&& payload)
{
    auto ext = buffer_.data().back();
    auto mod = ext.size() % 4;
    if (mod != 0) {
        buffer_.push_back(std::vector<uint8_t>(4 - mod, 0));
    }
    buffer_.push_back(std::move(payload), true);
}

//void RtpPacket::set_frame(Frame frame)
//{
//    frame_ = frame;
//}

void RtpPacket::parse()
{
}

bco::Buffer RtpPacket::find_extension(RTPExtensionType type)
{
    for (const auto& entry : extension_entries_) {
        if (entry.type == type) {
            return buffer_.subbuf(entry.offset, entry.offset + entry.length);
        }
    }
    return bco::Buffer();
}

void RtpPacket::promote_two_bytes_header_and_reserve_n_bytes(uint8_t n_bytes)
{
    extension_mode_ = ExtensionMode::kTwoByte;
    if (extension_entries_.empty()) {
        //第一次插入ext elem，并且是two bytes
        //插入16字节的特殊xx
        //reserve n bytes
        buffer_.push_back(std::vector<uint8_t>(2 + n_bytes));
        size_t pos = buffer_.size();
        uint16_t magic = static_cast<uint16_t>(0x100);
        buffer_.write_big_endian_at(pos, magic);
    } else {
        //uint16_t magic = 0x1'0000;
        auto tmp = buffer_.data().back();
        tmp[0] = 1;
        tmp[1] = 0;
        //每个 extension element增加了1字节
        buffer_.push_back(std::vector<uint8_t>(extension_entries_.size() + n_bytes));
        auto exts = extension_entries_.size();
        for (auto it = extension_entries_.rbegin(); it != extension_entries_.rend(); it++) {
            auto ext_index = &buffer_[it->offset];
            ::memmove(ext_index + exts, ext_index, it->length);
            *(ext_index - 1) = it->length;
            *(ext_index - 2) = static_cast<uint8_t>(it->type);
            it->offset += static_cast<uint16_t>(exts);
            exts -= 1;
        }
    }
}

void RtpPacket::allocate_n_bytes_for_extension(uint8_t bytes)
{
    if (extension_entries_.empty()) {
        //第一次插入ext elem，并且是 one byte
        //插入16字节特殊xx
        //reserve n bytes
        auto ext = std::vector<uint8_t>(2 + bytes);
        ext[0] = 0xBE;
        ext[1] = 0xDE;
        ext[2] = 0;
        ext[3] = 0;
        buffer_.push_back(std::move(ext), true);
    } else {
        buffer_.push_back(std::vector<uint8_t>(bytes));
    }
}

} // namespace brtc