cmake_minimum_required(VERSION 3.5)

project(minemapnik LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -DHAVE_CAIRO=1)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -DHAVE_CAIRO=1)

include_directories(/usr/local/include/cairo)
add_executable(minemapnik main.cpp argparse.hpp)
#target_include_directories(minemapnik /usr/local/include)
target_link_libraries(minemapnik /usr/local/lib/libmapnik.so /usr/lib/libicuuc.so.67 /usr/local/lib/libcairo.so)
