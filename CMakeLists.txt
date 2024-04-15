cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

project(UdpClientServer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost COMPONENTS system thread atomic program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

add_subdirectory(client)
add_subdirectory(server)
