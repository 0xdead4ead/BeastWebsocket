cmake_minimum_required(VERSION 3.11)

find_package(Boost 1.66 COMPONENTS system thread regex)

set(OUTPUT_NAME ex4_chat_client)

#add_subdirectory("${PROJECT_SOURCE_DIR}/examples/ex4_chat_client/input_console")

include_directories("${PROJECT_SOURCE_DIR}/extern")
include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories(${Boost_INCLUDE_DIRS})
set(SOURCES
    ex4_chat_client.cpp)
set(HEADERS
	${BEAST_WEBSOCKET_HEADERS}
    ${PROJECT_SOURCE_DIR}/examples/chat_message.hpp)

add_executable(${OUTPUT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${OUTPUT_NAME} Boost::system Boost::thread Boost::regex pthread icui18n)

