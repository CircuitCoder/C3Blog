cmake_minimum_required(VERSION 3.2.0)

project(C3Blog)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -Wextra")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(Boost_USE_MULTITHREADED ON)

add_definitions(-DRAPIDJSON_HAS_STDSTRING=1)

find_package(CURL REQUIRED)
find_package(Boost 1.54.0 COMPONENTS system thread filesystem program_options REQUIRED)
find_package(LevelDB REQUIRED)
find_package(Threads REQUIRED)
find_package(Discount REQUIRED)
find_package(RapidJSON REQUIRED)
find_package(PkgConfig)

pkg_search_module(YamlCPP REQUIRED yaml-cpp)
pkg_search_module(TinyXML2 REQUIRED tinyxml2)

include_directories(${CURL_INCLUDE})
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${LevelDB_INCLUDE})
include_directories(${YamlCPP_INCLUDE})
include_directories(${Discount_INCLUDE})
include_directories(${TinyXML2_INCLUDE})
include_directories(${RapidJSON_INCLUDE})

include_directories("${C3Blog_SOURCE_DIR}/src")
include_directories(SYSTEM "${C3Blog_SOURCE_DIR}/include")

file(GLOB_RECURSE SRC_LIST src/*.cpp)
file(GLOB_RECURSE HDR_LIST src/*.h)

message(STATUS "Source directory: ${C3Blog_SOURCE_DIR}")
message(STATUS "Binary directory: ${C3Blog_BINARY_DIR}")

add_executable(c3_blog ${SRC_LIST} ${HDR_LIST})

target_link_libraries(c3_blog ${CURL_LIBRARIES})
target_link_libraries(c3_blog ${Boost_LIBRARIES})
target_link_libraries(c3_blog ${LevelDB_LIBRARIES})
target_link_libraries(c3_blog ${YamlCPP_LIBRARIES})
target_link_libraries(c3_blog ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(c3_blog ${Discount_LIBRARIES})
target_link_libraries(c3_blog ${TinyXML2_LIBRARIES})
target_link_libraries(c3_blog ${RapidJSON_LIBRARIES})
