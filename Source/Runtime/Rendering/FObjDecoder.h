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
    int32 Index[3] = {-1,-1,-1};

    FTriangleIndices() = default;
    FTriangleIndices(int A, int B, int C) : Index{ A, B, C } {}
};

struct FCorner { int32 V, VT, VN; };

struct FObjMaterialInfo
{
    FString Name;

    FVector Ambient{ 0.2f, 0.2f, 0.2f };   // Ka 주변 색상
    FVector Diffuse{ 0.8f, 0.8f, 0.8f };   // Kd 확산 색상
    FVector Specular{ 0.0f, 0.0f, 0.0f };  // Ks 반사색
    FVector Emissive{ 0.0f, 0.0f, 0.0f };  // Ke 발광
    FVector TransmissionFilter{ 1.0f, 1.0f, 1.0f }; // Tf 투과 필터 색상
    float SpecularExponent = 0.0f;         // Ns 반사율
    float Opacity = 1.0f;                  // d  (Tr 는 1 - d) 투명성
    float OpticalDensity = 1.0f;           // Ni 굴절률
    int32 IlluminationModel = 0;           // illum 조명 모델

    FString DiffuseTexture;             // map_Kd 디퓨즈 컬러 맵
    FString AmbientTexture;             // map_Ka 주변 색상 맵
    FString SpecularTexture;            // map_Ks 반사 색상 맵
    FString AlphaTexture;               // map_d 알파 텍스처 맵
    FString NormalTexture;              // map_bump / bump범프 맵
    FString EmissiveTexture;            // map_Ke 발광 텍스처
    FString SpecularExponentTexture;    // map_Ns 반사광 하이라이트 구성 요소
    FString ReflectionTexture;          // refl 구형 반사 맵
    FString DisplacementTexture;        // disp 변위 맵
    FString DecalTexture;               // decal 스텐실 데칼 텍스처
};


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
struct FObjModelData
{
    FName TextureName{ "None" };
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
    static bool DecodeFromFile(const FString& AbsolutePath, FObjModelData& Out);
};