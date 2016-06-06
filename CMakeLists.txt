cmake_minimum_required(VERSION 3.2.0)

project(C3Blog)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")

set(CMAKE_CXX_STANDARD 11)

set(Boost_USE_MULTITHREADED ON)

find_package(Boost 1.54.0 COMPONENTS system thread REQUIRED)
find_package(LevelDB REQUIRED)
find_package(Threads)
find_package(PkgConfig)

pkg_search_module(YamlCPP REQUIRED yaml-cpp)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${LevelDB_INCLUDE})
include_directories(${YamlCPP_INCLUDE})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories("${C3Blog_SOURCE_DIR}/src")
include_directories(SYSTEM "${C3Blog_SOURCE_DIR}/include")

file(GLOB_RECURSE SRC_LIST src/*.cpp)
file(GLOB_RECURSE HDR_LIST src/*.h)

message(STATUS "Source directory: ${C3Blog_SOURCE_DIR}")
message(STATUS "Binary directory: ${C3Blog_BINARY_DIR}")

add_executable(c3_blog ${SRC_LIST} ${HDR_LIST})

target_link_libraries(c3_blog ${Boost_LIBRARIES})
target_link_libraries(c3_blog ${LevelDB_LIBRARIES})
target_link_libraries(c3_blog ${YamlCPP_LIBRARIES})
target_link_libraries(c3_blog ${CMAKE_THREAD_LIBS_INIT})
