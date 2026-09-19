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
        RenderDatas.at(0).MeshId = StaticMesh->MeshId;
        RenderDatas.at(0).MaterialId = GetMaterialID();

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

TArray<FRenderData> UStaticMeshComponent::GetRenderDatas(const FCamera& Camera)
{
    TArray<FRenderData> OutDatas;

    if (StaticMesh && StaticMesh->StaticMeshAsset)
    {
        const auto& Sections = StaticMesh->StaticMeshAsset->GetSections();
        const size_t MaterialCount = StaticMesh->StaticMaterials.size();

        OutDatas.reserve(MaterialCount);

        for (size_t i = 0; i < MaterialCount; ++i)
        {
            FRenderData rdata;
            rdata.MeshId = GetMeshID();

            // 컴포넌트 오버라이드 머티리얼이 있으면 반영, 없으면 에셋 기본 머티리얼
            rdata.MaterialId = GetMaterial(static_cast<int32>(i));

            rdata.TextureId = StaticMesh->StaticMaterials[i].DiffuseTextureId;
            rdata.NormalTextureId = StaticMesh->StaticMaterials[i].NormalTextureId;
            rdata.SpecularTextureId = StaticMesh->StaticMaterials[i].SpecularTextureId;

            if (i < Sections.size())
            {
                rdata.startidx = Sections[i].FirstIndex;
                rdata.indicesCount = Sections[i].IndexCount;
            }
            else
            {
                rdata.startidx = 0;
                rdata.indicesCount = StaticMesh->StaticMeshAsset->GetIndexCount();
            }

            OutDatas.push_back(std::move(rdata));
        }
    }

    return OutDatas;
}



const FRenderData& UStaticMeshComponent::GetPureRenderData() const
{
    FRenderData& MutableData = const_cast<FRenderData&>(RenderDatas.at(0));
    MutableData.MeshId = GetMeshID();
    MutableData.MaterialId = GetMaterialID();

    if (StaticMesh)
    {
        FName TexId = StaticMesh->GetDefaultTextureID();
        MutableData.TextureId = !TexId.IsNone() ? TexId : FName("None");
        MutableData.NormalTextureId = StaticMesh->GetDefaultNormalTextureID();
        MutableData.SpecularTextureId = StaticMesh->GetDefaultSpecularTextureID();
    }

    return RenderDatas.at(0);
}

void UStaticMeshComponent::SetMaterial(int32 Slot, const FName& InMaterialId)
{
    if (Slot < 0 || Slot > RenderDatas.size()) return;
    if (Slot >= static_cast<int32>(OverrideMaterials.size()))
    {
        OverrideMaterials.resize(Slot + 1, FName("None"));
    }

    OverrideMaterials[Slot] = InMaterialId;
    RenderDatas[Slot].MaterialId = OverrideMaterials[Slot];
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
