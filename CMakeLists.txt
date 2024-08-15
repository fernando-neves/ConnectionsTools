cmake_minimum_required (VERSION 3.10.2)

project(ConnectionsToolsBoost)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/../debug")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/../debug")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/../debug")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/../release")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/../release")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/../release")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/../relwithdebinfo")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/../relwithdebinfo")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/../relwithdebinfo")

if(MSVC)
else()
    add_compile_options(-Wall)
    add_compile_options(-Wextra)
endif()

find_package(Threads REQUIRED)
link_libraries(Threads::Threads)

include_directories(submodules/asio/asio/include)
include_directories(submodules/plog/include)
include_directories(submodules/jsoncpp/include)

option(DEBUGGING_MESSAGES_ON_UPSTREAM "Enable debugging messages for the upstream module" OFF)

if(DEBUGGING_MESSAGES_ON_UPSTREAM)
    add_definitions(-DDEBUGGING_MESSAGES_ON_UPSTREAM)
endif()

set(JSONCPP_WITH_TESTS OFF CACHE BOOL "Compile and (for jsoncpp_check) run JsonCpp test executables")
set(JSONCPP_WITH_EXAMPLE OFF CACHE BOOL "Compile JsonCpp example")

add_subdirectory(submodules/jsoncpp)

add_subdirectory(tools/tcp_echo_server)
add_subdirectory(tools/tcp_echo_client)

add_subdirectory(tools/udp_echo_server)
add_subdirectory(tools/udp_echo_client)