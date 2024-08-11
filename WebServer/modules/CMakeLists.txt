# 列出所有模块
set(MODULES
    thread_pool
    message_queue 
    server
    http_parser
    http_response
    http_request
    http_types
    logger
    config_manager
    app
    route
    router
    controller
    # 添加其他模块...
)

# 遍历所有模块并添加它们
foreach(MODULE ${MODULES})
    add_subdirectory(${MODULE})
endforeach()
