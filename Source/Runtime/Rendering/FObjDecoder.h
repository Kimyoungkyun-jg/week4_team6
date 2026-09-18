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


struct FObjMaterialInfo
{
    FString Name;

    FVector Ambient{ 0.2f, 0.2f, 0.2f };   // Ka
    FVector Diffuse{ 0.8f, 0.8f, 0.8f };   // Kd
    FVector Specular{ 0.0f, 0.0f, 0.0f };  // Ks
    float SpecularExponent = 0.0f;         // Ns
    float Opacity = 1.0f;                  // d  (Tr 는 1 - d)
    int32 IlluminationModel = 0;           // illum

    FString DiffuseTexture;   // map_Kd
    FString AmbientTexture;   // map_Ka
    FString SpecularTexture;  // map_Ks
    FString AlphaTexture;     // map_d
    FString NormalTexture;    // map_bump / bump / norm
};

// Cooked Data
struct FStaticMeshDecoder // 이름바꿔야됨
{
    std::string PathFileName;

    TArray<FVertexData> Vertices;
    TArray<uint32> Indices;

    TArray<FObjMaterialInfo> Materials;
    TArray<int32> TriangleMaterials;
};

// Raw Data
// 없는 항목(vt/vn 생략)은 -1.
struct FObjInfo
{
    TArray<FVector4> VertexList;
    TArray<FVector> ColorList;
    TArray<FVector> UVList;
    TArray<FVector> NormalList;

    TArray<FVector> VertexIndexList;
    TArray<FVector> ColorIndexList;
    TArray<FVector> UVIndexList;
    TArray<FVector> NormalIndexList;

    TArray<int32> MaterialList;
    TArray<int32> TextureList;

    TArray<FString> MaterialLibs;
    TArray<FObjMaterialInfo> Materials;
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
    static TSortedMap<FString, FStaticMeshDecoder*> ObjStaticMeshMap;
    FObjInfo ObjInfo;

    FString ObjDirectory;
    int32 CurrentMaterial = -1;
    int32 DefiningMaterial = -1;

    // obj
    void AddVertexList(std::string_view Line);
    void AddUVList(std::string_view Line);
    void AddNormalList(std::string_view Line);
    void ParseLine(std::string_view Line);
    void ParseFace(std::string_view Line);
    void AddMaterialLib(std::string_view Line);
    void UseMaterial(std::string_view Line);
    FObjInfo ParseObjFile(const FString& File);
    FObjInfo StartObjFileParser(const FString& PathFileName);

    // mtl
    void ParseMtlFile(const FString& File);
    void ParseMtlLine(std::string_view Line);
    int32 FindOrAddMaterial(std::string_view Name);

    static bool CookStaticMesh(const FObjInfo& Info, FStaticMeshDecoder& Out);

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
