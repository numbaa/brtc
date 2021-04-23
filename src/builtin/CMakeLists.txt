project(brtc_builtin)

add_brtc_object(dxgi_capture "src/builtin"
    "capture/dxgi_capture.h"
    "capture/dxgi_capture.cpp"
)
target_link_libraries(dxgi_capture
    PRIVATE
        brtc
        d3d11.lib
        dxgi.lib
)

add_brtc_object(mfx_decoder "src/builtin"
    "decoder/mfx_decoder.h"
    "decoder/mfx_decoder.cpp"
)
target_link_libraries(mfx_decoder
    PRIVATE
        brtc
        d3d11.lib
        mfx
)

add_brtc_object(mfx_encoder "src/builtin"
    "encoder/mfx_encoder.h"
    "encoder/mfx_encoder.cpp"
)
target_link_libraries(mfx_encoder
    PRIVATE
        brtc
        d3d11.lib
        mfx
        glog::glog
)

add_brtc_object(mfx_common "src/builtin"
    "mfx_common/mfx_allocator.h"
    "mfx_common/mfx_allocator.cpp"
)
target_link_libraries(mfx_common
    PUBLIC
        d3d11.lib
        mfx
        glog::glog
)

add_brtc_object(d3d11_render "src/builtin"
    "render/d3d11_render.h"
    "render/d3d11_render.cpp"
)
target_link_libraries(d3d11_render
    PRIVATE
        brtc
        d3d11.lib
        SDL2main
        SDL2-static
)

add_brtc_object(p2p "src/builtin"
    "p2p/p2p.h"
    "p2p/p2p.cpp"
)

add_library(${PROJECT_NAME} STATIC
    ${BRTC_PUBLIC_HEADERS}
    "builtin.cpp"
    $<TARGET_OBJECTS:dxgi_capture>
    $<TARGET_OBJECTS:mfx_common>
    $<TARGET_OBJECTS:mfx_decoder>
    $<TARGET_OBJECTS:mfx_encoder>
    $<TARGET_OBJECTS:d3d11_render>
    $<TARGET_OBJECTS:p2p>
)

target_include_directories(${PROJECT_NAME}
  PUBLIC ${PUBLIC_INCLUDE_DIR}
  PRIVATE ${SRC_DIR}
)
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        bco
        mfx
        SDL2main
        SDL2-static
)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "src/builtin")

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PUBLIC
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
endif()

add_library(brtc::builtin ALIAS ${PROJECT_NAME})