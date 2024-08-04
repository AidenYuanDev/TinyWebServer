# 添加静态库
add_library(thread_pool STATIC)

target_sources(thread_pool
    PRIVATE
        src/thread_pool.cpp
    PUBLIC
        include/thread_pool.h
)

target_include_directories(thread_pool
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(thread_pool
    PRIVATE
    logger
    config_manager
    yaml-cpp
)
# 测试
add_subdirectory(test)
