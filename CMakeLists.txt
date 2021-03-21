cmake_minimum_required(VERSION 3.8)
project(brtc)

set(CMAKE_CXX_STANDARD 20)

if(WIN32)
add_definitions(-DNOMINMAX)
set(PLAT_SPEC_LIBRARIES "d3d11.lib" "dxgi.lib" "ws2_32.lib")
else()
set (PLAT_SPEC_LIBRARIES )
endif()

set(PUBLIC_INCLUDE_DIR "include")
set(SRC_DIR "src")

set(BRTC_PUBLIC_HEADERS
  "${PUBLIC_INCLUDE_DIR}/brtc.h"
  "${PUBLIC_INCLUDE_DIR}/brtc/frame.h"
  "${PUBLIC_INCLUDE_DIR}/brtc/interface.h"
)

set(BRTC_SOURCES_BUILTIN
  "${SRC_DIR}/builtin/capture/dxgi_capture.h"
  "${SRC_DIR}/builtin/capture/dxgi_capture.cpp"

  "${SRC_DIR}/builtin/decoder/mfx_decoder.h"
  "${SRC_DIR}/builtin/decoder/mfx_decoder.cpp"

  "${SRC_DIR}/builtin/encoder/mfx_encoder.h"
  "${SRC_DIR}/builtin/encoder/mfx_encoder.cpp"

  "${SRC_DIR}/builtin/render/d3d11_render.h"
  "${SRC_DIR}/builtin/render/d3d11_render.cpp"

  "${SRC_DIR}/builtin/p2p/p2p.h"
)
set(BRTC_SOURCES


  "${SRC_DIR}/controller/media_sender_impl.h"
  "${SRC_DIR}/controller/media_sender_impl.cpp"
  "${SRC_DIR}/controller/media_sender.cpp"
  "${SRC_DIR}/controller/media_receiver_impl.h"
  "${SRC_DIR}/controller/media_receiver_impl.cpp"
  "${SRC_DIR}/controller/media_receiver.cpp"

  "${SRC_DIR}/transport/transport.cpp"
  "${SRC_DIR}/transport/transport.h"

  "${SRC_DIR}/frame_assembler/frame_assembler.h"
  "${SRC_DIR}/common/udp_packet.h"
  "${SRC_DIR}/frame_buffer/frame_buffer.h"
  "${SRC_DIR}/common/rtp.h"
  "${SRC_DIR}/common/rtp.cpp")

add_library(${PROJECT_NAME}
  ${BRTC_PUBLIC_HEADERS}
  ${BRTC_SOURCES}
  )

add_subdirectory("third_party/SDL")
add_subdirectory("third_party/glog")
add_subdirectory("third_party/bco")
if (WIN32)
add_subdirectory("third_party/mfx_builder")
else()
add_subdirectory("third_party/IntelMediaSDK")
endif()


target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${PLAT_SPEC_LIBRARIES}
        "glog::glog"
        "SDL2-static"
        "SDL2main"
        "mfx"
        "bco"
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    "include"
)

add_subdirectory(examples)