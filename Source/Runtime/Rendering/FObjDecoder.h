#pragma once

#include "Runtime/Core/FString.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/Core/FName.h"
#include "Runtime/Rendering/Vertices.h"
#include "Runtime/Geometry/FAxisAlignedBoundingBox.h"

struct FObjModelData
{
	TArray<FVertexData> Vertices;
	TArray<uint32> Indices;
	FAxisAlignedBoundingBox LocalBounds;
	FName TextureName{ "None" };
	bool bIsValid = false;
};

class FObjDecoder
{
public:
	
	static bool DecodeFromFile(const FString& FilePath, FObjModelData& OutData);

	static bool DecodeFromString(const FString& FileContent, FObjModelData& OutData, const FString& BaseDirectory);
};
