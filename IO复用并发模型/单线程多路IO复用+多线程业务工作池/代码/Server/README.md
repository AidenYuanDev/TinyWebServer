# CMake_Template

## 前言

CMake复杂项目目录模板，测试使用`google test`
可以对每个模块进行测试。

项目结构如下：

~~~bash
.
├── CMakeLists.txt
├── README.md
├── cmake
│   ├── FindDependencies.cmake
│   └── ProjectSettings.cmake
├── include
│   └── project_name
│       ├── module1.h
│       └── module2.h
├── modules
│   └── modules1
│       ├── CMakeLists.txt
│       ├── include
│       │   └── internal.h
│       ├── src
│       │   └── module1.cpp
│       └── test
│           ├── CMakeLists.txt
│           └── module1_test.cpp
├── src
│   ├── CMakeLists.txt
│   └── main.cpp
└── test
    ├── CMakeLists.txt
    └── test_add.cpp
~~~

## 使用

~~~bash

mkdir build
cd build
cmake ..
cmake --build .
~~~



