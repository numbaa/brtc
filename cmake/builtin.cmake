
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

    "${SRC_DIR}/builtin/builtin.cpp"
)

add_library(brtc_builtin STATIC
  ${BRTC_PUBLIC_HEADERS}
  ${BRTC_SOURCES_BUILTIN}
)

target_link_libraries(brtc_builtin
  PRIVATE
    bco
    brtc
    d3d11.lib
    dxgi.lib
    mfx
    common
    glog::glog
    SDL2main
    SDL2-static
)

set_target_properties(brtc_builtin PROPERTIES FOLDER "src/builtin")

if(WIN32)
target_compile_definitions(brtc_builtin PUBLIC
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)
endif()