#include "AMasterYi.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"

IMPLEMENT_UCLASS(AMasterYi, AActor)
UCLASS_META(AMasterYi, DisplayName, "MasterYi Actor")

AMasterYi::AMasterYi()
{
	// 프리미티브 컴포넌트 생성 및 루트 장착
	CreateRootComponent(UPrimitiveComponent::StaticClass());

	if (auto* PrimComp = GetPrimitiveComponent())
	{
		PrimComp->SetMeshID(FName("MasterYi"));
		PrimComp->SetMaterialID(FName("Textured"));
		PrimComp->SetTextureID(FName("MasterYi_Head"));
		PrimComp->SetRenderType(ERenderType::Texture);
	}

	FTransform DefaultTransform;
	DefaultTransform.Scale3D = FVector(5.0f, 5.0f, 5.0f);
	SetTransform(DefaultTransform);
}

UPrimitiveComponent* AMasterYi::GetPrimitiveComponent() const
{
	return RootComponent ? RootComponent->Cast<UPrimitiveComponent>() : nullptr;
}
