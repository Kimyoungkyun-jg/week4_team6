#pragma once

#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/Rendering/FMesh.h"
#include "Runtime/Rendering/FMaterial.h"
#include "Runtime/Geometry/FAxisAlignedBoundingBox.h"


class FStaticMesh;

class UStaticMesh : public UObject
{
    GENERATED_BODY()
    DECLARE_UCLASS(UStaticMesh, UObject)

public:
    UStaticMesh() = default;
    UStaticMesh(const FName& InMeshId, const FName& InMaterialId = FName("None"));

    // 렌더 메시
    FName MeshId{"None"};
    TSharedPtr<FStaticMesh> StaticMeshAsset = nullptr;

    // 기본 머티리얼 슬롯
    TArray<FName> DefaultMaterialIds;

    // 바운딩 박스
    FAxisAlignedBoundingBox LocalBounds{};

    // 접근자
    const FAxisAlignedBoundingBox& GetBounds() const { return LocalBounds; }
    
    TSharedPtr<FStaticMesh> GetStaticMeshAsset() const { return StaticMeshAsset; }

    FName GetDefaultTextureID() const {
        return (StaticMeshAsset && !StaticMeshAsset->DefaultTextureId.IsNone()) 
            ? StaticMeshAsset->DefaultTextureId : FName("None");
    }

    const FName& GetDefaultMaterialID(int32 Slot = 0) const {
        static const FName SimpleMat("Simple");
        if (Slot >= 0 && Slot < static_cast<int32>(DefaultMaterialIds.size()) && !DefaultMaterialIds[Slot].IsNone())
        {
            return DefaultMaterialIds[Slot];
        }
        return SimpleMat;
    }


    const FString& GetAssetPathFileName() {
        return StaticMeshAsset->PathFileName;
    }

    void SetStaticMeshAsset(TSharedPtr<FStaticMesh> InStaticMesh) {
        StaticMeshAsset = InStaticMesh;
    }

    void SetStaticMeshAsset(FStaticMesh* InStaticMesh) {
        StaticMeshAsset = TSharedPtr<FStaticMesh>(InStaticMesh);
    }
};
