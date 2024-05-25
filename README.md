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
|—— lib
|—— sylar(源代码文件)
|    |—— config.cc(配置源文件)
|    |—— config.h(配置头文件)
|    |—— fiber.cc(协程源文件)
|    |—— fiber.h(协程头文件)
|    |—— log.cc(日志源文件)
|    |—— log.h(日志头文件)
|    |—— macro.h(断言定义)
|    |—— scheduler.cc(协程调度器源文件)
|    |—— scheduler.h(协程调度器头文件)
|    |—— singleton.h()
|    |—— sylar.h(总头文件)
|    |—— thread.cc(线程源文件)
|    |—— thread.h(线程头文件)
|    |—— util.cc(自定义工具)
|    └—— util.h
```


### 日志系统
### 配置系统
### 线程库和锁
### 协程库












