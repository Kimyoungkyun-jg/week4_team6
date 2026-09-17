#pragma once

#include "FMesh.h"
#include "Runtime/Core/TSortedMap.h"
#include "Runtime/Core/FString.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/Core/IntTypes.h"
#include "Runtime/Math/FVector.h"
#include "Runtime/Math/FVector2.h"
#include "Runtime/Math/FVector4.h"
#include <string_view>

struct FNormalVertex
{
    FVector pos;
    FVector normal;
    FVector4 color;
    FVector2 tex;
};

// Cooked Data
struct FStaticMeshDecoder // 이름바꿔야됨
{
    std::string PathFileName;

    TArray<FNormalVertex> Vertices;
    TArray<uint32> Indices;
};

// Raw Data
// *IndexList 는 파싱 시 OBJ의 1-based 인덱스를 0-based 로 변환해 저장한다.
// 없는 항목(vt/vn 생략)은 -1.
struct FObjInfo
{
    TArray<FVector4> VertexList;
    TArray<FVector> ColorList;
    TArray<FVector> UVList;
    TArray<FVector> NormalList;

    TArray<FVector4> VertexIndexList;
    TArray<FVector> ColorIndexList;
    TArray<FVector> UVIndexList;
    TArray<FVector> NormalIndexList;

    TArray<int32> MaterialList;
    TArray<int32> TextureList;
};

struct FObjMaterialInfo
{
    // Diffuse Scalar
    // Diffuse Texture
};

struct FObjImporter
{
    // Obj Parsing (*.obj to FObjInfo)
    // Material Parsing (*.obj to MaterialInfo)
    // Convert the Raw data to Cooked data (FStaticMesh)
};

class FObjDecoder
{
private:
    static TSortedMap<FString, FStaticMeshDecoder*>       ObjStaticMeshMap;
    FObjInfo ObjInfo;

    void AddVertexList(std::string_view Line);
    void AddUVList(std::string_view Line);
    void AddNormalList(std::string_view Line);
    void ParseLine(std::string_view Line);
    void ParseFace(std::string_view Line);
    FObjInfo ParseObjFile(const FString& File);
    FObjInfo StartObjFileParser(const FString& PathFileName);

public:
    static FStaticMeshDecoder* LoadObjStaticMeshAsset(const FString& PathFileName);

    //static UStaticMesh* LoadObjStaticMesh(const std::string& PathFileName) {
    //    for (TObjectIterator<UStaticMesh> It; It; ++It)
    //    {
    //        UStaticMesh* StaticMesh = *It;
    //        if (StaticMesh->GetAssetPathFileName() == PathFileName)
    //            return It;
    //    }

    //    FStaticMeshDecoder* Asset = FObjManager::LoadObjStaticMeshAsset(PathFileName);
    //    UStaticMesh* StaticMesh = ConstructObject<UStaticMesh>();
    //    StaticMesh->SetStaticMeshAsset(StaticMeshAsset);
    //}
};

// Preload
//FObjManager::LoadObjStaticMesh("Data/Cube.obj");
//FObjManager::LoadObjStaticMesh("Data/TeaPot.obj");
