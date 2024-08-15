cmake_minimum_required (VERSION 3.10.2)

project(udp_echo_server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
else()
    add_compile_options(-Wall)
    add_compile_options(-Wextra)
endif()

find_package(Threads REQUIRED)
link_libraries(Threads::Threads)

include_directories(../../submodules/asio/asio/include)
include_directories(../../submodules/plog/include)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE jsoncpp_lib)