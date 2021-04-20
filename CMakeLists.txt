cmake_minimum_required(VERSION 3.8)
project(brtc)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
include("cmake/funcs.cmake")

option(BRTC_BUILD_WITH_EXAMPLES "Build with examples" OFF)
option(BRTC_BUILD_BUILTIN "Build with builtin components" OFF)

set(CMAKE_CXX_STANDARD 20)
set(PUBLIC_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
set(BRTC_OUTPUT_DIR ${CMAKE_BINARY_DIR})
set(BRTC_PUBLIC_HEADERS
  "${PUBLIC_INCLUDE_DIR}/brtc.h"
  "${PUBLIC_INCLUDE_DIR}/brtc/frame.h"
  "${PUBLIC_INCLUDE_DIR}/brtc/interface.h"
  "${PUBLIC_INCLUDE_DIR}/brtc/builtin.h"
)

add_subdirectory(${SRC_DIR})
add_subdirectory(${THIRD_PARTY_DIR})

if (BRTC_BUILD_BUILTIN)
  add_subdirectory(${SRC_DIR}/builtin)
endif()

if (BRTC_BUILD_WITH_EXAMPLES)
  add_subdirectory(examples)
endif()