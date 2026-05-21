#ifndef FILETRANSFER_H
#define FILETRANSFER_H

#include "protocol.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

struct FileUploadState
{
    std::ofstream file;
    std::string fileName;
    std::uint64_t fileSize = 0;
    std::uint64_t receivedSize = 0;
};

class FileTransferService
{
public:
    explicit FileTransferService(std::string uploadDir = "uploads")
        : m_uploadDir(std::move(uploadDir))
    {
    }

    bool ensureUploadDir() const
    {
        /*
        * 确保上传目录存在：
        * - 如果目录不存在，则创建目录
        * - 返回目录是否可用
        */
        std::error_code ec;
        std::filesystem::create_directories(m_uploadDir, ec);
        return !ec;
    }

    std::vector<std::string> listFiles() const
    {
        /*
        * 列出上传目录中的文件：
        * - 遍历上传目录，收集所有常规文件的文件名
        * - 返回文件列表
        */
        std::vector<std::string> files;
        std::error_code ec;
        for (const auto &entry : std::filesystem::directory_iterator(m_uploadDir, ec)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path().filename().string());
            }
        }
        return files;
    }

    bool beginUpload(FileUploadState &state,
                     const std::string &payload,
                     std::string &savedName,
                     std::string &error) const
    {
        /*
        * 处理文件上传开始请求：
        * - 解析上传请求数据包，获取文件名和文件大小
        * - 验证文件名合法性，防止目录遍历攻击
        * - 创建文件并初始化上传状态
        * - 返回上传是否成功
        */
        std::string fileName;
        Protocol::QWord fileSize = 0;
        if (!Protocol::parseFileBeginPayload(payload, fileName, fileSize)) {
            error = "Invalid file header.";
            return false;
        }

        savedName = sanitizeFileName(fileName);
        if (savedName.empty()) {
            error = "Invalid file name.";
            return false;
        }

        state.file.close();
        state.fileName = savedName;
        state.fileSize = fileSize;
        state.receivedSize = 0;
        state.file.open(makeFilePath(savedName), std::ios::binary | std::ios::trunc);
        if (!state.file.is_open()) {
            error = "Server cannot create file.";
            return false;
        }

        return true;
    }

    bool writeChunk(FileUploadState &state, const std::string &payload, std::string &error) const
    {
        /*
        * 处理文件上传数据块：
        * - 验证上传状态，确保上传已经开始
        * - 将数据块写入文件
        * - 更新已接收数据大小
        * - 返回写入是否成功
        */
        if (!state.file.is_open()) {
            error = "Upload begin packet required.";
            return false;
        }

        state.file.write(payload.data(), static_cast<std::streamsize>(payload.size()));
        state.receivedSize += payload.size();
        return true;
    }

    bool endUpload(FileUploadState &state, std::string &fileName, std::string &error) const
    {
        /*
        * 处理文件上传结束请求：
        * - 验证上传状态，确保上传已经开始
        * - 关闭文件并完成上传
        * - 验证文件完整性，确保接收的数据大小与预期一致
        * - 如果文件不完整，删除文件并返回错误
        * - 返回上传是否成功
        */
        if (!state.file.is_open()) {
            error = "No upload is in progress.";
            return false;
        }

        state.file.close();
        fileName = state.fileName;
        const bool ok = state.receivedSize == state.fileSize;

        state.fileName.clear();
        state.fileSize = 0;
        state.receivedSize = 0;

        if (!ok) {
            std::filesystem::remove(makeFilePath(fileName));
            error = "Upload incomplete, file deleted.";
            return false;
        }

        return true;
    }

    bool readFileInfo(const std::string &rawName, std::string &cleanName, Protocol::QWord &fileSize) const
    {
        /*
        * 处理文件下载请求：
        * - 验证文件名合法性，防止目录遍历攻击
        * - 检查文件是否存在
        * - 获取文件大小
        * - 返回文件信息是否成功
        */
        cleanName = sanitizeFileName(rawName);
        const std::string path = makeFilePath(cleanName);
        if (!std::filesystem::exists(path)) {
            return false;
        }

        fileSize = static_cast<Protocol::QWord>(std::filesystem::file_size(path));
        return true;
    }

    bool openFileForRead(const std::string &fileName, std::ifstream &in) const
    {
        /*
        * 打开文件以供下载：
        * - 验证文件名合法性，防止目录遍历攻击
        * - 打开文件并返回是否成功
        */
        in.open(makeFilePath(fileName), std::ios::binary);
        return in.is_open();
    }

    void removeIncompleteUpload(const FileUploadState &state) const
    {
        /*
        * 删除未完成的上传文件：
        * - 验证上传状态，确保有未完成的上传
        * - 删除文件以清理服务器资源
        */
        if (!state.fileName.empty()) {
            std::filesystem::remove(makeFilePath(state.fileName));
        }
    }

private:
    std::string sanitizeFileName(const std::string &fileName) const
    {
        /*
        * 清理文件名以防止目录遍历攻击：
        * - 替换文件名中的非法字符（如路径分隔符）为下划线
        * - 返回清理后的文件名
        */
        std::string result = fileName;
        for (char &ch : result) {
            if (ch == '/' || ch == '\\' || ch == ':' || ch == '*' || ch == '?' || ch == '"' || ch == '<' || ch == '>' || ch == '|') {
                ch = '_';
            }
        }
        return result;
    }

    std::string makeFilePath(const std::string &fileName) const
    {
        /*
        * 构造文件路径：
        * - 将上传目录与文件名组合成完整的文件路径
        * - 返回文件路径
        */
        return m_uploadDir + "/" + fileName;
    }

private:
    std::string m_uploadDir;
};

#endif
