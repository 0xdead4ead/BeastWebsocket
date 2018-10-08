cmake_minimum_required(VERSION 3.11)

project(beast_ws_server)

set(CMAKE_CXX_STANDARD_REQUIRED on)

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 14)
endif()

message(${CMAKE_CXX_STANDARD})

add_subdirectory("${PROJECT_SOURCE_DIR}/examples/ex1_echo_server")
add_subdirectory("${PROJECT_SOURCE_DIR}/examples/ex2_echo_client")
add_subdirectory("${PROJECT_SOURCE_DIR}/examples/ex3_chat_server")
add_subdirectory("${PROJECT_SOURCE_DIR}/examples/ex4_chat_client")
