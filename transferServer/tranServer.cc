// file_transfer_client.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

class FileTransferClient {
public:
    FileTransferClient(const string& serverIp, int port): serverIp(serverIp), port(port) {}
    
    bool connectToServer() {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(sockfd < 0) {
            cerr << "创建socket失败" << endl;
            return false;
        }
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        if(inet_pton(AF_INET, serverIp.c_str(), &serverAddr.sin_addr) <= 0) {
            cerr << "无效的地址" << endl;
            return false;
        }
        if(connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            cerr << "连接失败" << endl;
            return false;
        }
        return true;
    }
    
    void closeConnection() {
        close(sockfd);
    }
    
    // 上传文件
    void uploadFile(const string& filename) {
        // 打开文件
        ifstream ifs(filename, ios::binary);
        if(!ifs.is_open()){
            cout << "无法打开文件: " << filename << endl;
            return;
        }
        //获取没有路径的文件名
        string filenameWithoutPath = filename.substr(filename.find_last_of("/\\") + 1);
        // 发送命令
        string command = "UPLOAD " + filenameWithoutPath + "\n";
        write(sockfd, command.c_str(), command.size());
        // 获取文件大小
        ifs.seekg(0, ios::end);
        long fileSize = ifs.tellg();
        ifs.seekg(0, ios::beg);
        string sizeStr = to_string(fileSize) + "\n";
        write(sockfd, sizeStr.c_str(), sizeStr.size());
        // 发送文件内容
        char buffer[1024];
        while(!ifs.eof()){
            ifs.read(buffer, sizeof(buffer));
            int count = ifs.gcount();
            if(count > 0) {
                write(sockfd, buffer, count);
            }
        }
        ifs.close();
        // 读取服务端响应
        char resp[1024];
        memset(resp, 0, sizeof(resp));
        int n = readLine(sockfd, resp, sizeof(resp));
        cout << "服务端响应: " << resp;
    }
    
    // 下载文件
    void downloadFile(const string& filename) {
        // 发送下载命令
        string command = "DOWNLOAD " + filename + "\n";
        write(sockfd, command.c_str(), command.size());
        // 读取文件大小
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        int n = readLine(sockfd, buffer, sizeof(buffer));
        if(n <= 0){
            cout << "读取文件大小失败" << endl;
            return;
        }
        long fileSize = atol(buffer);
        if(fileSize <= 0) {
            cout << "文件不存在或为空" << endl;
            return;
        }
        // 创建文件保存下载内容
        //去除路径
        string filenameWithoutPath = filename.substr(filename.find_last_of("/\\") + 1);
        ofstream ofs(filenameWithoutPath, ios::binary);
        if(!ofs.is_open()){
            cout << "无法打开文件写入: " << filenameWithoutPath << endl;
            return;
        }
        long remaining = fileSize;
        while(remaining > 0) {
            int toRead = remaining > sizeof(buffer) ? sizeof(buffer) : remaining;
            int r = read(sockfd, buffer, toRead);
            if(r <= 0) break;
            ofs.write(buffer, r);
            remaining -= r;
        }
        ofs.close();
        cout << "下载完成: " << filenameWithoutPath << endl;
    }
    
    // 删除服务端文件
    void deleteFile(const string& filename) {
        string command = "DELETE " + filename + "\n";
        write(sockfd, command.c_str(), command.size());
        char resp[1024];
        memset(resp, 0, sizeof(resp));
        int n = readLine(sockfd, resp, sizeof(resp));
        cout << "服务端响应: " << resp;
    }
    
private:
    string serverIp;
    int port;
    int sockfd;
    
    int readLine(int fd, char *buffer, int maxLength) {
        int i = 0;
        char ch;
        while(i < maxLength - 1) {
            int n = read(fd, &ch, 1);
            if(n <= 0) break;
            buffer[i++] = ch;
            if(ch == '\n') break;
        }
        buffer[i] = '\0';
        return i;
    }
};

// int main(int argc, char* argv[]){
//     if(argc < 4) {
//         cout << "用法: " << argv[0] << " <server_ip> <port> <command> [filename]\n";
//         cout << "命令: upload, download, delete\n";
//         return 0;
//     }
//     string serverIp = argv[1];
//     int port = atoi(argv[2]);
//     string command = argv[3];
//     string filename = "";
//     if(argc >= 5) {
//         filename = argv[4];
//     }
    
//     FileTransferClient client(serverIp, port);
//     if(!client.connectToServer()){
//         return 1;
//     }
//     if(command == "upload") {
//         if(filename.empty()){
//             cout << "请提供要上传的文件名" << endl;
//         } else {
//             client.uploadFile(filename);
//         }
//     } else if(command == "download") {
//         if(filename.empty()){
//             cout << "请提供要下载的文件名" << endl;
//         } else {
//             client.downloadFile(filename);
//         }
//     } else if(command == "delete") {
//         if(filename.empty()){
//             cout << "请提供要删除的文件名" << endl;
//         } else {
//             client.deleteFile(filename);
//         }
//     } else {
//         cout << "未知命令" << endl;
//     }
//     client.closeConnection();
//     return 0;
// }
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <mutex>
#include <vector>
#include <unordered_set>

// 监控 transfer_list.txt 并定期上传新文件
class TransferListWatcher {
public:
    TransferListWatcher(const std::string& listPath,
                        const std::string& serverIp,
                        int port,
                        std::chrono::seconds interval)
        : listPath_(listPath),
          client_(serverIp, port),
          interval_(interval),
          stopFlag_(false)
    {
        if (!client_.connectToServer()) {
            throw std::runtime_error("无法连接到服务器");
        }
    }

    ~TransferListWatcher() {
        stop();
        client_.closeConnection();
    }

    // 启动后台线程
    void start() {
        workerThread_ = std::thread(&TransferListWatcher::run, this);
    }

    // 停止监控
    void stop() {
        {
            std::lock_guard<std::mutex> lg(mutex_);
            stopFlag_ = true;
        }
        if (workerThread_.joinable()) {
            workerThread_.join();
        }
    }

private:
    void run() {
        while (true) {
            {
                std::lock_guard<std::mutex> lg(mutex_);
                if (stopFlag_) break;
            }
            scanAndUpload();
            std::this_thread::sleep_for(interval_);
        }
    }

    void scanAndUpload() {
        std::ifstream ifs(listPath_);
        if (!ifs.is_open()) {
            std::cerr << "无法打开转移列表: " << listPath_ << std::endl;
            return;
        }

        std::string line;
        while (std::getline(ifs, line)) {
            if (line.empty()) continue;
            // 忽略已处理
            if (uploaded_.count(line) == 0) {
                // 去除前缀 FILE: 或 FD:
                std::string path = stripPrefix(line);
                try {
                    client_.uploadFile(path);
                    uploaded_.insert(line);
                } catch (const std::exception& ex) {
                    std::cerr << "上传失败(" << path << "): " << ex.what() << std::endl;
                }
            }
        }
    }

    std::string stripPrefix(const std::string& entry) {
        const char* prefixes[] = {"FILE:", "FD:"};
        for (auto pfx : prefixes) {
            if (entry.rfind(pfx, 0) == 0) {
                return entry.substr(std::strlen(pfx));
            }
        }
        return entry;
    }

    std::string listPath_;
    FileTransferClient client_;
    std::chrono::seconds interval_;
    std::thread workerThread_;
    std::mutex mutex_;
    bool stopFlag_;
    std::unordered_set<std::string> uploaded_;  // 跟踪已上传条目
};

int main() {
    try {
        TransferListWatcher watcher("../src/transfer_list.txt", "127.0.0.1", 9000, std::chrono::seconds(10));
        watcher.start();

        std::cout << "按 Enter 退出..." << std::endl;
        std::cin.get();

        watcher.stop();
    } catch (const std::exception& ex) {
        std::cerr << "启动失败: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
