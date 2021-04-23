project(send_stream)

add_executable(${PROJECT_NAME}
  $<TARGET_OBJECTS:brtc_examples_common>
  "main.cpp"
)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "examples")
target_compile_definitions(${PROJECT_NAME} PUBLIC
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

target_link_libraries(${PROJECT_NAME}
  brtc::brtc
  brtc::builtin
  bco
  glog::glog
  d3d11.lib
  dxgi.lib
)

set_target_properties(${PROJECT_NAME}
  PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${BRTC_OUTPUT_DIR}
)