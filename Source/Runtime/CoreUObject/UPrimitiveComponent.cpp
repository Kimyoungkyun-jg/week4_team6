#include "UPrimitiveComponent.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Rendering/ShaderConstants.h"
#include "UClass.h"
#include "Runtime/Engine/UScene.h"

IMPLEMENT_UCLASS(UPrimitiveComponent, USceneComponent)

void UPrimitiveComponent::Initialize()
{
    Super::Initialize();
    RenderData.type = ERenderType::Primitive;
}

void UPrimitiveComponent::Register(UScene& InScene)
{
    if (RenderData.type == ERenderType::None)
    {
        RenderData.type = (!RenderData.TextureId.IsNone() && RenderData.TextureId != FName("None"))
            ? ERenderType::Texture
            : ERenderType::Primitive;
    }

    Super::Register(InScene);
    InScene.AddRenderComponent(this);
}

void UPrimitiveComponent::Unregister()
{
    if (Scene)
    {
        Scene->RemoveRenderComponent(this);
    }
    Super::Unregister();
}

void UPrimitiveComponent::SetRelativeTransform(const FTransform& RelativeTransform)
{
    Super::SetRelativeTransform(RelativeTransform);
}

FAxisAlignedBoundingBox UPrimitiveComponent::CalcLocalBounds()
{
    return {};
}

bool UPrimitiveComponent::SetTextureByName(const FName& InTextureName)
{
    RenderData.TextureId = InTextureName;
    return true;
}

FMatrix UPrimitiveComponent::GetModelMatrix()
{
    return GetGlobalTransform().ToMatrix();
}
