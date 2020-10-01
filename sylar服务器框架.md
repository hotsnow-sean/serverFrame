# sylar （原作者名字）

## 开发环境
Windows10 gcc8.1 cmake

## 项目路径
bin -- 二进制
build -- 中间文件路径
cmake -- cmake函数文件
CMakeLists.txt -- cmake的定义文件
lib -- 库输出路径
Makefile
sylar -- 源代码路径
tests -- 测试代码

## 日志系统
1） Log4J

    Logger（定义日志类别）
        |
        |-------Formatter（日志格式）
        |
    Appender（日志输出地方）

## 配置系统

Config --> Json

配置系统的原则，约定优于配置

自定义类型需要实现两个函数，用于序列化与反序列化，进而结合配置文件
```c++
void to_json(json& j, const person& p) {
    j = json{{"name", p.name}, {"address", p.address}, {"age", p.age}};
}

void from_json(const json& j, person& p) {
    j.at("name").get_to(p.name);
    j.at("address").get_to(p.address);
    j.at("age").get_to(p.age);
}
```

配置的事件机制

## 日志系统整合配置系统

```yaml
logs:
    - name: root
      level: (debug,info,warn,error,fatal)
      formatter: "%d%T%p%T%t%m%n"
      appender:
        - type: (StdoutLogAppender, FileLogAppender)
          level: (...)
          file: /logs/xxx.log
```

## 协程库封装

## socket函数库

## http协议开发

## 分布协议

## 推荐系统