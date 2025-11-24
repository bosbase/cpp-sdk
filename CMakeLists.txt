cmake_minimum_required(VERSION 3.16)
project(bosbase-cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(bosbase
    src/client.cpp
    src/auth_store.cpp
    src/base_service.cpp
    src/record_service.cpp
    src/realtime.cpp
    src/pubsub.cpp
)

find_package(CURL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(OpenSSL REQUIRED)

target_include_directories(bosbase
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/websocketpp
)

target_link_libraries(bosbase
    PUBLIC
        CURL::libcurl
        Boost::system
        Boost::thread
        OpenSSL::SSL
        OpenSSL::Crypto
)

target_include_directories(bosbase PRIVATE ${Boost_INCLUDE_DIRS})

target_compile_definitions(bosbase PUBLIC BOSBASE_CPP_SDK)

install(TARGETS bosbase EXPORT bosbaseTargets)
install(DIRECTORY include/ DESTINATION include)
