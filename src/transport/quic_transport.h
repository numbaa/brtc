#pragma once
#include <bco/buffer.h>

namespace brtc {

class QuicTransport {
public:
    QuicTransport() = default;
    void on_recv_data(bco::Buffer buff);
};

} // namespace
