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


struct FSectionKey
{
    int32 Object = -1;
    int32 Group = -1;
    int32 Material = -1;
    auto operator<=>(const FSectionKey&) const = default;
    // 비교 연산자 6개를 전부 만들어준다. 결과를 0과 비교해서 사용
    // 예시 if ((A <=> B) < 0) -> A가 작다
};

struct FMeshSection
{
    uint32 FirstIndex = 0; // Indices 배열에서 이 그룹이 시작하는 위치
    uint32 IndexCount = 0; // 이 그룹의 인덱스 개수 (삼각형 수 × 3)

    int32 Object = -1; //해당 섹션의 ObjectName
    int32 Group = -1; //해당 색션의 Group 번호
    int32 MaterialIndex = -1; // 해당 섹션의 Material번호

    FAxisAlignedBoundingBox LocalBounds;  // 이 그룹만의 바운딩 박스 (파츠 피킹용)
    
};

struct FObjGroupInfo
{
    FString Name;
};

struct FObjObjectInfo
{
    FString Name;
};


// Cooked Data
struct FObjModelData // 이름바꿔야됨
{
    std::string PathFileName;

    TArray<FVertexData> Vertices; // 정점들 (큐브기준 24)
    TArray<uint32> Indices; // 사용할 인덱스 순서 (큐브기준 36)


    FName TextureName{ "None" };
    bool bIsValid = false;

    TArray<FMeshSection> Sections;

    TArray<FObjMaterialInfo> Materials;
    TArray<FObjGroupInfo> Groups;
    TArray<FObjObjectInfo> ObjectNames;    
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
    TArray<int32> GroupList;
    TArray<int32> ObjectNamesList;
    TArray<int32> TextureList;

    TArray<FString> MaterialLibs;
    TArray<FObjMaterialInfo> Materials;

    TArray<FObjGroupInfo> Groups;
    TArray<FObjObjectInfo> ObjectNames;
};

class FObjDecoder
{
private:
    static TSortedMap<FString, FObjModelData*> ObjStaticMeshMap;
    FObjInfo ObjInfo;

    FString ObjDirectory;
    int32 CurrentMaterial = -1;    
    int32 DefiningMaterial = -1;

    int32 CurrentGroup = -1;
    int32 CurrentObjectName = -1;

    // obj
    void AddVertexList(std::string_view Line);
    void AddUVList(std::string_view Line);
    void AddNormalList(std::string_view Line);

    void ParseLine(std::string_view Line);
    void ParseFace(std::string_view Line);

    void AddMaterialLib(std::string_view Line);
    void UseMaterial(std::string_view Line);

    void UseGroup(std::string_view Line);
    int32 FindOrAddGroup(std::string_view Name);

    void UseObjectName(std::string_view Line);
    int32 FindOrAddObjectName(std::string_view Name);

    FObjInfo ParseObjFile(const FString& File);
    FObjInfo StartObjFileParser(const FString& PathFileName);

    // mtl
    void ParseMtlFile(const FString& File);
    void ParseMtlLine(std::string_view Line);
    int32 FindOrAddMaterial(std::string_view Name);

    static bool CookStaticMesh(const FObjInfo& Info, FObjModelData& Out);

public:
    static bool DecodeFromFile(const FString& AbsolutePath, FObjModelData& Out);

    static FObjModelData* LoadObjStaticMeshAsset(const FString& PathFileName);

};