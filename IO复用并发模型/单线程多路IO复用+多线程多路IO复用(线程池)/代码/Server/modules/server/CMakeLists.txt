# 添加静态库
add_library(server STATIC)

target_sources(server
    PRIVATE
        src/server.cpp
    PUBLIC
        include/server.h
)

target_include_directories(server
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(server
    PRIVATE
    thread_pool
    client_context
)

# 测试
# add_subdirectory(test)
