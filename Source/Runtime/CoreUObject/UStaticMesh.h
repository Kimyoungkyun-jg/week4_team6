#pragma once

#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/Rendering/FMesh.h"
#include "Runtime/Rendering/FMaterial.h"
#include "Runtime/Geometry/FAxisAlignedBoundingBox.h"

class FStaticMesh;

// 머티리얼 슬롯 정보
struct FStaticMaterial
{
    FName MaterialId{ "Simple" };
    FName DiffuseTextureId{ "None" };
    FName NormalTextureId{ "None" };
    FName SpecularTextureId{ "None" };
};

class UStaticMesh : public UObject
{
    GENERATED_BODY()
    DECLARE_UCLASS(UStaticMesh, UObject)

public:
    UStaticMesh() = default;
    UStaticMesh(const FName& InMeshId, const FName& InMaterialId = FName("None"));

    // 렌더 메시
    FName MeshId{ "None" };
    TSharedPtr<FStaticMesh> StaticMeshAsset = nullptr;

    // 슬롯 목록
    TArray<FStaticMaterial> StaticMaterials;


    // 바운딩 박스
    FAxisAlignedBoundingBox LocalBounds{};

    // 유효성 확인
    bool IsValid() const { return StaticMeshAsset != nullptr; }

    // 접근자
    const FAxisAlignedBoundingBox& GetBounds() const { return LocalBounds; }
    TSharedPtr<FStaticMesh> GetStaticMeshAsset() const { return StaticMeshAsset; }
    int32 GetMaterialSlotCount() const;

    const FName& GetDefaultMaterialID(int32 Slot = 0) const;
    const FName& GetDefaultTextureID(int32 Slot = 0) const;
    const FName& GetDefaultNormalTextureID(int32 Slot = 0) const;
    const FName& GetDefaultSpecularTextureID(int32 Slot = 0) const;

    // 슬롯 설정 메서드
    void SetMaterialSlot(int32 Slot, const FName& InMaterialId, const FName& InDiffuse = FName("None"), const FName& InNormal = FName("None"), const FName& InSpecular = FName("None"));
    void SetDefaultMaterialID(int32 Slot, const FName& InMaterialId);
    void SetDefaultTextureID(int32 Slot, const FName& InTextureId);
    void SetDefaultNormalTextureID(int32 Slot, const FName& InTextureId);
    void SetDefaultSpecularTextureID(int32 Slot, const FName& InTextureId);

    // 에셋 정보 조회
    const FString& GetAssetPathFileName() const;

    // 메시 에셋 설정
    void SetStaticMeshAsset(TSharedPtr<FStaticMesh> InStaticMesh);
    void SetStaticMeshAsset(FStaticMesh* InStaticMesh);

private:
    void InitializeFromAsset(const FName& InMaterialId);
    static FName DetermineMaterialId(const FName& FallbackMaterialId, const FName& Diffuse, const FName& Normal, const FName& Specular);
};
