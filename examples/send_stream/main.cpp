#include <memory>

#include <bco.h>
#include <brtc.h>

class SendStream {
    using Context = bco::Context<bco::net::Select>;
public:
    SendStream();
    void set_listen_addr(bco::net::Address addr);
    void start();

private:
    std::shared_ptr<Context> context_;
    brtc::MediaSender sender_;
    bco::net::Address listen_addr_;
};

SendStream::SendStream()
    : context_(std::make_shared<Context>(std::make_unique<bco::SimpleExecutor>()))
    , sender_(
          brtc::TransportInfo {}, nullptr /*encoder*/, nullptr /*capture*/, context_, context_, context_)
{
}

void SendStream::set_listen_addr(bco::net::Address addr)
{
    listen_addr_ = addr;
}

void SendStream::start()
{
    context_->start();
    sender_.start();
}

int main()
{
    SendStream stream;
    stream.set_listen_addr(bco::net::Address { bco::to_ipv4("127.0.0.1"), 33445 });
    stream.start();

    return 0;
}