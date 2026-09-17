#include "USphereComp.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "UClass.h"

IMPLEMENT_UCLASS(USphereComp, UPrimitiveComponent)
UCLASS_META(USphereComp, DisplayName, "Sphere")
UCLASS_META(USphereComp, MeshName, "Sphere")

void USphereComp::Initialize() {
  Super::Initialize();
  SetMeshID(FName("Sphere"));
  SetMaterialID(FName("Simple"));
}
