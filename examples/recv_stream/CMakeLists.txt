project(recv_stream)

add_executable(${PROJECT_NAME}
  "main.cpp"
)

target_link_libraries(${PROJECT_NAME}
  bco
  brtc
  glog::glog
)