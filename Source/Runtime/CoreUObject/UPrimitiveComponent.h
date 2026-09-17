#pragma once

#include "Runtime/Engine/FCamera.h"
#include "Runtime/Geometry/FAxisAlignedBoundingBox.h"
#include "Runtime/Rendering/FRenderQueue.h"
#include "Runtime/Engine/ShowFlags.h"
#include "USceneComponent.h"

class UPrimitiveComponent : public USceneComponent {
  GENERATED_BODY()
  DECLARE_UCLASS(UPrimitiveComponent, USceneComponent)

public:
    void Initialize() override;
    void Register(UScene& InScene) override;
    void Unregister() override;

    virtual FMatrix GetRenderMatrix(const FCamera& Camera) const { return GetGlobalTransform().ToMatrix(); }
    virtual void SetRelativeTransform(const FTransform& RelativeTransform) override;

    // FRenderData 조회 및 설정
    virtual const FRenderData& GetRenderData(const FCamera& Camera){ return RenderData; }
    const FRenderData& GetPureRenderData() const { return RenderData; }


    // ID 접근자
    void SetMeshID(const FName& InMeshId)         { RenderData.MeshId = InMeshId; }
    void SetMaterialID(const FName& InMaterialId) { RenderData.MaterialId = InMaterialId; }
    void SetTextureID(const FName& InTextureId)   { RenderData.TextureId = InTextureId; }
    void SetRenderType(ERenderType InType)       { RenderData.type = InType; }
    const FName& GetMeshID() const               { return RenderData.MeshId; }
    const FName& GetMaterialID() const           { return RenderData.MaterialId; }
    const FName& GetTextureID() const            { return RenderData.TextureId; }
    ERenderType GetRenderType() const            { return RenderData.type; }

    // 충돌 판정용 바운드 계산
    virtual FAxisAlignedBoundingBox CalcLocalBounds();

    // 텍스처 이름으로 머티리얼 텍스처 교체
    bool SetTextureByName(const FName& InTextureName);

    // 색상 설정 및 조회
    const FVector& GetColor() const { return Color; }
    void SetColor(const FVector& InColor) {
        Color = InColor;
        ColorAmount = 1.0f;
    }
    float GetColorAmount() const { return ColorAmount; }
    void SetColorAmount(float InAmount) { ColorAmount = InAmount; }

    virtual EEngineShowFlags GetShowFlag() const { return EEngineShowFlags::SF_Primitives; }

    FMatrix GetModelMatrix();

protected:
    UPrimitiveComponent() = default;

    FRenderData RenderData = {
       .MeshId = FName("None"),
       .MaterialId = FName("None"),
       .TextureId = FName("None"),
       .type = ERenderType::None,
       .bSelected = false,
    };

    FVector Color{1.0f, 1.0f, 1.0f};
    float ColorAmount = 0.0f;
};
