cmake_minimum_required(VERSION 3.10)
project(server)

set(SOURCES
    main.cpp
    server.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ../include)