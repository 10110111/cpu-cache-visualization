cmake_minimum_required(VERSION 3.10.0)
project(cpu-cache-visualization VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 5.9 REQUIRED Core Gui)

add_executable(cpu-cache-visualization visual.cpp)
target_link_libraries(cpu-cache-visualization Qt5::Core Qt5::Gui)
