# 添加静态库
add_library(client_context STATIC)

target_sources(client_context
    PRIVATE
        ./src/client_context.cpp 
    PUBLIC
        ./include/client_context.h
)

target_include_directories(client_context
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 测试
# add_subdirectory(test)
