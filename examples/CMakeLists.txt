project(examples)

add_library(brtc_examples_common OBJECT
  "common/d3d_helper.h"
  "common/d3d_helper.cpp"
)

target_include_directories(brtc_examples_common
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(brtc_examples_common
  PUBLIC mfx d3d11.lib dxgi.lib
)

add_subdirectory(recv_stream)

add_subdirectory(send_stream)
