#pragma once

#include "Runtime/Core/FString.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/Core/TMap.h"
#include "Runtime/Core/FName.h"
#include "Runtime/Rendering/Vertices.h"
#include "Runtime/Geometry/FAxisAlignedBoundingBox.h"

struct FVertexKey
{
	FVertexKey() = default;
	FVertexKey(int32 _p, int32 _uv, int32 _n)
		:PosIndex(_p), UVIndex(_uv), NormalIndex(_n)
	{}

	bool operator== (const FVertexKey& Other) const = default;

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

constexpr int32 INVALID_INDEX = -1;

template<>
struct std::hash<FVertexKey>
{
	size_t operator()(const FVertexKey& Key) const
	{
		size_t Hash = std::hash<int32>()(Key.PosIndex);
		Hash ^= std::hash<int32>()(Key.UVIndex) + 0x9e3779b9 + (Hash << 6) + (Hash >> 2);
		Hash ^= std::hash<int32>()(Key.NormalIndex) + 0x9e3779b9 + (Hash << 6) + (Hash >> 2);
		return Hash;
	}
};

struct FObjMaterialInfo
{
	FName MaterialName{ "None" };
	FVector KaAmbient{ 0.2f, 0.2f, 0.2f };
	FVector KdDiffuse{ 0.8f, 0.8f, 0.8f };
	FVector KsSpecular{ 1.f,1.f,1.f };
	FVector KeEmissive{ 0.f,0.f,0.f };
	float NsShininess = 32.f;
	float DOpacity = 1.f;
	int32 Illumination = 2;
	FName TextureName{ "uv-test.png" };
};

struct FObjMeshSection
{
	uint32 StartIndex = 0;
	uint32 IndexCount = 0;
	int32 MaterialIndex = INVALID_INDEX;
};

struct FObjVertexInfo
{
	FName ObjectName{ "None" };
	TArray<FVertexData> Vertices;
	TArray<uint32> Indices;
	TArray<FObjMaterialInfo> Materials;
	TArray<FObjMeshSection> Sections;
	FAxisAlignedBoundingBox LocalBounds;
	//FName TextureName{ "uv-test.png" };
	bool bIsValid = false;
};

class FObjDecoder
{
public:
	static bool DecodeFromFile(const FString& FilePath, FObjVertexInfo& VetexInfoOut, FObjMaterialInfo& MaterialInfoOut);

private:
	static bool DecodeObjFile(const FString& FileContent, FObjVertexInfo& OutData);

	static int32 ResolveIndex(const std::string_view& String, const uint32 Count);

	[[nodiscard]]
	static FVertexData MakeVertex(const FVertexKey& Key, const TArray<FVector>& Positions, const TArray<FVector2>& UVs, const TArray<FVector>& Normals);

	static void ComputeStaticBounds(FObjVertexInfo& OutData);

	static bool DecodeMtlFile(const FString& FileContent, FObjMaterialInfo& OutData);

};
