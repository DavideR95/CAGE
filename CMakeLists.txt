cmake_minimum_required(VERSION 3.5)

set(CMAKE_CPP_COMPILER icpc)
project(graphlet)


set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3 -march=armv8-a+simd")

#include_directories(/opt/intel/oneapi/vtune/latest/sdk/include)
# link_directories(/opt/intel/oneapi/vtune/latest/sdk/lib32)
#link_directories(/opt/intel/oneapi/vtune/latest/sdk/lib64)


add_subdirectory(third_party/abseil-cpp)
#add_subdirectory(third_party/gflags)

add_library(
  lib
  permute/permute.cpp
  util/cuckoo.cpp
  util/fastio.cpp
  util/graph.cpp
  util/binary_search.cpp
  util/dynarray.cpp
)


target_link_libraries(lib PUBLIC #[[gflags::gflags]] absl::strings absl::memory absl::span)
target_include_directories(lib PUBLIC .)


add_executable(main main.cpp)
target_link_libraries(main lib)

add_executable(alg3 alg3.cpp)
target_link_libraries(alg3 lib)

add_executable(baseline_en main_enhanced.cpp)
target_link_libraries(baseline_en lib)

add_executable(baseline_delete main_deletion.cpp)
target_link_libraries(baseline_delete lib)

add_executable(baseline_ultra main_ultra.cpp)
target_link_libraries(baseline_ultra lib)

