#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>
#include <limits.h>
#include <unistd.h>
#include <cstdio>
#include "ITransferWriter.h"
class FileWriter : public ITransferWriter {
public:
    explicit FileWriter(const std::string& path)
        : ofs_(path, std::ios::app)
    {
        if (!ofs_) throw std::runtime_error("无法打开文件：" + path);
    }

    void write(const std::string& fileName) override {
        ofs_ << "FILE:" << fileName << "\n";
    }

    void write(int fd) override {
        // Resolve filename from file descriptor (Linux-specific /proc)
        char linkPath[64];
        char filePath[PATH_MAX];
        std::snprintf(linkPath, sizeof(linkPath), "/proc/self/fd/%d", fd);
        ssize_t len = ::readlink(linkPath, filePath, sizeof(filePath) - 1);
        if (len != -1) {
            filePath[len] = '\0';
            write(std::string(filePath));  // Delegate to filename overload
        } else {
            // Fallback: write raw fd
            ofs_ << "FD:" << fd << "\n";
        }
    }

private:
    std::ofstream ofs_;
};
