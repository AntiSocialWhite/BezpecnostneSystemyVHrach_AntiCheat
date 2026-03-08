#include "cheater_info.h"

#include <cstring>

namespace ac_shared
{
    SessionBridge::SessionBridge()
        : m_mapping(nullptr), m_block(nullptr)
    {
    }

    SessionBridge::~SessionBridge()
    {
        Close();
    }

    bool SessionBridge::CreateForMain(DWORD gamePid, const std::string& sessionId)
    {
        Close();

        m_mapping = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            nullptr,
            PAGE_READWRITE,
            0,
            sizeof(SharedSessionBlock),
            kMappingName
        );

        if (!m_mapping)
            return false;

        m_block = reinterpret_cast<SharedSessionBlock*>(
            MapViewOfFile(m_mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedSessionBlock))
            );

        if (!m_block)
        {
            CloseHandle(m_mapping);
            m_mapping = nullptr;
            return false;
        }

        ZeroMemory(m_block, sizeof(SharedSessionBlock));
        return WriteSession(gamePid, sessionId);
    }

    bool SessionBridge::OpenForDll()
    {
        Close();

        m_mapping = OpenFileMappingA(FILE_MAP_READ, FALSE, kMappingName);
        if (!m_mapping)
            return false;

        m_block = reinterpret_cast<SharedSessionBlock*>(
            MapViewOfFile(m_mapping, FILE_MAP_READ, 0, 0, sizeof(SharedSessionBlock))
            );

        if (!m_block)
        {
            CloseHandle(m_mapping);
            m_mapping = nullptr;
            return false;
        }

        return true;
    }

    void SessionBridge::Close()
    {
        if (m_block)
        {
            UnmapViewOfFile(m_block);
            m_block = nullptr;
        }

        if (m_mapping)
        {
            CloseHandle(m_mapping);
            m_mapping = nullptr;
        }
    }

    bool SessionBridge::WriteSession(DWORD gamePid, const std::string& sessionId)
    {
        if (!m_block)
            return false;

        ZeroMemory(m_block, sizeof(SharedSessionBlock));

        m_block->magic = kMagic;
        m_block->game_pid = gamePid;
        m_block->ready = 0;

        strncpy_s(m_block->session_id, sessionId.c_str(), _TRUNCATE);

        InterlockedExchange(&m_block->ready, 1);
        return true;
    }

    std::string SessionBridge::ReadSession(DWORD expectedPid) const
    {
        if (!m_block)
            return "";

        if (m_block->magic != kMagic)
            return "";

        if (m_block->ready != 1)
            return "";

        if (m_block->game_pid != expectedPid)
            return "";

        return std::string(m_block->session_id);
    }

    bool SessionBridge::IsOpen() const
    {
        return m_block != nullptr;
    }

    SharedSessionBlock* SessionBridge::GetBlock() const
    {
        return m_block;
    }
}