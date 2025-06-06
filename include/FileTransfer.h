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

//提取类的头文件
class FileTransferClient{
    public:
    FileTransferClient(const string& serverIp, int port);
    ~FileTransferClient();
    bool connectToServer();
    void deleteFile(const string& filename);
    void uploadFile(const string& filename);
    int downloadFile(const string& filename);
    void closeConnection();

    private:
    string serverIp;
    int port;
    int sockfd;
    int readLine(int fd, char *buffer, int maxLength);
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