
#controller
add_brtc_object(media_sender "src/controller"
  "${SRC_DIR}/controller/media_sender_impl.h"
  "${SRC_DIR}/controller/media_sender_impl.cpp"
  "${SRC_DIR}/controller/media_sender.cpp"
)
target_link_libraries(media_sender
  PRIVATE
    bco
    common
)

add_brtc_object(media_receiver "src/controller"
  "${SRC_DIR}/controller/media_receiver_impl.h"
  "${SRC_DIR}/controller/media_receiver_impl.cpp"
  "${SRC_DIR}/controller/media_receiver.cpp"
)
target_link_libraries(media_receiver
  PRIVATE
    bco
    common
)

#transport
add_brtc_object(transport "src/transport"
  "${SRC_DIR}/transport/transport.cpp"
  "${SRC_DIR}/transport/transport.h"
  $<TARGET_OBJECTS:rtp_transport>
  $<TARGET_OBJECTS:quic_transport>
  $<TARGET_OBJECTS:sctp_transport>
)
target_link_libraries(transport
  PRIVATE
    bco
)


add_brtc_object(rtp_transport "src/transport"
  "${SRC_DIR}/transport/rtp_transport.h"
  "${SRC_DIR}/transport/rtp_transport.cpp"
)
target_link_libraries(rtp_transport
  PRIVATE
    bco
)

add_brtc_object(quic_transport "src/transport"
  "${SRC_DIR}/transport/quic_transport.h"
  "${SRC_DIR}/transport/quic_transport.cpp"
)
target_link_libraries(quic_transport
  PRIVATE
    bco
)

add_brtc_object(sctp_transport "src/transport"
  "${SRC_DIR}/transport/sctp_transport.h"
  "${SRC_DIR}/transport/sctp_transport.cpp"
)
target_link_libraries(sctp_transport
  PRIVATE
    bco
)

#common
add_brtc_object(common "src/common"
  "${SRC_DIR}/common/udp_packet.h"
  "${SRC_DIR}/common/rtp.h"
  "${SRC_DIR}/common/rtp.cpp"
  "${SRC_DIR}/common/time_utils.h"
  "${SRC_DIR}/common/sequence_number_util.h"
  "${SRC_DIR}/common/mod_ops.h"
)
target_link_libraries(common
  PUBLIC
    bco
)

#video
add_brtc_object(packetizer "src/video"
  "${SRC_DIR}/video/packetizer/packetizer.h"
  "${SRC_DIR}/video/packetizer/packetizer.cpp"
  "${SRC_DIR}/video/packetizer/packetizer_h264.h"
  "${SRC_DIR}/video/packetizer/packetizer_h264.cpp"
)
target_link_libraries(packetizer
  PRIVATE
    glog::glog
    common
)

add_brtc_object(frame_assembler "src/video"
  "${SRC_DIR}/video/frame_assembler/frame_assembler.h"
  "${SRC_DIR}/video/frame_assembler/frame_assembler.cpp"
)
target_link_libraries(frame_assembler
  PRIVATE
    glog::glog
    common
)

add_brtc_object(frame_buffer "src/video"
  "${SRC_DIR}/video/frame_buffer/frame_buffer.h"
  "${SRC_DIR}/video/frame_buffer/frame_buffer.cpp"
  "${SRC_DIR}/video/frame_buffer/decoded_frames_history.h"
  "${SRC_DIR}/video/frame_buffer/decoded_frames_history.cpp"
)
target_link_libraries(frame_buffer
  PRIVATE
    glog::glog
    common
)

add_brtc_object(reference_finder "src/video"
  "${SRC_DIR}/video/reference_finder/reference_finder.h"
  "${SRC_DIR}/video/reference_finder/reference_finder.cpp"
  "${SRC_DIR}/video/reference_finder/rtp_generic_ref_finder.h"
  "${SRC_DIR}/video/reference_finder/rtp_generic_ref_finder.cpp"
  "${SRC_DIR}/video/reference_finder/rtp_seq_num_only_ref_finder.cpp"
  "${SRC_DIR}/video/reference_finder/rtp_seq_num_only_ref_finder.h"
  "${SRC_DIR}/video/reference_finder/rtp_vp8_ref_finder.cpp"
  "${SRC_DIR}/video/reference_finder/rtp_vp8_ref_finder.h"
  "${SRC_DIR}/video/reference_finder/rtp_vp9_ref_finder.cpp"
  "${SRC_DIR}/video/reference_finder/rtp_vp9_ref_finder.h"
  "${SRC_DIR}/video/reference_finder/rtp_frame_id_only_ref_finder.cpp"
  "${SRC_DIR}/video/reference_finder/rtp_frame_id_only_ref_finder.h"
  "${SRC_DIR}/video/reference_finder/vp9_globals.h"
  "${SRC_DIR}/video/reference_finder/vp8_globals.h"
  "${SRC_DIR}/video/reference_finder/h264_globals.h"
  "${SRC_DIR}/video/reference_finder/common_constants.h"
)
target_link_libraries(reference_finder
  PRIVATE
    glog::glog
    common
)

add_library(${PROJECT_NAME} STATIC
    $<TARGET_OBJECTS:media_sender>
    $<TARGET_OBJECTS:media_receiver>
    $<TARGET_OBJECTS:transport>
    $<TARGET_OBJECTS:common>
    $<TARGET_OBJECTS:packetizer>
    $<TARGET_OBJECTS:frame_assembler>
    $<TARGET_OBJECTS:frame_buffer>
    $<TARGET_OBJECTS:reference_finder>
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    bco
  PRIVATE
    ${PLAT_SPEC_LIBRARIES}
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    "include"
)