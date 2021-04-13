project(send_stream)

add_executable(${PROJECT_NAME}
  "main.cpp"
)

target_link_libraries(${PROJECT_NAME}
  brtc
  bco
  glog::glog
)