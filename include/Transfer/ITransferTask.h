#pragma once
#include<string>
// ITransferTask：负责“收集哪些文件需要转移”
// 并委托给 ITransferWriter 处理
class ITransferTask {
public:
    virtual ~ITransferTask() = default;

    // 添加待转移项
    virtual void add(const std::string& fileName) = 0;
    virtual void add(int fileDescriptor)    = 0;

    // 执行转移：把所有待转移项“写入”
    virtual void execute() = 0;
};
