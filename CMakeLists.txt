cmake_minimum_required(VERSION 3.16)
project(CPU_Visualizer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(cpu_visualizer
    main.cpp
    QtMainWindow.cpp
    CPU.cpp
    QtMainWindow.h
    CPU.h CPU.cpp
)

target_include_directories(cpu_visualizer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(cpu_visualizer PRIVATE Qt6::Widgets)
