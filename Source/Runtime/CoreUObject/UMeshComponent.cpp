#include "UMeshComponent.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "UClass.h"
#include "Runtime/Engine/UScene.h"

IMPLEMENT_UCLASS(UMeshComponent, UPrimitiveComponent)

void UMeshComponent::Initialize()
{
    Super::Initialize();
}

void UMeshComponent::Register(UScene& InScene)
{
    Super::Register(InScene);
    InScene.AddRenderComponent(this);
}

void UMeshComponent::Unregister()
{
    if (Scene)
    {
        Scene->RemoveRenderComponent(this);
    }
    Super::Unregister();
}

bool UMeshComponent::SetTextureByName(const FName& InTextureName)
{
    RenderData.TextureId = InTextureName;
    return true;
}
