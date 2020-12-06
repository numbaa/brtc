#include <Windows.h>
#include <WinSock2.h>
#include "transport.h"

namespace brtc {

Transport::Transport()
{
}

Transport::~Transport()
{
    WSACleanup();
}

bool Transport::init()
{
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data)) {
        return false;
    }
    socket_ = socket(AF_INET6, SOCK_DGRAM, 0);
    if (socket_ == INVALID_SOCKET) {
        return false;
    }
    SOCKADDR_IN local_addr;
    local_addr.sin_family = AF_INET6;
    local_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(1234);
    SOCKADDR_IN remote_addr;
    remote_addr.sin_family = AF_INET6;
    remote_addr.sin_addr.S_un.S_addr = 0; //FILL ME
    remote_addr.sin_port= htons(1234);
    if (bind(socket_, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        return false;
    }
    //定时发，收到对方的包就认为通了，然后开始发rtp包
}

void Transport::send_packet(std::span<uint8_t> data)
{
    //
}

void Transport::set_observer(TransportObserver* observer)
{
    observer_ = observer;
}

}