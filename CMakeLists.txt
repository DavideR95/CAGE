cmake_minimum_required(VERSION 3.5)

set(CMAKE_CPP_COMPILER icpc) # this may not always work, use export CXX = icpx or icpc 
project(graphlet)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3 ")

# Uncomment these directives if you plan to use vtune
#include_directories(/opt/intel/oneapi/vtune/latest/sdk/include)
#link_directories(/opt/intel/oneapi/vtune/latest/sdk/lib64)


add_subdirectory(third_party/abseil-cpp)

add_library(
  lib
  permute/permute.cpp
  util/cuckoo.cpp
  util/fastio.cpp
  util/graph.cpp
  util/binary_search.cpp
  util/dynarray.cpp
)


target_link_libraries(lib PUBLIC absl::strings absl::memory absl::span)
target_include_directories(lib PUBLIC .)

add_executable(CAGE main.cpp)
target_link_libraries(CAGE lib) # include also libittnotify.a dl pthread to use with vtune
