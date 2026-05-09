cmake_minimum_required(VERSION 3.16)

project(IM_Chat LANGUAGES CXX)

option(BUILD_CLIENT "Build the Qt client" ON)
option(BUILD_SERVER "Build the epoll server" ON)
option(BUILD_SERVER_REACTOR "Build the reactor-based server" OFF)

if(BUILD_CLIENT)
    add_subdirectory(Client)
endif()

if(BUILD_SERVER)
    add_subdirectory(Server)
endif()

if(BUILD_SERVER_REACTOR)
    add_subdirectory(Server_Reactor)
endif()
