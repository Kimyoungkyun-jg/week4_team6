#include "UCylinderComp.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "UClass.h"

IMPLEMENT_UCLASS(UCylinderComp, UPrimitiveComponent)
UCLASS_META(UCylinderComp, DisplayName, "Cylinder")
UCLASS_META(UCylinderComp, MeshName, "Cylinder")

void UCylinderComp::Initialize() {
  Super::Initialize();
  SetMeshID(FName("Cylinder"));
  SetMaterialID(FName("Simple"));
}
