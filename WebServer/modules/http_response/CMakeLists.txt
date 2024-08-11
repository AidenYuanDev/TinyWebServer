# 添加静态库
add_library(http_response STATIC)

target_sources(http_response
    PRIVATE
        src/http_response.cpp
    PUBLIC
        include/http_response.h
)

target_include_directories(http_response
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(http_response
    PRIVATE
    http_types
)

# 测试
# add_subdirectory(test)
