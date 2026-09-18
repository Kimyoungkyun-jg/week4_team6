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
        DefaultMaterialIds.push_back(InMaterialId);
    }
    else if (StaticMeshAsset && !StaticMeshAsset->DefaultTextureId.IsNone() && StaticMeshAsset->DefaultTextureId != FName("None"))
    {
        DefaultMaterialIds.push_back(FName("Textured"));
    }
    else
    {
        DefaultMaterialIds.push_back(FName("Simple"));
    }
}
