# 高性能服务器框架Project

## 开发环境
  - linux Centos7
  - gcc 9.1
  - Cmake

## 项目路径
  - bin(二进制)
  - build(中间文件路径)
  - cmake(cmake函数文件夹)
  - CmakeLists.txt(Cmake定义文件)
  - lib(库的输出路径)
  - Makefile(编码规范)
  - sylar(源代码路径)
  - tests(测试代码路径)
  - other *.txt(测试日志模块的输出文件）

## 目前开发进度
  - .[日志系统]日志系统
  - .[配置系统]配置系统
  - .[线程库和锁]线程库和锁
  - .[协程库]协程库
  -   ....

### 文件目录
```C++
|—— CMakeFiles
|—— bin (test的二进制文件)
|   |—— config (配置系统测试)
|   |    |—— log.yml
|   |    |—— log2.yml
|   |    └—— test.yml
|   |—— test
|   |—— test_config(配置)
|   |—— test_fiber(协程)
|   |—— test_scheduler(协程调度器)
|   |—— test_thread(线程)
|   └—— test_util(工具)
|—— cmake
|    └—— utils.cmake(重新自定义的宏文件)
|—— include
|












