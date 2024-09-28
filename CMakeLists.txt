cmake_minimum_required(VERSION 3.15)
project(event_processor)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build architectures for Mac OS X" FORCE)
add_executable(event_processor
    main.cpp
)