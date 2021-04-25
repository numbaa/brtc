add_subdirectory_with_folder("glog" "third_party/glog")

add_subdirectory_with_folder("bco" "third_party/bco")

if (BRTC_BUILD_BUILTIN)
  add_subdirectory_with_folder("SDL" "third_party/SDL")
  if (WIN32)
    add_subdirectory_with_folder("mfx_builder" "third_party/IntelMediaSDK" )
  else()
    add_subdirectory_with_folder("IntelMediaSDK" "third_party/IntelMediaSDK" )
  endif()

  if (BRTC_BUILD_NVCODEC)
    add_subdirectory_with_folder("nvcodec_builder" "third_party/NvCodecSDK" )
  endif()
endif()