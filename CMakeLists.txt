cmake_minimum_required(VERSION 3.11)

project(BeastWebsocket)

if(NOT CMAKE_CXX_FLAGS)
    set(CMAKE_CXX_FLAGS -std=c++14)
endif()

message("CXX=" ${CMAKE_CXX_COMPILER})
message("CXXFLAGS=" ${CMAKE_CXX_FLAGS})

set(BEAST_WEBSOCKET_HEADERS
	${PROJECT_SOURCE_DIR}/include/server.hpp
	${PROJECT_SOURCE_DIR}/include/client.hpp
	${PROJECT_SOURCE_DIR}/include/base.hpp
	${PROJECT_SOURCE_DIR}/include/session.hpp
	PARENT_SCOPE)

set(BEAST_WEBSOCKET_INCLUDE_DIR
	${PROJECT_SOURCE_DIR}/include
	${PROJECT_SOURCE_DIR}/extern
	PARENT_SCOPE)

add_subdirectory("${PROJECT_SOURCE_DIR}/examples/ex1_echo_server")
add_subdirectory("${PROJECT_SOURCE_DIR}/examples/ex2_echo_client")
add_subdirectory("${PROJECT_SOURCE_DIR}/examples/ex3_chat_server")
add_subdirectory("${PROJECT_SOURCE_DIR}/examples/ex4_chat_client")
