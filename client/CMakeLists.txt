cmake_minimum_required(VERSION 3.10)
project(client)

set(SOURCES
    main.cpp
    client.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})