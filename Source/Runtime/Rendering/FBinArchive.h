#pragma once

#include "Runtime/Core/IntTypes.h"

class FBinArchive
{
public:
    FBinArchive();
    ~FBinArchive() = default;

    void Clear();
    void ResetReadPosition();
    const TArray<uint8>& GetBytes() const;
    size_t GetRemainingBytes() const;
    void SetBytes(TArray<uint8>&& InBytes);

    bool SerializeUInt32(uint32 Value);
    bool DeserializeUInt32(uint32& Value);
    bool SerializeInt32(int32 Value);
    bool DeserializeInt32(int32& Value);
    bool SerializeFloat(float Value);
    bool DeserializeFloat(float& Value);
    bool SerializeBool(bool Value);
    bool DeserializeBool(bool& Value);

    // 문자열/배열 객체의 내부 포인터가 아닌 실제 문자만 저장한다.
    bool SerializeString(const FString& Value);
    bool DeserializeString(FString& Value);

private:
    bool SerializeBytes(const void* Data, uint32 ByteCount);
    bool DeserializeBytes(void* OutData, uint32 ByteCount);

public:
    static constexpr size_t MaxArchiveBytes = 512ULL * 1024 * 1024;

private:
    TArray<uint8> Bytes;
    size_t ReadOffset;
};
