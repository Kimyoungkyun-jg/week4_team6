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
    return GetMaterial(0);
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

    if (!StaticMesh || !StaticMesh->StaticMeshAsset)
    {
        return OutDatas;
    }


    const FName CurrentMeshId = GetMeshID();

    if (!StaticMesh || !StaticMesh->StaticMeshAsset)
    {
        return OutDatas;
    }

    const auto& Sections = StaticMesh->StaticMeshAsset->Sections;

    if (!Sections.empty())
    {
        const size_t Count = std::min(StaticMesh->Materials.size(), Sections.size());
        OutDatas.reserve(Count);

        for (size_t i = 0; i < Count; ++i)
        {
            FRenderData rdata;
            rdata.MeshId = CurrentMeshId;
            rdata.MaterialId = GetMaterial(static_cast<int32>(i));

            rdata.startidx = Sections[i].FirstIndex;
            rdata.indicesCount = Sections[i].IndexCount;

            if (bIsMovingUV)
            {
                offset = fmodf(offset + 0.1f, 1.0f);
                rdata.Constants.UVOffset.X = offset;
            }


            OutDatas.push_back(std::move(rdata));
        }
    }
    else
    {
        FRenderData rdata;
        rdata.MeshId = CurrentMeshId;
        rdata.MaterialId = GetMaterial(0);

        rdata.startidx = 0;
        rdata.indicesCount = -1; 

        if (bIsMovingUV)
        {
            offset = fmodf(offset + 0.1f, 1.0f);
            rdata.Constants.UVOffset.X = offset;
        }

        OutDatas.push_back(std::move(rdata));
    }

    return OutDatas;
}

const FRenderData& UStaticMeshComponent::GetPureRenderData() const
{
    FRenderData& MutableData = const_cast<FRenderData&>(RenderDatas.at(0));
    MutableData.MeshId = GetMeshID();
    MutableData.MaterialId = GetMaterial(0);
    MutableData.startidx = 0;
    MutableData.indicesCount = (StaticMesh && StaticMesh->StaticMeshAsset)
        ? StaticMesh->StaticMeshAsset->GetIndexCount()
        : -1;

    return MutableData;
}

void UStaticMeshComponent::SetMaterial(int32 Slot, const FName& InMaterialId)
{
    if (Slot < 0) return;
    if (Slot >= static_cast<int32>(OverrideMaterials.size()))
    {
        OverrideMaterials.resize(Slot + 1, FName("None"));
    }

    OverrideMaterials[Slot] = InMaterialId;
}

FName UStaticMeshComponent::GetMaterial(int32 Slot) const
{
    if (Slot >= 0 && Slot < static_cast<int32>(OverrideMaterials.size()) && !OverrideMaterials[Slot].IsNone())
    {
        return OverrideMaterials[Slot];
    }

    if (StaticMesh)
    {
        return StaticMesh->Materials[Slot];
    }

    return FName("Simple");
}