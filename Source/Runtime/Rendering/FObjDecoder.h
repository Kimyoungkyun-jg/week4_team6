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
#include "FRenderResourceLibrary.h"

// 재질 속성 정보


// 섹션 그룹화 키
struct FSectionKey
{
    int32 Object = -1;
    int32 Group = -1;
    int32 Material = -1;
    auto operator<=>(const FSectionKey&) const = default;
};

struct FObjGroupInfo
{
    FString Name;
};

struct FObjObjectInfo
{
    FString Name;
};

// 디코딩 완료 모델 데이터
struct FObjModelData
{
    std::string PathFileName;

    TArray<FVertexData> Vertices;
    TArray<uint32> Indices;

    FName TextureName{ "None" };
    FName NormalTextureName{ "None" };
    FName SpecularTextureName{ "None" };
    bool bIsValid = false;

    TArray<FMeshSection> Sections;

    TArray<FObjMaterialInfo> Materials;
    TArray<FObjGroupInfo> Groups;
    TArray<FObjObjectInfo> ObjectNames;

    bool HasTextures() const
    {
        return (!TextureName.IsNone() && TextureName != FName("None")) ||
               (!NormalTextureName.IsNone() && NormalTextureName != FName("None")) ||
               (!SpecularTextureName.IsNone() && SpecularTextureName != FName("None"));
    }

    bool HasSections() const { return !Sections.empty(); }
};

// 원시 파싱 데이터
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

// OBJ 디코더 클래스
class FObjDecoder
{
private:
    FObjInfo ObjInfo;

    FString ObjDirectory;
    int32 CurrentMaterial = -1;    
    int32 DefiningMaterial = -1;

    int32 CurrentGroup = -1;
    int32 CurrentObjectName = -1;

    // 정점 및 속성 추가
    void AddVertexList(std::string_view Line);
    void AddUVList(std::string_view Line);
    void AddNormalList(std::string_view Line);

    // 라인 및 면 파싱
    void ParseLine(std::string_view Line);
    void ParseFace(std::string_view Line);

    // 머티리얼 라이브러리
    void AddMaterialLib(std::string_view Line);
    void UseMaterial(std::string_view Line);

    // 그룹 및 오브젝트
    void UseGroup(std::string_view Line);
    int32 FindOrAddGroup(std::string_view Name);

    void UseObjectName(std::string_view Line);
    int32 FindOrAddObjectName(std::string_view Name);

    FObjInfo ParseObjFile(const FString& File);
    FObjInfo StartObjFileParser(const FString& PathFileName);

    // 머티리얼 파싱
    void ParseMtlFile(const FString& File);
    void ParseMtlLine(std::string_view Line);
    int32 FindOrAddMaterial(std::string_view Name);

    static bool CookStaticMesh(const FObjInfo& Info, FObjModelData& Out);

public:
    static bool DecodeFromFile(const FString& AbsolutePath, FObjModelData& Out);
};