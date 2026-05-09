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
        std::error_code ec;
        std::filesystem::create_directories(m_uploadDir, ec);
        return !ec;
    }

    std::vector<std::string> listFiles() const
    {
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
        in.open(makeFilePath(fileName), std::ios::binary);
        return in.is_open();
    }

    void removeIncompleteUpload(const FileUploadState &state) const
    {
        if (!state.fileName.empty()) {
            std::filesystem::remove(makeFilePath(state.fileName));
        }
    }

private:
    std::string sanitizeFileName(const std::string &fileName) const
    {
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
        return m_uploadDir + "/" + fileName;
    }

private:
    std::string m_uploadDir;
};

#endif
