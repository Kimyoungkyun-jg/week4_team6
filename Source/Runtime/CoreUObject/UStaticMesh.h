#pragma once

#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/Rendering/FMesh.h"
#include "Runtime/Rendering/FMaterial.h"
#include "Runtime/Geometry/FAxisAlignedBoundingBox.h"


class UStaticMesh : public UObject
{
    GENERATED_BODY()
    DECLARE_UCLASS(UStaticMesh, UObject)

public:
    UStaticMesh() = default;
    UStaticMesh(const FName& InMeshId, const FName& InMaterialId = FName("None"));

    // 렌더 메시
    FName MeshId{"None"};
    TSharedPtr<FMesh> RenderMesh = nullptr;

    // 기본 머티리얼 슬롯
    TArray<FName> DefaultMaterialIds;

    // 바운딩 박스
    FAxisAlignedBoundingBox LocalBounds{};

    // 접근자
    const FAxisAlignedBoundingBox& GetBounds() const { return LocalBounds; }
    
    FName GetDefaultMaterialID(int32 Slot = 0) const {
        return DefaultMaterialIds.empty() ? FName("Simple") : DefaultMaterialIds[Slot];
    }
};
