project(recv_stream)

add_executable(${PROJECT_NAME}
  "main.cpp"
)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "examples")
target_compile_definitions(${PROJECT_NAME} PUBLIC
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)
target_link_libraries(${PROJECT_NAME}
  bco
  brtc::brtc
  brtc::builtin
  glog::glog
)

set_target_properties(${PROJECT_NAME}
  PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${BRTC_OUTPUT_DIR}
)