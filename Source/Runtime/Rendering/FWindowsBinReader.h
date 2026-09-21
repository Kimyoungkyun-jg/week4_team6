// Todo: Bin - 이 파일 전체는 바이너리 캐시 기능을 위해 추가한 구현이다.
#pragma once

#include "FBinArchive.h"
#include <filesystem>
#include <fstream>

class FWindowsBinReader
{
public:
    // 파일 전체를 읽은 후 Archive에 넘긴다. 실패하면 기존 Archive는 유지한다.
    static bool Load(const std::filesystem::path& Path, FBinArchive& OutArchive)
    {
        std::ifstream File(Path, std::ios::binary | std::ios::ate);
        
        if (!File)
        {
            return false;
        }

        const auto End = File.tellg();
        if (End == std::streampos(-1))
        {
            return false;
        }

        const auto Size = static_cast<std::streamoff>(End);
        if (Size < 0 || static_cast<uint64>(Size) > FBinArchive::MaxArchiveBytes)
        {
            return false;
        }

        File.seekg(0, std::ios::beg);
        if (!File)
        {
            return false;
        }
            
        TArray<uint8> Loaded(static_cast<size_t>(Size));
        if (!Loaded.empty())
        {
            File.read(reinterpret_cast<char*>(Loaded.data()), static_cast<std::streamsize>(Size));
        }

        if (!File)
        {
            return false;
        }

        OutArchive.SetBytes(std::move(Loaded));

        return true;
    }
};
