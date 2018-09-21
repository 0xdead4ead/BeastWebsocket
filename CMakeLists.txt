cmake_minimum_required(VERSION 3.11)

project(beast_ws_server)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED on)

add_subdirectory("${PROJECT_SOURCE_DIR}/examples/ex1_echo_server")
add_subdirectory("${PROJECT_SOURCE_DIR}/examples/ex2_echo_client")
