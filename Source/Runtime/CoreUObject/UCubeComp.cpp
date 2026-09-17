#include "UCubeComp.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "UClass.h"

IMPLEMENT_UCLASS(UCubeComp, UStaticMeshComponent)
UCLASS_META(UCubeComp, DisplayName, "Cube")
UCLASS_META(UCubeComp, MeshName, "Cube")

void UCubeComp::Initialize() {
  Super::Initialize();
  SetMeshID(FName("Cube"));
  SetMaterialID(FName("Textured"));
}
