cmake_minimum_required(VERSION 3.8)
project(brtc)

set(CMAKE_CXX_STANDARD 20)

add_executable(${PROJECT_NAME}
  src/main.cpp
  src/packetizer.h
  src/packetizer.cpp
  src/rtp.h
  src/transport.h
  src/transport.cpp
  src/frame.h
)

# target_include_directories(${PROJECT_NAME}
#   PUBLIC
#     "third_party/asio/asio/include"
# )