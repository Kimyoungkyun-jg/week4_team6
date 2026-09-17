#include "AInstancingActor.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/CoreUObject/UInstancePrimitiveComponent.h"

IMPLEMENT_UCLASS(AInstancingActor, AActor)
UCLASS_META(AInstancingActor, DisplayName, "Instancing Actor")

AInstancingActor::AInstancingActor()
{
	CreateRootComponent(UInstancePrimitiveComponent::StaticClass());

	if (auto* PrimComp = GetRootComponent()->Cast<UInstancePrimitiveComponent>())
	{
		PrimComp->SetMeshID(FName("MasterYi"));
		PrimComp->SetMaterialID(FName("Instance_Textured"));
		PrimComp->SetTextureID(FName("MasterYi_Head"));
	}

	FTransform DefaultTransform;
	DefaultTransform.Scale3D = FVector(5.0f, 5.0f, 5.0f);
	SetTransform(DefaultTransform);

}
