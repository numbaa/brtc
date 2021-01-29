#include "media_sender.h"
#include "../common/packetizer.h"

namespace brtc {

MediaSender::MediaSender()
{
}

void MediaSender::start()
{
    while (true) {
        //dxgi caputre��ͬ���ģ�camera capture���첽��
        //���Ը�CameraCapture��һ��ͬ���ӿڣ�ʲôʱ��ȥ��������������
        Frame frame = capture_->capture_one_frame();
        //encode���첽�ģ�����ʹ��ģʽ��ͬһ���ӿڼ���ι�������������ݣ�����ι���������ݷ������ӿ�
        //���������ݽӿڣ��ǰ����ݻص����ң�����������ȥȡ����
        std::vector<uint8_t> encoded_frame = encoder_->encode_one_frame(frame);
        Packetizer packetizer { encoded_frame };
        if (!packetizer.is_valid_frame())
            continue;
        RtpPacket packet;
        while (packetizer.next_packet(packet)) {
            //����transport֮ǰ��Ӧ��Ҫ�����ش���fec�ȵȴ���
            transport_->send_packet(packet.frame);
        }
    }
}

} // namespace brtc