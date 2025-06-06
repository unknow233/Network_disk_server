// ITransferWriter：负责“如何写入”
// 比如写到文本文件、写到消息队列、写到数据库……
#pragma once
#include  <string>
class ITransferWriter {
public:
    virtual ~ITransferWriter() = default;

    // 把单个转移单元提交给下游（文件名或文件描述符）
    virtual void write(const std::string& fileName) = 0;
    virtual void write(int fileDescriptor) = 0;
};

