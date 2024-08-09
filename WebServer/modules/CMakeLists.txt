# 列出所有模块
set(MODULES
    thread_pool
    client_context
    server
    http_parser
    http_response
    logger
    config_manager
    # 添加其他模块...
)

# 遍历所有模块并添加它们
foreach(MODULE ${MODULES})
    add_subdirectory(${MODULE})
endforeach()
