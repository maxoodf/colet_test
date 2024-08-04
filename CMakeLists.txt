cmake_minimum_required(VERSION 3.28)
project(test)

set(CMAKE_CXX_STANDARD 20)

find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED)

add_executable(test main.cpp)
target_link_libraries(test
        boost::boost
        openssl::openssl
)
