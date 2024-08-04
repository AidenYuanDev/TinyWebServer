# 添加静态库
add_library(logger STATIC)

target_sources(logger
    PRIVATE
        ./src/logger.cpp 
    PUBLIC
        ./include/logger.h
)

target_include_directories(logger
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(logger
    PRIVATE
    config_manager
    yaml-cpp
)

# 测试
# add_subdirectory(test)
