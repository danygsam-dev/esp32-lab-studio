cmake_minimum_required(VERSION 3.20)
project(ESP32LabStudio VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(esp32-lab-studio
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.hpp
)

target_link_libraries(esp32-lab-studio PRIVATE Qt6::Widgets)
target_compile_options(esp32-lab-studio PRIVATE -Wall -Wextra -Wpedantic)
