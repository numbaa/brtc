cmake_minimum_required(VERSION 3.8)
project(brtc)

set(CMAKE_CXX_STANDARD 20)

if(WIN32)
add_definitions(-DNOMINMAX)
set(PLAT_SPEC_LIBRARIES "d3d11.lib" "dxgi.lib" "ws2_32.lib")
else()
set (PLAT_SPEC_LIBRARIES )
endif()

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
 "src/decoder/mfx_decoder.cpp"
 "src/common/d3d_mgr.h"
 "src/common/d3d_mgr.cpp"
 "src/encoder/mfx_encoder.h"
 "src/encoder/mfx_encoder.cpp"
 "src/common/mfx_allocator.h"
 "src/common/mfx_allocator.cpp")

add_subdirectory("third_party/SDL")
add_subdirectory("third_party/glog")
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
)

# target_include_directories(${PROJECT_NAME}
#   PUBLIC
#     "third_party/asio/asio/include"
# )