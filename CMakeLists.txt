cmake_minimum_required(VERSION 2.0)

project(thread-pool)

include_directories(inc)

add_executable(
    thread-pool 
    src/Main.cpp
    src/TcpSocket.cpp
    src/ThreadPoolManager.cpp
)

target_link_libraries(thread-pool pthread)
