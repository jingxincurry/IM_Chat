cmake_minimum_required(VERSION 3.16)

project(im_server VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(im_server_reactor
    src/Reactor.cpp
    src/ReactorImplementation.cpp
    src/EpollDemultiplexer.cpp
    src/SelectDemultiplexer.cpp
    chatserver.cpp
    chatserver.h
    filetransfer.h
    protocol.h
    main.cpp
)

target_include_directories(im_server_reactor PRIVATE include)
