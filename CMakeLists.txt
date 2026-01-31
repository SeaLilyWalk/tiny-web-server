cmake_minimum_required(VERSION 3.5)
project(web_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Release)

find_package(OpenSSL REQUIRED)

set(SOURCE_FILES
    src/connection.cc
    src/server.cc
    src/main.cc
)

add_executable(web_server ${SOURCE_FILES})

target_link_libraries(web_server
    OpenSSL::SSL
    OpenSSL::Crypto
    pthread
)
