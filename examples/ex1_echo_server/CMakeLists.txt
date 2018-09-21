cmake_minimum_required(VERSION 3.11)

find_package(Boost 1.66 COMPONENTS system thread regex)

set(OUTPUT_NAME ex1_echo_server)

include_directories("${PROJECT_SOURCE_DIR}/extern")
include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories(${Boost_INCLUDE_DIRS})
set(SOURCES
    ex1_echo_server.cpp)
set(HEADERS
    ${PROJECT_SOURCE_DIR}/include/server.hpp
    ${PROJECT_SOURCE_DIR}/include/client.hpp
    ${PROJECT_SOURCE_DIR}/include/base.hpp
    ${PROJECT_SOURCE_DIR}/include/session.hpp)

add_executable(${OUTPUT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${OUTPUT_NAME} Boost::system Boost::thread Boost::regex pthread icui18n)
