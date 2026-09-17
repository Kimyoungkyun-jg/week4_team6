#pragma once

#include "Runtime/Core/FString.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/Rendering/Vertices.h"
#include "Runtime/Geometry/FAxisAlignedBoundingBox.h"

struct FObjModelData
{
	TArray<FVertexData> Vertices;
	TArray<uint32> Indices;
	FAxisAlignedBoundingBox LocalBounds;
	bool bIsValid = false;
};

class FObjDecoder
{
public:

};
