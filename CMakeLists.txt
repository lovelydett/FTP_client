cmake_minimum_required(VERSION 3.5.1)
project( ftp_client )


SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11  -O0 -g")

set(PROJECT_DIR ./)

link_directories (${PROJECT_DIR}/lib) 

 add_executable(ftp_client
 src/main.c
 )

