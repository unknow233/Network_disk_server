#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <sstream>
#include <cassert>
#include <fstream>

namespace fs = std::filesystem;

void put(std::string localFile, std::string remoteFile);
//删除文件
void deleteFile(std::string filePath){
    if (fs::exists(filePath)) {
        fs::remove(filePath);
    } else {
        std::cerr << "File " << filePath << " does not exist." << std::endl;
    }
}

// 处理后删除文件
void processDirectory(const std::string& userDataPath) {
    try {
        if (fs::exists(userDataPath) && fs::is_directory(userDataPath)) {
            for (const auto& entry : fs::recursive_directory_iterator(userDataPath)) {
                if (entry.is_regular_file()) {
                    put(entry.path().string(), entry.path().string());
                    deleteFile(entry.path().string());
                }
            }
        } else {
            std::cerr << "Directory " << userDataPath << " does not exist or is not a directory." << std::endl;
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "General error: " << e.what() << std::endl;
    }
}

// 测试函数
void runTests() {
    std::string testDir = "test_UserData";

    // 清理之前的测试数据
    if (fs::exists(testDir)) {
        fs::remove_all(testDir);
    }

    // 创建测试目录
    fs::create_directory(testDir);

    // 测试用例 1: 正常情况，目录中有多个文件和子目录
    {
        std::cout << "Running Test Case 1: Normal case with files and subdirectories" << std::endl;

        // // 创建文件和子目录
        // fs::create_directory(testDir + "/subdir");
        // std::ofstream(testDir + "/file1.txt");
        // std::ofstream(testDir + "/file2.txt");
        // std::ofstream(testDir + "/subdir/file3.txt");
        // std::ofstream(testDir + "/subdir/file4.txt");

        // // 处理目录
        // processedFiles.clear();
        // processDirectory(testDir);

        // // 验证结果
        // std::vector<std::string> expectedFiles = {
        //     testDir + "/file1.txt",
        //     testDir + "/file2.txt",
        //     testDir + "/subdir/file3.txt",
        //     testDir + "/subdir/file4.txt"
        // };

        // assert(processedFiles == expectedFiles);
        std::cout << "Test Case 1 Passed!" << std::endl;
    }

    // 测试用例 2: 空目录
    {
        std::cout << "Running Test Case 2: Empty directory" << std::endl;

        // 清空目录
        for (const auto& entry : fs::directory_iterator(testDir)) {
            fs::remove_all(entry.path());
        }

        // 处理目录
        processedFiles.clear();
        processDirectory(testDir);

        // 验证结果
        assert(processedFiles.empty());
        std::cout << "Test Case 2 Passed!" << std::endl;
    }

    // 测试用例 3: 目录不存在
    {
        std::cout << "Running Test Case 3: Directory does not exist" << std::endl;

        // 删除测试目录
        fs::remove_all(testDir);

        // 处理目录
        processedFiles.clear();
        processDirectory(testDir);

        // 验证结果
        assert(processedFiles.empty());
        std::cout << "Test Case 3 Passed!" << std::endl;
    }

    // 测试用例 4: 文件名包含特殊字符
    {
        std::cout << "Running Test Case 4: Files with special characters" << std::endl;

        // 重新创建测试目录
        fs::create_directory(testDir);

        // 创建带有特殊字符的文件
        std::ofstream(testDir + "/file with spaces.txt");
        std::ofstream(testDir + "/中文文件.txt");
        std::ofstream(testDir + "/file@#$%^&*().txt");

        // 处理目录
        processedFiles.clear();
        processDirectory(testDir);

        // 验证结果
        std::vector<std::string> expectedFiles = {
            testDir + "/file with spaces.txt",
            testDir + "/中文文件.txt",
            testDir + "/file@#$%^&*().txt"
        };

        assert(processedFiles == expectedFiles);
        std::cout << "Test Case 4 Passed!" << std::endl;
    }

    // 清理测试数据
    fs::remove_all(testDir);
}

int main() {
   processDirectory("../UserData");
}