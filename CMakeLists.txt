cmake_minimum_required(VERSION 3.28)

project(test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(Boost REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME}
        PRIVATE
        include/HTTPClient.h
        include/WebsocketClient.h
)

target_sources(${PROJECT_NAME}
        PRIVATE
        src/main.cpp
        src/HTTPClient.cpp
        src/WebsocketClient.cpp
)

target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(test
        boost::boost
        openssl::openssl
)
