cmake_minimum_required(VERSION 3.8)
project(brtc)

set(CMAKE_CXX_STANDARD 20)

add_executable(${PROJECT_NAME}
  "src/main.cpp"
  "src/controller/controller.h"
  "src/common/packetizer.h"
  "src/common/packetizer.cpp"
  "src/common/rtp.h"
  "src/common/transport.h"
  "src/common/transport.cpp"
  "src/common/frame.h"
  "src/common/video_source.h"
  "src/common/encoder.h"
  "src/common/decoder.h"
  "src/common/render.h"
  "src/capture/dxgi_capture.h"
  "src/capture/dxgi_capture.cpp"
  "src/render/d3d11_render.h"
  "src/render/d3d11_render.cpp"
  "src/decoder/mfx_decoder.h"
 "src/decoder/mfx_decoder.cpp" "src/common/d3d_helper.h")


target_include_directories(${PROJECT_NAME}
  PRIVATE
    "$ENV{INTELMEDIASDKROOT}/include"
)

# target_include_directories(${PROJECT_NAME}
#   PUBLIC
#     "third_party/asio/asio/include"
# )