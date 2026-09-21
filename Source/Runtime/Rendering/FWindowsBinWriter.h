// Todo: Bin - 이 파일 전체는 바이너리 캐시 기능을 위해 추가한 구현이다.
#pragma once

#include "FBinArchive.h"
#include <filesystem>
#include <fstream>

class FWindowsBinWriter
{
public:
    // Archive의 내용을 해석하지 않고 그대로 디스크에 기록한다.
    static bool Save(const std::filesystem::path& Path, const FBinArchive& Archive)
    {
        std::ofstream File(Path, std::ios::binary | std::ios::trunc);
        if (!File) return false;
        const auto& Bytes = Archive.GetBytes();
        if (!Bytes.empty())
            File.write(reinterpret_cast<const char*>(Bytes.data()),
                static_cast<std::streamsize>(Bytes.size()));
        File.close(); // 마지막 버퍼 기록의 실패까지 확인한다.
        return !File.fail();
    }
};
