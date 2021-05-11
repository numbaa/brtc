project(brtc)

add_brtc_object(brtc_media_sender "src/controller"
  "controller/media_sender_impl.h"
  "controller/media_sender_impl.cpp"
  "controller/media_sender.cpp"
)
target_link_libraries(brtc_media_sender
  PRIVATE
    bco
    brtc_common
)

add_brtc_object(brtc_media_receiver "src/controller"
  "controller/media_receiver_impl.h"
  "controller/media_receiver_impl.cpp"
  "controller/media_receiver.cpp"
)
target_link_libraries(brtc_media_receiver
  PRIVATE
    bco
    brtc_common
)

#transport
add_brtc_object(brtc_transport "src/transport"
  "transport/transport.cpp"
  "transport/transport.h"
)
target_link_libraries(brtc_transport
  PRIVATE
    bco
)


add_brtc_object(brtc_rtp_transport "src/transport"
  "transport/rtp_transport.h"
  "transport/rtp_transport.cpp"
)
target_link_libraries(brtc_rtp_transport
  PRIVATE
    bco
)

add_brtc_object(brtc_quic_transport "src/transport"
  "transport/quic_transport.h"
  "transport/quic_transport.cpp"
)
target_link_libraries(brtc_quic_transport
  PRIVATE
    bco
)

add_brtc_object(brtc_sctp_transport "src/transport"
  "transport/sctp_transport.h"
  "transport/sctp_transport.cpp"
)
target_link_libraries(brtc_sctp_transport
  PRIVATE
    bco
)

#common
add_brtc_object(brtc_common "src/common"
  "common/udp_packet.h"
  "common/time_utils.h"
  "common/sequence_number_util.h"
  "common/mod_ops.h"
  "common/empty.cpp"
)


#rtp
add_brtc_object(brtc_rtp "src/rtp"
  "rtp/rtp.h"
  "rtp/rtp.cpp"
  "rtp/extension.h"
  "rtp/extension.cpp"
)
target_link_libraries(brtc_rtp
  PUBLIC
    bco
)

#video
add_brtc_object(brtc_packetizer "src/video"
  "video/packetizer/packetizer.h"
  "video/packetizer/packetizer.cpp"
  "video/packetizer/packetizer_h264.h"
  "video/packetizer/packetizer_h264.cpp"
)
target_link_libraries(brtc_packetizer
  PRIVATE
    glog::glog
    brtc_common
    brtc_rtp
)

add_brtc_object(brtc_frame_assembler "src/video"
  "video/frame_assembler/frame_assembler.h"
  "video/frame_assembler/frame_assembler.cpp"
)
target_link_libraries(brtc_frame_assembler
  PRIVATE
    glog::glog
    brtc_common
    brtc_rtp
)

add_brtc_object(brtc_frame_buffer "src/video"
  "video/frame_buffer/frame_buffer.h"
  "video/frame_buffer/frame_buffer.cpp"
  "video/frame_buffer/decoded_frames_history.h"
  "video/frame_buffer/decoded_frames_history.cpp"
)
target_link_libraries(brtc_frame_buffer
  PRIVATE
    glog::glog
    brtc_common
    brtc_rtp
)

add_brtc_object(brtc_reference_finder "src/video"
  "video/reference_finder/reference_finder.h"
  "video/reference_finder/reference_finder.cpp"
  "video/reference_finder/rtp_generic_ref_finder.h"
  "video/reference_finder/rtp_generic_ref_finder.cpp"
  "video/reference_finder/rtp_seq_num_only_ref_finder.cpp"
  "video/reference_finder/rtp_seq_num_only_ref_finder.h"
  "video/reference_finder/rtp_vp8_ref_finder.cpp"
  "video/reference_finder/rtp_vp8_ref_finder.h"
  "video/reference_finder/rtp_vp9_ref_finder.cpp"
  "video/reference_finder/rtp_vp9_ref_finder.h"
  "video/reference_finder/rtp_frame_id_only_ref_finder.cpp"
  "video/reference_finder/rtp_frame_id_only_ref_finder.h"
  "video/reference_finder/vp9_globals.h"
  "video/reference_finder/vp8_globals.h"
  "video/reference_finder/h264_globals.h"
  "video/reference_finder/common_constants.h"
)
target_link_libraries(brtc_reference_finder
  PRIVATE
    glog::glog
    brtc_common
    brtc_rtp
)

add_library(${PROJECT_NAME} STATIC
    $<TARGET_OBJECTS:brtc_media_sender>
    $<TARGET_OBJECTS:brtc_media_receiver>
    $<TARGET_OBJECTS:brtc_transport>
    $<TARGET_OBJECTS:brtc_rtp_transport>
    $<TARGET_OBJECTS:brtc_quic_transport>
    $<TARGET_OBJECTS:brtc_sctp_transport>
    $<TARGET_OBJECTS:brtc_common>
    $<TARGET_OBJECTS:brtc_packetizer>
    $<TARGET_OBJECTS:brtc_frame_assembler>
    $<TARGET_OBJECTS:brtc_frame_buffer>
    $<TARGET_OBJECTS:brtc_reference_finder>
    $<TARGET_OBJECTS:brtc_rtp>
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    bco
  PRIVATE
    ${PLAT_SPEC_LIBRARIES}
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    ${PUBLIC_INCLUDE_DIR}
)

set_target_properties(${PROJECT_NAME}
  PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${BRTC_OUTPUT_DIR}
)

add_library(brtc::brtc ALIAS brtc)