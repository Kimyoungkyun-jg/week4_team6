#include "UStaticMesh.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "UClass.h"

IMPLEMENT_UCLASS(UStaticMesh, UObject)

UStaticMesh::UStaticMesh(const FName& InMeshId, const FName& InMaterialId)
    : MeshId(InMeshId)
{
    StaticMeshAsset = FRenderResourceLibrary::Get().GetMesh(InMeshId);
    if (!StaticMeshAsset)
    {
        return;
    }

    LocalBounds = StaticMeshAsset->GetLocalBounds();
    InitializeFromAsset(InMaterialId);
}

void UStaticMesh::InitializeFromAsset(const FName& InMaterialId)
{
    StaticMaterials.clear();

    if (!StaticMeshAsset)
    {
        SynchronizeCompatibilityArrays();
        return;
    }

    const auto& Sections = StaticMeshAsset->GetSections();

    // 다중 섹션이 존재하는 경우 섹션별로 슬롯 초기화
    if (!Sections.empty())
    {
        for (const auto& Section : Sections)
        {
            FName Diffuse = !Section.DiffuseTextureName.IsNone() ? Section.DiffuseTextureName : Section.TextureName;
            FName Normal = Section.NormalTextureName;
            FName Specular = Section.SpecularTextureName;
            FName MatId = DetermineMaterialId(InMaterialId, Diffuse, Normal, Specular);

            StaticMaterials.push_back(FStaticMaterial{ MatId, Diffuse, Normal, Specular });
        }
    }
    else
    {
        // 단일 메시 처리
        FName Diffuse = StaticMeshAsset->DefaultTextureId;
        FName Normal = StaticMeshAsset->DefaultNormalTextureId;
        FName Specular = StaticMeshAsset->DefaultSpecularTextureId;
        FName MatId = DetermineMaterialId(InMaterialId, Diffuse, Normal, Specular);

        StaticMaterials.push_back(FStaticMaterial{ MatId, Diffuse, Normal, Specular });
    }

    SynchronizeCompatibilityArrays();
}

FName UStaticMesh::DetermineMaterialId(const FName& FallbackMaterialId, const FName& Diffuse, const FName& Normal, const FName& Specular)
{
    if (!FallbackMaterialId.IsNone() && FallbackMaterialId != FName("None") && FallbackMaterialId != FName("Simple"))
    {
        return FallbackMaterialId;
    }

    const bool bHasAnyTexture = (!Diffuse.IsNone() && Diffuse != FName("None")) ||
                                (!Normal.IsNone() && Normal != FName("None")) ||
                                (!Specular.IsNone() && Specular != FName("None"));

    return bHasAnyTexture ? FName("Textured") : FName("Simple");
}

void UStaticMesh::SynchronizeCompatibilityArrays()
{
    DefaultMaterialIds.clear();
    DefaultTextureIds.clear();
    DefaultNormalTextureIds.clear();
    DefaultSpecularTextureIds.clear();

    for (const auto& Mat : StaticMaterials)
    {
        DefaultMaterialIds.push_back(Mat.MaterialId);
        DefaultTextureIds.push_back(Mat.DiffuseTextureId);
        DefaultNormalTextureIds.push_back(Mat.NormalTextureId);
        DefaultSpecularTextureIds.push_back(Mat.SpecularTextureId);
    }
}

int32 UStaticMesh::GetMaterialSlotCount() const
{
    return static_cast<int32>(StaticMaterials.size());
}

const FName& UStaticMesh::GetDefaultMaterialID(int32 Slot) const
{
    static const FName SimpleMat("Simple");
    if (Slot >= 0 && Slot < static_cast<int32>(StaticMaterials.size()) && !StaticMaterials[Slot].MaterialId.IsNone())
    {
        return StaticMaterials[Slot].MaterialId;
    }
    return SimpleMat;
}

const FName& UStaticMesh::GetDefaultTextureID(int32 Slot) const
{
    static const FName NoneName("None");
    if (Slot >= 0 && Slot < static_cast<int32>(StaticMaterials.size()) && !StaticMaterials[Slot].DiffuseTextureId.IsNone())
    {
        return StaticMaterials[Slot].DiffuseTextureId;
    }
    return (StaticMeshAsset && !StaticMeshAsset->DefaultTextureId.IsNone())
        ? StaticMeshAsset->DefaultTextureId : NoneName;
}

const FName& UStaticMesh::GetDefaultNormalTextureID(int32 Slot) const
{
    static const FName NoneName("None");
    if (Slot >= 0 && Slot < static_cast<int32>(StaticMaterials.size()) && !StaticMaterials[Slot].NormalTextureId.IsNone())
    {
        return StaticMaterials[Slot].NormalTextureId;
    }
    return (StaticMeshAsset && !StaticMeshAsset->DefaultNormalTextureId.IsNone())
        ? StaticMeshAsset->DefaultNormalTextureId : NoneName;
}

const FName& UStaticMesh::GetDefaultSpecularTextureID(int32 Slot) const
{
    static const FName NoneName("None");
    if (Slot >= 0 && Slot < static_cast<int32>(StaticMaterials.size()) && !StaticMaterials[Slot].SpecularTextureId.IsNone())
    {
        return StaticMaterials[Slot].SpecularTextureId;
    }
    return (StaticMeshAsset && !StaticMeshAsset->DefaultSpecularTextureId.IsNone())
        ? StaticMeshAsset->DefaultSpecularTextureId : NoneName;
}

void UStaticMesh::SetMaterialSlot(int32 Slot, const FName& InMaterialId, const FName& InDiffuse, const FName& InNormal, const FName& InSpecular)
{
    if (Slot < 0) return;
    if (Slot >= static_cast<int32>(StaticMaterials.size()))
    {
        StaticMaterials.resize(Slot + 1);
    }
    StaticMaterials[Slot] = FStaticMaterial{ InMaterialId, InDiffuse, InNormal, InSpecular };
    SynchronizeCompatibilityArrays();
}

void UStaticMesh::SetDefaultMaterialID(int32 Slot, const FName& InMaterialId)
{
    if (Slot < 0) return;
    if (Slot >= static_cast<int32>(StaticMaterials.size()))
    {
        StaticMaterials.resize(Slot + 1);
    }
    StaticMaterials[Slot].MaterialId = InMaterialId;
    SynchronizeCompatibilityArrays();
}

void UStaticMesh::SetDefaultTextureID(int32 Slot, const FName& InTextureId)
{
    if (Slot < 0) return;
    if (Slot >= static_cast<int32>(StaticMaterials.size()))
    {
        StaticMaterials.resize(Slot + 1);
    }
    StaticMaterials[Slot].DiffuseTextureId = InTextureId;
    SynchronizeCompatibilityArrays();
}

void UStaticMesh::SetDefaultNormalTextureID(int32 Slot, const FName& InTextureId)
{
    if (Slot < 0) return;
    if (Slot >= static_cast<int32>(StaticMaterials.size()))
    {
        StaticMaterials.resize(Slot + 1);
    }
    StaticMaterials[Slot].NormalTextureId = InTextureId;
    SynchronizeCompatibilityArrays();
}

void UStaticMesh::SetDefaultSpecularTextureID(int32 Slot, const FName& InTextureId)
{
    if (Slot < 0) return;
    if (Slot >= static_cast<int32>(StaticMaterials.size()))
    {
        StaticMaterials.resize(Slot + 1);
    }
    StaticMaterials[Slot].SpecularTextureId = InTextureId;
    SynchronizeCompatibilityArrays();
}

const FString& UStaticMesh::GetAssetPathFileName() const
{
    static const FString EmptyString;
    return StaticMeshAsset ? StaticMeshAsset->PathFileName : EmptyString;
}

void UStaticMesh::SetStaticMeshAsset(TSharedPtr<FStaticMesh> InStaticMesh)
{
    StaticMeshAsset = InStaticMesh;
    if (StaticMeshAsset)
    {
        LocalBounds = StaticMeshAsset->GetLocalBounds();
        InitializeFromAsset(FName("None"));
    }
    else
    {
        LocalBounds = FAxisAlignedBoundingBox{};
        StaticMaterials.clear();
        SynchronizeCompatibilityArrays();
    }
}

void UStaticMesh::SetStaticMeshAsset(FStaticMesh* InStaticMesh)
{
    SetStaticMeshAsset(TSharedPtr<FStaticMesh>(InStaticMesh));
}
