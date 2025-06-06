
#include "FileTransfer.h"
    FileTransferClient::FileTransferClient(const string& serverIp, int port): serverIp(serverIp), port(port) {}
    
    bool FileTransferClient::connectToServer() {
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
    
    void FileTransferClient::closeConnection() {
        close(sockfd);
    }
    
    // 上传文件
    void FileTransferClient::uploadFile(const string& filename) {
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
    int FileTransferClient::downloadFile(const string& filename) {
        // 发送下载命令
        string command = "DOWNLOAD " + filename + "\n";
        write(sockfd, command.c_str(), command.size());
        // 读取文件大小
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        int n = readLine(sockfd, buffer, sizeof(buffer));
        if(n <= 0){
            cout << "读取文件大小失败" << endl;
            return -1;
        }
        long fileSize = atol(buffer);
        if(fileSize <= 0) {
            cout << "文件不存在或为空" << endl;
            return -1;
        }
        // 创建文件保存下载内容
        //去除路径
        string filenameWithoutPath = filename.substr(filename.find_last_of("/\\") + 1);
        ofstream ofs(filenameWithoutPath, ios::binary);
        if(!ofs.is_open()){
            cout << "无法打开文件写入: " << filenameWithoutPath << endl;
            return -1;
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
        return 0;
    }
    
    // 删除服务端文件
    void FileTransferClient::deleteFile(const string& filename) {
        string command = "DELETE " + filename + "\n";
        write(sockfd, command.c_str(), command.size());
        char resp[1024];
        memset(resp, 0, sizeof(resp));
        int n = readLine(sockfd, resp, sizeof(resp));
        cout << "服务端响应: " << resp;
    }
    
    
int FileTransferClient::readLine(int fd, char *buffer, int maxLength) {
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

    FileTransferClient::~FileTransferClient()
    {
        closeConnection();
        
    }
