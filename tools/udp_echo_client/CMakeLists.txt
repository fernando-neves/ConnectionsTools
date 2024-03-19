cmake_minimum_required (VERSION 3.10.2)

project(udp_echo_client)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/../bin/tools/debug/${PROJECT_NAME}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/../bin/tools/release/${PROJECT_NAME}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/../bin/tools/relwithdebinfo/${PROJECT_NAME}")

if(MSVC)
else()
    add_compile_options(-Wall)
    add_compile_options(-Wextra)
endif()

find_package(Threads REQUIRED)
link_libraries(Threads::Threads)

include_directories(../../submodules/asio/asio/include)
include_directories(../../submodules/plog/include)

add_executable(${PROJECT_NAME}
    main.cpp)
