# 列出所有模块
set(MODULES
    static_file_controller 
    # 添加其他模块...
)

# 遍历所有模块并添加它们
foreach(MODULE ${MODULES})
    add_subdirectory(${MODULE})
endforeach()
