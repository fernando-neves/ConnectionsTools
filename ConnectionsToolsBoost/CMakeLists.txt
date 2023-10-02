cmake_minimum_required (VERSION 3.12)

project(ConnectionsToolsBoost)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../bin")

if(MSVC)
else()
    add_compile_options(-Wall)
    add_compile_options(-Wextra)
endif()

find_package(Threads REQUIRED)
link_libraries(Threads::Threads)

include_directories(submodules/asio/asio/include)
include_directories(submodules/plog/include)

add_subdirectory(tools/tcp_echo_server)
add_subdirectory(tools/tcp_echo_client)

add_subdirectory(tools/udp_echo_server)
add_subdirectory(tools/udp_echo_client)