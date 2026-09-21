// Todo: Bin - 이 파일 전체는 바이너리 캐시 기능을 위해 추가한 구현이다.
#pragma once

#include <cassert>

#include "Runtime/Core/IntTypes.h"
#include "Runtime/Core/FString.h"
#include "Runtime/Core/TArray.h"
#include <cstring>
#include <utility>

// 파일 입출력과 독립적인 바이너리 저장소. 상속이나 모드 전환을 사용하지 않는다.
class FBinArchive
{
public:
    static constexpr size_t MaxArchiveBytes = 512ULL * 1024 * 1024;

    void Clear() { Bytes.clear(); ReadOffset = 0; }
    void ResetReadPosition() { ReadOffset = 0; }
    const TArray<uint8>& GetBytes() const { return Bytes; }
    size_t GetRemainingBytes() const { return Bytes.size() - ReadOffset; }

    void SetBytes(TArray<uint8>&& InBytes)
    {
        assert(InBytes.size() <= MaxArchiveBytes);

        Bytes = std::move(InBytes);
        ReadOffset = 0;
    }

    // 메모리 내용을 배열 끝에 추가한다. 파일에는 아직 쓰지 않는다.
    bool SerializeBytes(const void* Data, size_t ByteCount)
    {
        if (ByteCount > MaxArchiveBytes - Bytes.size())
        {
            return false;
        }

        if (ByteCount == 0) return true;
        if (!Data) return false;
        const auto* Begin = static_cast<const uint8*>(Data);
        Bytes.insert(Bytes.end(), Begin, Begin + ByteCount);
        return true;
    }

    // 읽기 범위를 확인한 뒤 값을 복원하고 커서를 이동한다.
    bool DeserializeBytes(void* OutData, size_t ByteCount)
    {
        if (ByteCount > GetRemainingBytes()) return false;
        if (ByteCount == 0) return true;
        if (!OutData) return false;
        
        std::memcpy(OutData, Bytes.data() + ReadOffset, ByteCount);
        ReadOffset += ByteCount;

        return true;
    }

    bool SerializeUInt32(uint32 Value) { return SerializeBytes(&Value, sizeof(Value)); }
    bool DeserializeUInt32(uint32& Value) { return DeserializeBytes(&Value, sizeof(Value)); }
    bool SerializeInt32(int32 Value) { return SerializeBytes(&Value, sizeof(Value)); }
    bool DeserializeInt32(int32& Value) { return DeserializeBytes(&Value, sizeof(Value)); }
    bool SerializeFloat(float Value) { return SerializeBytes(&Value, sizeof(Value)); }
    bool DeserializeFloat(float& Value) { return DeserializeBytes(&Value, sizeof(Value)); }

    bool SerializeBool(bool Value)
    {
        const uint8 Byte = Value ? 1 : 0;
        return SerializeBytes(&Byte, sizeof(Byte));
    }
    bool DeserializeBool(bool& Value)
    {
        uint8 Byte = 0;
        if (!DeserializeBytes(&Byte, sizeof(Byte)) || Byte > 1) return false;
        Value = Byte != 0;
        return true;
    }

    // 문자열/배열 객체의 내부 포인터가 아닌 실제 문자만 저장한다.
    bool SerializeString(const FString& Value)
    {
        if (Value.size() > MaxArchiveBytes) return false;
        return SerializeUInt32(static_cast<uint32>(Value.size()))
            && SerializeBytes(Value.data(), Value.size());
    }
    bool DeserializeString(FString& Value)
    {
        uint32 Length = 0;
        if (!DeserializeUInt32(Length) || Length > GetRemainingBytes()) return false;
        FString Loaded(Length, '\0');
        if (!DeserializeBytes(Loaded.data(), Length)) return false;
        Value = std::move(Loaded);
        return true;
    }

private:
    TArray<uint8> Bytes;
    size_t ReadOffset = 0;
};
