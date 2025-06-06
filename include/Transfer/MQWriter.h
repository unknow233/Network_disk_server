//暂时缺少实际使用
#include "ITransferTask.h"
#include "ITransferWriter.h"
class MQClient {
public:
    void send(const std::string& topic, const std::string& msg) {}
};
class MQWriter : public ITransferWriter {
public:
    explicit MQWriter(MQClient& client, const std::string& topic)
        : client_(client), topic_(topic) {}

    void write(const std::string& fileName) override {
        client_.send(topic_, "FILE:" + fileName);
    }
    void write(int fd) override {
        client_.send(topic_, "FD:" + std::to_string(fd));
    }

private:
    MQClient&      client_;
    std::string    topic_;
};
