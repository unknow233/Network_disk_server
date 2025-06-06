#include "ITransferTask.h"
#include "ITransferWriter.h"
#include <memory>
#include <vector>
#include <limits.h>
#include <unistd.h>
class FileTransferTask : public ITransferTask {
public:
    explicit FileTransferTask(std::shared_ptr<ITransferWriter> writer)
        : writer_(std::move(writer)) {
            log.open("Transferlog.txt");
        }

    void add(const std::string& fileName) override {
        pendingNames_.push_back(fileName);
    }

    void add(int fileDescriptor) override {
          // 解析文件描述符对应的路径并入队
        log  << "fileDescriptor: " << fileDescriptor << std::endl;
        char linkPath[64];
        char filePath[PATH_MAX];
        std::snprintf(linkPath, sizeof(linkPath), "/proc/self/fd/%d", fileDescriptor);
        ssize_t len = ::readlink(linkPath, filePath, sizeof(filePath) - 1);
        if (len != -1) {
            filePath[len] = '\0';
            pendingNames_.push_back(std::string(filePath));
        } else {
            // 若解析失败，可保存特殊标识
            pendingNames_.push_back("<fd:" + std::to_string(fileDescriptor) + ">");
        }
        log  << "filePath: " << filePath <<std::endl;
    }

    void execute() override {
        for (auto& name : pendingNames_) {
            writer_->write(name);
        }
        for (auto& fd : pendingFDs_) {
            writer_->write(fd);
        }
        // 执行完毕后可清空
        pendingNames_.clear();
        pendingFDs_.clear();
        log<<"execute done"<<std::endl;
        log.close();
    }

private:
    std::shared_ptr<ITransferWriter> writer_;
    std::vector<std::string> pendingNames_;
    std::vector<int>         pendingFDs_;
    std::ofstream log;
};
