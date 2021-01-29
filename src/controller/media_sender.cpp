#include "media_sender.h"
#include "../common/packetizer.h"

namespace brtc {

MediaSender::MediaSender()
{
}

void MediaSender::start()
{
    while (true) {
        //dxgi caputre是同步的，camera capture是异步的
        //可以给CameraCapture做一个同步接口，什么时候去调都能立即返回
        Frame frame = capture_->capture_one_frame();
        //encode是异步的，两种使用模式，同一个接口即是喂数据又是吐数据，或者喂数据吐数据分两个接口
        //对于吐数据接口，是把数据回调给我，还是我主动去取数据
        std::vector<uint8_t> encoded_frame = encoder_->encode_one_frame(frame);
        Packetizer packetizer { encoded_frame };
        if (!packetizer.is_valid_frame())
            continue;
        RtpPacket packet;
        while (packetizer.next_packet(packet)) {
            //交给transport之前，应该要经过重传、fec等等处理
            transport_->send_packet(packet.frame);
        }
    }
}

} // namespace brtc