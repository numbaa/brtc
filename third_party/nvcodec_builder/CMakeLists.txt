project(nvcodec)

set(NVCODEC_API_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/../NvCodecSDK/Interface")
set(NVCODEC_SRC_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/../NvCodecSDK/Samples/NvCodec")
set(NVCODEC_LIB_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/../NvCodecSDK/Lib")
set(NVCODEC_SOURCES
    "${NVCODEC_SRC_FOLDER}/NvDecoder/NvDecoder.h"
    "${NVCODEC_SRC_FOLDER}/NvDecoder/NvDecoder.cpp"

    "${NVCODEC_SRC_FOLDER}/NvEncoder/NvEncoderOutputInVidMemD3D11.cpp"
    "${NVCODEC_SRC_FOLDER}/NvEncoder/NvEncoderOutputInVidMemD3D11.h"
    "${NVCODEC_SRC_FOLDER}/NvEncoder/NvEncoder.cpp"
    "${NVCODEC_SRC_FOLDER}/NvEncoder/NvEncoder.h"
    "${NVCODEC_SRC_FOLDER}/NvEncoder/NvEncoderCuda.cpp"
    "${NVCODEC_SRC_FOLDER}/NvEncoder/NvEncoderCuda.h"
    "${NVCODEC_SRC_FOLDER}/NvEncoder/NvEncoderD3D9.cpp"
    "${NVCODEC_SRC_FOLDER}/NvEncoder/NvEncoderD3D9.h"
    "${NVCODEC_SRC_FOLDER}/NvEncoder/NvEncoderD3D11.cpp"
    "${NVCODEC_SRC_FOLDER}/NvEncoder/NvEncoderD3D11.h"
    "${NVCODEC_SRC_FOLDER}/NvEncoder/NvEncoderGL.cpp"
    "${NVCODEC_SRC_FOLDER}/NvEncoder/NvEncoderGL.h"
    "${NVCODEC_SRC_FOLDER}/NvEncoder/NvEncoderOutputInVidMemCuda.cpp"
    "${NVCODEC_SRC_FOLDER}/NvEncoder/NvEncoderOutputInVidMemCuda.h"

    "${NVCODEC_SRC_FOLDER}/../Utils/NvCodecUtils.h"
    "${NVCODEC_SRC_FOLDER}/../Utils/Logger.h"
)

add_library(${PROJECT_NAME} STATIC
    ${NVCODEC_SOURCES}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${NVCODEC_API_FOLDER}
        ${NVCODEC_SRC_FOLDER}
        "${NVCODEC_SRC_FOLDER}/../Utils"
)

if (WIN32)
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        "${NVCODEC_LIB_FOLDER}/x64/nvencodeapi.lib"
        "${NVCODEC_LIB_FOLDER}/x64/nvcuvid.lib"
)
else()
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        "${NVCODEC_LIB_FOLDER}/linux/stubs/x86_64/libnvidia-encode.so"
        "${NVCODEC_LIB_FOLDER}/linux/stubs/x86_64/libnvcuvid.so"
)
endif()
