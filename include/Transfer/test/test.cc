// Updated FileWriter and FileTransferTask with fd-to-filename resolution at add stage
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>
#include <limits.h>
#include <unistd.h>
#include <cstdio>
#include <fcntl.h>
// 抽象写入接口
class ITransferWriter {
public:
    virtual ~ITransferWriter() = default;
    virtual void write(const std::string& fileName) = 0;
    virtual void write(int fileDescriptor) = 0;
};

// FileWriter: 写入到文本文件，带 fd->filename 解析
class FileWriter : public ITransferWriter {
public:
    explicit FileWriter(const std::string& path)
        : ofs_(path, std::ios::app)
    {
        if (!ofs_) throw std::runtime_error("Cannot open file: " + path);
    }
    void write(const std::string& fileName) override {
        ofs_ << "FILE:" << fileName << "\n";
    }
    void write(int fd) override {
        // Delegate to filename overload
        char linkPath[64];
        char filePath[PATH_MAX];
        std::snprintf(linkPath, sizeof(linkPath), "/proc/self/fd/%d", fd);
        ssize_t len = ::readlink(linkPath, filePath, sizeof(filePath) - 1);
        if (len != -1) {
            filePath[len] = '\0';
            write(std::string(filePath));
        } else {
            ofs_ << "FD:" << fd << "\n";
        }
    }
private:
    std::ofstream ofs_;
};

// 抽象任务接口
class ITransferTask {
public:
    virtual ~ITransferTask() = default;
    virtual void add(const std::string& fileName) = 0;
    virtual void add(int fileDescriptor) = 0;
    virtual void execute() = 0;
};

// FileTransferTask: 在 add 时解析 fd->filename，统一存储 string
class FileTransferTask : public ITransferTask {
public:
    explicit FileTransferTask(std::shared_ptr<ITransferWriter> writer)
        : writer_(std::move(writer)) {}

    void add(const std::string& fileName) override {
        pendingNames_.push_back(fileName);
    }

    void add(int fileDescriptor) override {
        // 解析文件描述符对应的路径并入队
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
    }

    void execute() override {
        for (const auto& name : pendingNames_) {
            writer_->write(name);
        }
        pendingNames_.clear();
    }

private:
    std::shared_ptr<ITransferWriter> writer_;
    std::vector<std::string> pendingNames_;
};

// 示例用法
int main01() {
    auto writer = std::make_shared<FileWriter>("transfer_list.txt");
    FileTransferTask task(writer);
    task.add("/path/to/file.txt");
    int  fd= open("./another_file.txt", O_RDONLY);
    task.add(fd);  // fd 将被解析为实际路径或 <fd:3>
    task.execute();
    return 0;
}
