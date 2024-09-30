cmake_minimum_required(VERSION 3.15)
project(event_processor)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build architectures for Mac OS X" FORCE)
add_executable(event_processor
    main.cpp
    IEventProcessor.h
    Event.h
    RingBuffer.h
)