cmake_minimum_required(VERSION 3.11)

project(BeastWebsocket)

if(NOT CMAKE_CXX_FLAGS)
    set(CMAKE_CXX_FLAGS -std=c++14)
endif()

message("CXX=" ${CMAKE_CXX_COMPILER})
message("CXXFLAGS=" ${CMAKE_CXX_FLAGS})

add_subdirectory("${PROJECT_SOURCE_DIR}/examples/ex1_echo_server")
add_subdirectory("${PROJECT_SOURCE_DIR}/examples/ex2_echo_client")
add_subdirectory("${PROJECT_SOURCE_DIR}/examples/ex3_chat_server")
add_subdirectory("${PROJECT_SOURCE_DIR}/examples/ex4_chat_client")
