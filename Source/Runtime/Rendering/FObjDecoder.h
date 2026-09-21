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

#define EXPLICIT 0
#define SMOOTH 1
#define FLAT 2

struct FTriangleIndices {
    int32 Index[3] = { -1,-1,-1 };

    FTriangleIndices() = default;
    FTriangleIndices(int A, int B, int C) : Index{ A, B, C } {}
};

struct FCorner { int32 V, VT, VN; };


// 섹션 그룹화 키
struct FSectionKey
{
    int32 Object = -1;
    int32 Group = -1;
    int32 Material = -1;
    auto operator<=>(const FSectionKey&) const = default;
    // 비교 연산자 6개를 전부 만들어준다. 결과를 0과 비교해서 사용
    // 예시 if ((A <=> B) < 0) -> A가 작다
};

struct FObjGroupInfo
{
    FString Name;
};

struct FObjObjectInfo
{
    FString Name;
};

// Todo: Bin - Materials.bin의 데이터 항목. 정의는 메시 캐시와 분리한다.
class FBinArchive;

struct FMaterialBinaryEntry
{
    FString MaterialLibraryPath;
    FObjMaterialInfo Material;
};

// Cooked Data
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
    // Todo: Bin - 메시 캐시에는 정의 대신 참조 MTL 경로와 섹션 ID만 저장한다.
    TArray<FString> MaterialLibraryPaths;
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

    TArray<FTriangleIndices> VertexIndexList;
    TArray<FTriangleIndices> UVIndexList;
    TArray<FTriangleIndices> NormalIndexList;

    TArray<int32> MaterialList;
    TArray<int32> GroupList;
    TArray<int32> ObjectNamesList;
    TArray<int32> SmoothingGroupsList;

    TArray<FString> MaterialLibs;
    TArray<FObjMaterialInfo> Materials;

    TArray<FObjGroupInfo>  Groups;
    TArray<FObjObjectInfo> ObjectNames;
};

// OBJ 디코더 클래스
class FObjDecoder
{
private:
    // Todo: Bin - 한 디코더가 먼저 로딩한 공유 머티리얼을 모든 OBJ가 참조한다.
    TArray<FMaterialBinaryEntry> MaterialEntries;
    TMap<FString, TArray<FObjMaterialInfo>> ParsedMaterialsByLibrary;
    TMap<FString, FObjMaterialInfo> ParsedMaterialsById;
    const FObjDecoder* MaterialLibraryOwner = nullptr;
    bool bMaterialsLoaded = false;
    void ResolveSectionMaterials(FObjModelData& Model) const;
    FObjInfo ObjInfo;

    FString ObjDirectory;
    int32 CurrentMaterial = -1;
    int32 DefiningMaterial = -1;

    int32 CurrentGroup = -1;
    int32 CurrentObjectName = -1;
    int32 CurrentSmoothingGroup = 0; // 0 or off 사용안함

    // 정점 및 속성 추가
    void AddVertexList(std::string_view Line);
    void AddUVList(std::string_view Line);
    void AddNormalList(std::string_view Line);

    // 라인 및 면 파싱
    void ParseLine(std::string_view Line);
    void ParseFace(std::string_view Line);

    // 머티리얼 라이브러리
    void AddMaterialLib(std::string_view Line);
    // Todo: Bin - MTL을 다시 파싱하지 않고 이미 로딩한 정보를 연결한다.
    bool ImportMaterialLibrary(const FString& Path);
    void UseMaterial(std::string_view Line);

    // 그룹 및 오브젝트
    void UseGroup(std::string_view Line);
    int32 FindOrAddGroup(std::string_view Name);

    void UseSmoothingGroup(std::string_view Line);

    void SetSmoothingGroup(std::string_view Line);

    void UseObjectName(std::string_view Line);
    int32 FindOrAddObjectName(std::string_view Name);

    FObjInfo ParseObjFile(const FString& File);
    FObjInfo StartObjFileParser(const FString& PathFileName);

    // 머티리얼 파싱
    void ParseMtlFile(const FString& File);
    void ParseMtlLine(std::string_view Line);
    int32 FindOrAddMaterial(std::string_view Name);

    bool IsConvexDot(FVector PrevVertexPosition, FVector EarVertexPosition, FVector NextVertexPosition, FVector Normal);
    bool IsPointInTriangle(FVector A, FVector B, FVector C, FVector Q, FVector Normal);
    void StartEarClipping(const TArray<FCorner>& Corners);

    static bool CookStaticMesh(const FObjInfo& Info, FObjModelData& Out);

public:
    // Todo: Bin - Materials.bin을 만들 때만 독립적으로 MTL을 파싱한다.
    static bool DecodeMaterialsFromFile(const FString& Path, TArray<FObjMaterialInfo>& OutMaterials);
    // Todo: Bin - 파싱/직렬화/역직렬화와 파일 로딩은 모두 FObjDecoder가 담당한다.
    bool DecodeFromFile(const FString& AbsolutePath, FObjModelData& Out);
    bool LoadMaterials(const FString& AssetRoot, const TArray<FString>& SearchRoots);
    bool LoadObj(const FString& ObjPath, const FString& BinaryPath, FObjModelData& OutModel);
    const TArray<FMaterialBinaryEntry>& GetMaterials() const { return MaterialEntries; }

    static bool SerializeObjModel(FBinArchive& Archive, const FObjModelData& Model);
    static bool DeserializeObjModel(FBinArchive& Archive, FObjModelData& OutModel);
    static bool SaveObjModelBinary(const FString& Path, const FObjModelData& Model);
    static bool LoadObjModelBinary(const FString& Path, FObjModelData& OutModel);
    static bool SerializeMaterials(FBinArchive& Archive, const TArray<FMaterialBinaryEntry>& Materials);
    static bool DeserializeMaterials(FBinArchive& Archive, TArray<FMaterialBinaryEntry>& OutMaterials);
    static bool SaveMaterialsBinary(const FString& Path, const TArray<FMaterialBinaryEntry>& Materials);
    static bool LoadMaterialsBinary(const FString& Path, TArray<FMaterialBinaryEntry>& OutMaterials);
};
