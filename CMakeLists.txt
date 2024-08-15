cmake_minimum_required (VERSION 3.10.2)

project(ConnectionsToolsBoost)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/../bin/debug")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/../bin/debug")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/../bin/debug")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/../bin/release")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/../bin/release")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/../bin/release")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/../bin/relwithdebinfo")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/../bin/relwithdebinfo")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/../bin/relwithdebinfo")

if(MSVC)
else()
    add_compile_options(-Wall)
    add_compile_options(-Wextra)
endif()

find_package(Threads REQUIRED)
link_libraries(Threads::Threads)

include_directories(submodules/asio/asio/include)
include_directories(submodules/plog/include)
include_directories(submodules/json/include)

# Configurações do jsoncpp
set(JSONCPP_WITH_TESTS OFF CACHE BOOL "Compile and (for jsoncpp_check) run JsonCpp test executables")
set(JSONCPP_WITH_POST_BUILD_UNITTEST OFF CACHE BOOL "Automatically run unit-tests as a post build step")
set(JSONCPP_WITH_WARNING_AS_ERROR OFF CACHE BOOL "Force compilation to fail if a warning occurs")
set(JSONCPP_WITH_STRICT_ISO ON CACHE BOOL "Issue all the warnings demanded by strict ISO C and ISO C++")
set(JSONCPP_WITH_PKGCONFIG_SUPPORT ON CACHE BOOL "Generate and install .pc files")
set(JSONCPP_WITH_CMAKE_PACKAGE ON CACHE BOOL "Generate and install cmake package files")
set(JSONCPP_WITH_EXAMPLE OFF CACHE BOOL "Compile JsonCpp example")
set(JSONCPP_STATIC_WINDOWS_RUNTIME OFF CACHE BOOL "Use static (MT/MTd) Windows runtime")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build jsoncpp_lib as a shared library.")
set(BUILD_STATIC_LIBS ON CACHE BOOL "Build jsoncpp_lib as a static library.")
set(BUILD_OBJECT_LIBS OFF CACHE BOOL "Build jsoncpp_lib as a object library.")

add_subdirectory(submodules/jsoncpp jsoncpp_build)

add_subdirectory(tools/tcp_echo_server)
add_subdirectory(tools/tcp_echo_client)

add_subdirectory(tools/udp_echo_server)
add_subdirectory(tools/udp_echo_client)