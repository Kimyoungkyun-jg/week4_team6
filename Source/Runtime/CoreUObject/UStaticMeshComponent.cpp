#include "UStaticMeshComponent.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "UClass.h"

IMPLEMENT_UCLASS(UStaticMeshComponent, UMeshComponent)

void UStaticMeshComponent::Initialize()
{
    Super::Initialize();
}

bool UStaticMeshComponent::SetStaticMesh(UStaticMesh* InStaticMesh)
{
    StaticMesh = InStaticMesh;
    if (StaticMesh)
    {
        RenderData.MeshId = StaticMesh->MeshId;
        RenderData.MaterialId = GetMaterialID();

        FName TexId = StaticMesh->GetDefaultTextureID();
        SetTextureID(!TexId.IsNone() ? TexId : FName("None"));

        CalcLocalBounds();
    }
    return true;
}

const FName& UStaticMeshComponent::GetMeshID() const
{
    if (StaticMesh && !StaticMesh->MeshId.IsNone())
    {
        return StaticMesh->MeshId;
    }
    return Super::GetMeshID();
}

const FName& UStaticMeshComponent::GetMaterialID() const
{
    static const FName SimpleMat("Simple");
    if (!OverrideMaterials.empty() && !OverrideMaterials[0].IsNone())
    {
        return OverrideMaterials[0];
    }
    if (StaticMesh)
    {
        return StaticMesh->GetDefaultMaterialID(0);
    }
    return Super::GetMaterialID();
}

FAxisAlignedBoundingBox UStaticMeshComponent::CalcLocalBounds()
{
    if (StaticMesh)
    {
        return StaticMesh->GetBounds();
    }
    return Super::CalcLocalBounds();
}

const FRenderData& UStaticMeshComponent::GetRenderData(const FCamera& Camera)
{
    RenderData.MeshId = GetMeshID();
    RenderData.MaterialId = GetMaterialID();

    if (StaticMesh)
    {
        FName TexId = StaticMesh->GetDefaultTextureID();
        RenderData.TextureId = !TexId.IsNone() ? TexId : FName("None");
    }

    return RenderData;
}

const FRenderData& UStaticMeshComponent::GetPureRenderData() const
{
    FRenderData& MutableData = const_cast<FRenderData&>(RenderData);
    MutableData.MeshId = GetMeshID();
    MutableData.MaterialId = GetMaterialID();

    if (StaticMesh)
    {
        FName TexId = StaticMesh->GetDefaultTextureID();
        MutableData.TextureId = !TexId.IsNone() ? TexId : FName("None");
    }

    return RenderData;
}

void UStaticMeshComponent::SetMaterial(int32 Slot, const FName& InMaterialId)
{
    if (Slot < 0) return;
    if (Slot >= static_cast<int32>(OverrideMaterials.size()))
    {
        OverrideMaterials.resize(Slot + 1, FName("None"));
    }
    OverrideMaterials[Slot] = InMaterialId;
    RenderData.MaterialId = InMaterialId;
}

FName UStaticMeshComponent::GetMaterial(int32 Slot) const
{
    if (Slot >= 0 && Slot < static_cast<int32>(OverrideMaterials.size()) && !OverrideMaterials[Slot].IsNone())
    {
        return OverrideMaterials[Slot];
    }
    if (StaticMesh)
    {
        return StaticMesh->GetDefaultMaterialID(Slot);
    }
    return FName("None");
}
