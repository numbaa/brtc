#pragma once

namespace brtc {

class QuicTransport {
public:
    QuicTransport();
    void on_recv_data(bco::Buffer buff);
};

} // namespace
