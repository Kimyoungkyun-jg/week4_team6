#include "UStaticMesh.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "UClass.h"

IMPLEMENT_UCLASS(UStaticMesh, UObject)

UStaticMesh::UStaticMesh(const FName& InMeshId, const FName& InMaterialId)
    : MeshId(InMeshId)
{
    StaticMeshAsset = FRenderResourceLibrary::Get().GetMesh(InMeshId);
    if (StaticMeshAsset)
    {
        LocalBounds = StaticMeshAsset->GetLocalBounds();
    }

    if (!InMaterialId.IsNone() && InMaterialId != FName("None"))
    {
        MaterialIds.push_back(InMaterialId);
    }
    else if (StaticMeshAsset && !StaticMeshAsset->DefaultTextureId.IsNone() && StaticMeshAsset->DefaultTextureId != FName("None"))
    {
        MaterialIds.push_back(FName("Textured"));
    }
    else
    {
        MaterialIds.push_back(FName("Simple"));
    }
}


void UStaticMesh::InitMaterialIds()
{
    if (StaticMeshAsset && StaticMeshAsset->Sections.size())
    {
        MaterialIds.clear();
        const TArray<FMeshSection>& Sections = StaticMeshAsset->Sections;
        const TArray<FName>& ObjMaterialIdList = StaticMeshAsset->ObjMaterialIdList;
        for (const FMeshSection& Section : Sections)
        {
            if (0 <= Section.MaterialIndex && Section.MaterialIndex < static_cast<int32>(ObjMaterialIdList.size()))
            {
                MaterialIds.push_back(ObjMaterialIdList[Section.MaterialIndex]);
            }
            else
            {
                MaterialIds.push_back(FName("Simple"));
            }
            assert(Section.MaterialIndex < ObjMaterialIdList.size());
        }
        assert(MaterialIds.size() == Sections.size());
    }
}
