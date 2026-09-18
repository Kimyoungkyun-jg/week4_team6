#pragma once

#include "Runtime/Core/FString.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/Core/TMap.h"
#include "Runtime/Core/FName.h"
#include "Runtime/Rendering/Vertices.h"
#include "Runtime/Geometry/FAxisAlignedBoundingBox.h"
#include "Runtime/Core/PointerTypes.h"

struct FVertexKey
{
	FVertexKey() = default;
	FVertexKey(int32 _p, int32 _uv, int32 _n)
		:PosIndex(_p), UVIndex(_uv), NormalIndex(_n)
	{
	}

	bool operator== (const FVertexKey& Other) const
	{
		return this->PosIndex == Other.PosIndex && this->UVIndex == Other.UVIndex && this->NormalIndex == Other.NormalIndex;
	}

	int32& operator[] (const int32 Index)
	{
		assert(0 <= Index && Index < 3);
		switch (Index)
		{
		case 0:
			return PosIndex;
		case 1:
			return UVIndex;
		case 2:
			return NormalIndex;
		}
	}

	int32 PosIndex = -1;
	int32 UVIndex = -1;
	int32 NormalIndex = -1;
};

template<>
struct std::hash<FVertexKey>
{
	size_t operator() (const FVertexKey& Vertex) const
	{
		size_t PosIndexHash = std::hash<uint64>{}(static_cast<uint64>(Vertex.PosIndex));
		size_t UVIndexHash = std::hash<uint64>{}(static_cast<uint64>(Vertex.UVIndex));
		size_t NormalIndexHash = std::hash<uint64>{}(static_cast<uint64>(Vertex.NormalIndex));

		return std::hash<uint64>{}((0x9e3779b9 + PosIndexHash ^ (UVIndexHash << 6)) + (NormalIndexHash >> 2));
	}
};

struct FObjModelData
{
	TArray<FVertexData> Vertices;
	TArray<uint32> Indices;
	FAxisAlignedBoundingBox LocalBounds;
	FName ObjectName{ "None" };
	FName TextureName{ "None" };
	FName MaterialName{ "None" };
	bool bIsValid = false;
};

class FObjDecoder
{
public:
	static bool DecodeFromFile(const FString& FilePath, FObjModelData& OutData);

	static bool DecodeFromString(const FString& FileContent, FObjModelData& OutData, const FString& BaseDirectory);


private:
	static int32 ResolveIndex(const std::string_view& String, const uint32 Count);

	[[nodiscard]]
	static TSharedPtr<FVertexData> MakeVertex(const FVertexKey& Key, const TArray<FVector> Positions, const TArray<FVector2> UVs, const TArray<FVector> Normals);

	static void ComputeStaticBounds(FObjModelData& OutData);

};
