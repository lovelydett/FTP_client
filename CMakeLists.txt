cmake_minimum_required(VERSION 3.5.1)
project( ftp_client )


SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread -O0 -g")

set(PROJECT_DIR ./)
find_package(Threads REQUIRED)

 add_executable(ftp_client
 src/main.cpp
 )

