# 添加静态库
add_library(static_file_controller STATIC)

target_sources(static_file_controller
    PRIVATE
        src/static_file_controller.cpp
    PUBLIC
        include/static_file_controller.h
)

target_include_directories(static_file_controller
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${YAML_CPP_INCLUDE_DIRS}
)

target_link_libraries(static_file_controller
    PRIVATE
    http_types
    http_parser
    http_request 
    http_response
)

# 测试
# add_subdirectory(test)
