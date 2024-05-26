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
  - .[日志系统](#日志系统)
  - .[配置系统](#配置系统)
  - .[线程库和锁](#线程库和锁)
  - .[协程库](#协程库)
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
|    |—— singleton.h(获取线程模版)
|    |—— sylar.h(总头文件)
|    |—— thread.cc(线程源文件)
|    |—— thread.h(线程头文件)
|    |—— util.cc(自定义工具)
|    └—— util.h
|—— tests
|    |—— test.cc(日志测试)
|    |—— test_config.cc(配置系统测试)
|    |—— test_fiber.cc(协程测试)
|    |—— test_scheduler.cc(协程调度器测试)
|    |—— test_thread.cc(线程测试)
|    └—— test_util.cc(断言测试)
|—— other *.txt(测试日志模块的输出文件）
|—— CMakeCache.txt
|—— CMakeLists.txt(cmake配置)
└—— Makefile(make配置)
```
### 日志系统及配置系统类图
   ![zxh-服务器框架类图](https://github.com/22722679/High-performance-server-zxh-sylar/assets/116802544/16e25211-8566-4bf8-b7f8-ac53a3dc9f93)

### 日志系统
Log4j格式采用类似C语言中的printf函数的打印格式格式化日志信息，如下：
  ![image](https://github.com/22722679/High-performance-server-zxh-sylar/assets/116802544/c03f42b3-eea5-4a52-9744-23fc85fd388c)
程序中实现LogEvent可以将自己本身写进logger里，由于智能指针的问题无法使用流式调用，要用鞋狗的方法将缓存的字符串输出，抽象了一个Wrap，去写自己本身，析构时会自动写入。

### 配置系统
**规则：约定优于配置。序列化过程实现思路：**
```
template<T,  FromStr,  ToStr>
class  ConfigVar;
template<F ,  T,  >
LexicalCast;
//容器片特化，目前支持vector, list,set  ,  map  ,unordered_map ,   unordered_set
map/unordered_map     支持key   =  std::string
Config::Lookup(key),   key相同，类型不同的，不会有报错。这个需要处理
实现后就可以支持Config解析自定义类型，自定义类型可以和常规stl容器一起使用
```
**配置的事件机制：当一个配置项发生修改的时候，可以反向通知对应的代码，回调**


### 线程库和锁
### 协程库












