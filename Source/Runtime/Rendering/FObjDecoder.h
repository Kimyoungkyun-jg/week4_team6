#pragma once

#include <fstream>

#include "Runtime/Core/FString.h"
#include "Runtime/Core/TMap.h"
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
	FObjDecoder() = default;
	// Todo: Need to delete in FObjDecoder class
	~FObjDecoder() = default;

	void LoadObjStaticMeshAsset(const std::string& PathFileName, FObjModelData* OutModelData);

private:
	void LoadMaterialAssets(const FString& FullPathMaterialName, TArray<FName>* OutMaterialNames);

private:
	// Todo: Who has ownership of FObjModelData
	TMap<FString, FObjModelData*> StringModelDataMap;
	
	//static bool DecodeFromFile(const FString& FilePath, FObjModelData& OutData);
	//static bool DecodeFromString(const FString& FileContent, FObjModelData& OutData, const FString& BaseDirectory);
};
