#pragma once

#include "UPrimitiveComponent.h"

class UMeshComponent : public UPrimitiveComponent {
    GENERATED_BODY()
    DECLARE_UCLASS(UMeshComponent, UPrimitiveComponent)

public:
    void Initialize() override;
    void Register(UScene& InScene) override;

    // FRenderData 조회
    virtual const FRenderData& GetRenderData(const FCamera& Camera) override { return RenderData; }
    virtual const FRenderData& GetPureRenderData() const override { return RenderData; }

    // ID 접근자
    void SetMeshID(const FName& InMeshId)         { RenderData.MeshId = InMeshId; }
    void SetMaterialID(const FName& InMaterialId) { RenderData.MaterialId = InMaterialId; }
    void SetTextureID(const FName& InTextureId)   { RenderData.TextureId = InTextureId; }
    void SetRenderType(ERenderType InType)       { RenderData.type = InType; }
    const FName& GetMeshID() const               { return RenderData.MeshId; }
    const FName& GetMaterialID() const           { return RenderData.MaterialId; }
    const FName& GetTextureID() const            { return RenderData.TextureId; }
    ERenderType GetRenderType() const            { return RenderData.type; }

    // 텍스처 이름으로 머티리얼 텍스처 교체
    bool SetTextureByName(const FName& InTextureName);

protected:
    UMeshComponent() = default;

    FRenderData RenderData = {
       .MeshId = FName("None"),
       .MaterialId = FName("None"),
       .TextureId = FName("None"),
       .type = ERenderType::None,
       .bSelected = false,
    };
};
