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

add_executable(
    SimpleLogServer
    main.cpp
    simple_log_server.cpp
    simple_log_server.hpp
)

target_include_directories(SimpleLogServer PRIVATE ${securitysocket_include_dir})
target_link_libraries(SimpleLogServer PRIVATE securitysocket)
set_property(TARGET SimpleLogServer PROPERTY CXX_STANDARD 17)
set_property(TARGET SimpleLogServer PROPERTY CXX_STANDARD_REQUIRED ON)