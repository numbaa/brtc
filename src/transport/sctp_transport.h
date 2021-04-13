#pragma once

namespace brtc {

class SctpTransport {
public:
    SctpTransport();
    void on_recv_data(bco::Buffer buff);
};

} // namespace
