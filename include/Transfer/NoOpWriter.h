#pragma once
#include  "ITransferWriter.h"
#include  "ITransferTask.h"
//考虑有不需要转移的情况
class NoOpWriter : public ITransferWriter {
public:
    void write(const std::string&) override       { /* 什么也不做 */ }
    void write(int) override                      { /* 什么也不做 */ }
};

class NoOpTransferTask : public ITransferTask {
public:
    void add(const std::string&) override         { /* 什么也不做 */ }
    void add(int) override                        { /* 什么也不做 */ }
    void execute() override                       { /* 什么也不做 */ }
};
