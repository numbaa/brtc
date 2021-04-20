cmake_minimum_required(VERSION 3.8)
project(brtc)


option(BRTC_BUILD_WITH_EXAMPLES "Build with examples" OFF)
option(BRTC_BUILD_BUILTIN "Build with builtin components" OFF)

set(CMAKE_CXX_STANDARD 20)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(PUBLIC_INCLUDE_DIR "include")
set(SRC_DIR "src")

set(BRTC_PUBLIC_HEADERS
  "${PUBLIC_INCLUDE_DIR}/brtc.h"
  "${PUBLIC_INCLUDE_DIR}/brtc/frame.h"
  "${PUBLIC_INCLUDE_DIR}/brtc/interface.h"
  "${PUBLIC_INCLUDE_DIR}/brtc/builtin.h"
)

include("cmake/funcs.cmake")
include("cmake/core.cmake")
add_subdirectory_with_folder("third_party/glog" "third_party/glog")

add_subdirectory_with_folder("third_party/bco" "third_party/bco")

if (BRTC_BUILD_BUILTIN)
  include("cmake/builtin.cmake")
  add_subdirectory_with_folder("third_party/SDL" "third_party/SDL")
  if (WIN32)
    add_subdirectory_with_folder("third_party/mfx_builder" "third_party/mfx_builder")
  else()
    add_subdirectory_with_folder("third_party/IntelMediaSDK" "third_party/IntelMediaSDK")
  endif()
endif()

if (BRTC_BUILD_WITH_EXAMPLES)
  add_subdirectory(examples)
endif()