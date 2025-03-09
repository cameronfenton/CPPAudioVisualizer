cmake_minimum_required(VERSION 3.10)
project(AudioVisualizer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(AudioVisualizer 
    AudioVisualizer.cpp
    AudioCapture.cpp
)

target_link_libraries(AudioVisualizer
    ole32
    winmm
)