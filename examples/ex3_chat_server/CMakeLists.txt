cmake_minimum_required(VERSION 3.11)

find_package(Boost 1.66 COMPONENTS system thread regex)

set(OUTPUT_NAME ex3_chat_server)

include_directories("${PROJECT_SOURCE_DIR}/extern")
include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories(${Boost_INCLUDE_DIRS})
set(SOURCES
    ex3_chat_server.cpp)
set(HEADERS
	${BEAST_WEBSOCKET_HEADERS}
    ${PROJECT_SOURCE_DIR}/examples/chat_message.hpp)

add_executable(${OUTPUT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${OUTPUT_NAME} Boost::system Boost::thread Boost::regex pthread icui18n)
