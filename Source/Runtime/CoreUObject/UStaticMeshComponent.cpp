#include "UStaticMeshComponent.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "UClass.h"

IMPLEMENT_UCLASS(UStaticMeshComponent, UMeshComponent)

void UStaticMeshComponent::Initialize()
{
    Super::Initialize();
}
