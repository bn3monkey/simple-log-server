cmake_minimum_required (VERSION 3.16)
project("SimpleLogServer")

include(FetchContent)

set(BUILD_SECURITYSOCKET_SHARED OFF CACHE BOOL "Build Security Socket Static Libraries" FORCE)
FetchContent_Declare(SecuritySocket
    GIT_REPOSITORY https://github.com/bn3monkey/securitysocket
    GIT_TAG v2.1.1)
FetchContent_MakeAvailable(SecuritySocket)
set(securitysocket_include_dir ${securitysocket_SOURCE_DIR}/src)
message("Security Socket Include Directory : ${securitysocket_include_dir}")

set(SIMPLE_LOG_SERVER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")


set(SIMPLE_LOG_SERVER_PROTOCOL_DIR "${SIMPLE_LOG_SERVER_SOURCE_DIR}/protocol")
file(GLOB_RECURSE SIMPLE_LOG_SERVER_PROTOCOL_SOURCES 
    "${SIMPLE_LOG_SERVER_PROTOCOL_DIR}/*.cpp" 
    "${SIMPLE_LOG_SERVER_PROTOCOL_DIR}/*.c" 
    "${SIMPLE_LOG_SERVER_PROTOCOL_DIR}/*.hpp" 
    "${SIMPLE_LOG_SERVER_PROTOCOL_DIR}/*.h")
set(SIPPLE_LOG_SERVER_SERVER_SOURCE_DIR "${SIMPLE_LOG_SERVER_SOURCE_DIR}/server")
file(GLOB_RECURSE SIMPLE_LOG_SERVER_SERVER_SOURCES 
    "${SIPPLE_LOG_SERVER_SERVER_SOURCE_DIR}/*.cpp" 
    "${SIPPLE_LOG_SERVER_SERVER_SOURCE_DIR}/*.c" 
    "${SIPPLE_LOG_SERVER_SERVER_SOURCE_DIR}/*.hpp" 
    "${SIPPLE_LOG_SERVER_SERVER_SOURCE_DIR}/*.h")
set(SIPPLE_LOG_SERVER_CLIENT_SOURCE_DIR "${SIMPLE_LOG_SERVER_SOURCE_DIR}/client")
file(GLOB_RECURSE SIMPLE_LOG_SERVER_CLIENT_SOURCES 
    "${SIPPLE_LOG_SERVER_CLIENT_SOURCE_DIR}/*.cpp" 
    "${SIPPLE_LOG_SERVER_CLIENT_SOURCE_DIR}/*.c" 
    "${SIPPLE_LOG_SERVER_CLIENT_SOURCE_DIR}/*.hpp" 
    "${SIPPLE_LOG_SERVER_CLIENT_SOURCE_DIR}/*.h")

add_executable(
    SimpleLogServer
    app/main.cpp
    ${SIMPLE_LOG_SERVER_PROTOCOL_SOURCES}
    ${SIMPLE_LOG_SERVER_SERVER_SOURCES}
)

target_include_directories(SimpleLogServer PRIVATE ${securitysocket_include_dir} ${SIMPLE_LOG_SERVER_PROTOCOL_DIR} ${SIPPLE_LOG_SERVER_SERVER_SOURCE_DIR})
target_link_libraries(SimpleLogServer PRIVATE securitysocket)
set_property(TARGET SimpleLogServer PROPERTY CXX_STANDARD 17)
set_property(TARGET SimpleLogServer PROPERTY CXX_STANDARD_REQUIRED ON)