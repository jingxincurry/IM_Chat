#ifndef IM_CHAT_PROTOCOL_H
#define IM_CHAT_PROTOCOL_H

#include <cstdint>
#include <string>
#include <vector>

namespace Protocol {

using Byte = unsigned char;
using Word = std::uint16_t;
using DWord = std::uint32_t;
using QWord = std::uint64_t;

constexpr Word kPacketHead = 0xFEFF;

enum Command : Word
{
    CmdLogin = 1,
    CmdLoginOk = 2,
    CmdSystemMessage = 3,
    CmdChatMessage = 4,
    CmdUserList = 5,
    CmdFileList = 6,
    CmdFileUploadBegin = 7,
    CmdFileUploadChunk = 8,
    CmdFileUploadEnd = 9,
    CmdFileDownloadRequest = 10,
    CmdFileDownloadBegin = 11,
    CmdFileDownloadChunk = 12,
    CmdFileDownloadEnd = 13,
    CmdError = 14
};

class Packet
{
public:
    Packet()
        : m_cmd(0)
    {
    }

    Packet(Word cmd, const std::string &payload)
        : m_cmd(cmd)
        , m_payload(payload)
    {
    }

    Word command() const
    {
        return m_cmd;
    }

    const std::string &payload() const
    {
        return m_payload;
    }

    std::string serialize() const
    {
        std::string out;
        const DWord length = static_cast<DWord>(m_payload.size() + 4);
        out.reserve(static_cast<size_t>(length) + 6);

        appendWord(out, kPacketHead);
        appendDWord(out, length);
        appendWord(out, m_cmd);
        out += m_payload;
        appendWord(out, checksum(m_payload));
        return out;
    }

    static bool tryParse(std::string &buffer, Packet &packet)
    {
        size_t pos = 0;
        while (pos + 1 < buffer.size()) {
            if (readWord(buffer.data() + pos) == kPacketHead) {
                break;
            }
            ++pos;
        }

        if (pos > 0) {
            buffer.erase(0, pos);
        }

        if (buffer.size() < 10) {
            return false;
        }

        const DWord length = readDWord(buffer.data() + 2);
        if (length < 4) {
            buffer.erase(0, 2);
            return false;
        }

        const size_t totalSize = static_cast<size_t>(length) + 6;
        if (buffer.size() < totalSize) {
            return false;
        }

        const Word cmd = readWord(buffer.data() + 6);
        const size_t payloadSize = length - 4;
        const std::string payload = buffer.substr(8, payloadSize);
        const Word sum = readWord(buffer.data() + 8 + payloadSize);
        buffer.erase(0, totalSize);

        if (checksum(payload) != sum) {
            return false;
        }

        packet = Packet(cmd, payload);
        return true;
    }

    static void appendWord(std::string &out, Word value)
    {
        out.push_back(static_cast<char>(value & 0xFF));
        out.push_back(static_cast<char>((value >> 8) & 0xFF));
    }

    static void appendDWord(std::string &out, DWord value)
    {
        for (int i = 0; i < 4; ++i) {
            out.push_back(static_cast<char>((value >> (i * 8)) & 0xFF));
        }
    }

    static void appendQWord(std::string &out, QWord value)
    {
        for (int i = 0; i < 8; ++i) {
            out.push_back(static_cast<char>((value >> (i * 8)) & 0xFF));
        }
    }

    static void appendString(std::string &out, const std::string &value)
    {
        appendDWord(out, static_cast<DWord>(value.size()));
        out += value;
    }

    static Word readWord(const char *data)
    {
        return static_cast<Word>(static_cast<Byte>(data[0]))
            | (static_cast<Word>(static_cast<Byte>(data[1])) << 8);
    }

    static DWord readDWord(const char *data)
    {
        DWord value = 0;
        for (int i = 0; i < 4; ++i) {
            value |= static_cast<DWord>(static_cast<Byte>(data[i])) << (i * 8);
        }
        return value;
    }

    static QWord readQWord(const char *data)
    {
        QWord value = 0;
        for (int i = 0; i < 8; ++i) {
            value |= static_cast<QWord>(static_cast<Byte>(data[i])) << (i * 8);
        }
        return value;
    }

    static bool readString(const std::string &data, size_t &offset, std::string &value)
    {
        if (offset + 4 > data.size()) {
            return false;
        }

        const DWord length = readDWord(data.data() + offset);
        offset += 4;
        if (offset + length > data.size()) {
            return false;
        }

        value.assign(data.data() + offset, length);
        offset += length;
        return true;
    }

private:
    static Word checksum(const std::string &payload)
    {
        Word sum = 0;
        for (char ch : payload) {
            sum = static_cast<Word>(sum + static_cast<Byte>(ch));
        }
        return sum;
    }

private:
    Word m_cmd;
    std::string m_payload;
};

inline std::string makeStringPayload(const std::string &text)
{
    std::string payload;
    Packet::appendString(payload, text);
    return payload;
}

inline bool parseStringPayload(const std::string &payload, std::string &text)
{
    size_t offset = 0;
    return Packet::readString(payload, offset, text) && offset == payload.size();
}

inline std::string makeChatPayload(const std::string &sender, const std::string &message)
{
    std::string payload;
    Packet::appendString(payload, sender);
    Packet::appendString(payload, message);
    return payload;
}

inline bool parseChatPayload(const std::string &payload, std::string &sender, std::string &message)
{
    size_t offset = 0;
    return Packet::readString(payload, offset, sender)
        && Packet::readString(payload, offset, message)
        && offset == payload.size();
}

inline std::string makeStringListPayload(const std::vector<std::string> &items)
{
    std::string payload;
    Packet::appendDWord(payload, static_cast<DWord>(items.size()));
    for (const std::string &item : items) {
        Packet::appendString(payload, item);
    }
    return payload;
}

inline bool parseStringListPayload(const std::string &payload, std::vector<std::string> &items)
{
    items.clear();
    size_t offset = 0;
    if (payload.size() < 4) {
        return false;
    }

    const DWord count = Packet::readDWord(payload.data());
    offset += 4;
    for (DWord i = 0; i < count; ++i) {
        std::string item;
        if (!Packet::readString(payload, offset, item)) {
            return false;
        }
        items.push_back(item);
    }
    return offset == payload.size();
}

inline std::string makeFileBeginPayload(const std::string &fileName, QWord fileSize)
{
    std::string payload;
    Packet::appendString(payload, fileName);
    Packet::appendQWord(payload, fileSize);
    return payload;
}

inline bool parseFileBeginPayload(const std::string &payload, std::string &fileName, QWord &fileSize)
{
    size_t offset = 0;
    if (!Packet::readString(payload, offset, fileName)) {
        return false;
    }
    if (offset + 8 > payload.size()) {
        return false;
    }
    fileSize = Packet::readQWord(payload.data() + offset);
    offset += 8;
    return offset == payload.size();
}

} // namespace Protocol

#endif
