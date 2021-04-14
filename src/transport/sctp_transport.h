#pragma once
#include <bco/buffer.h>

namespace brtc {

class SctpTransport {
public:
    SctpTransport() = default;
    void on_recv_data(bco::Buffer buff);
};

} // namespace
