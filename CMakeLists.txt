cmake_minimum_required(VERSION 2.8)

project(epoll)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

add_executable(${PROJECT_NAME} "main.cpp")
