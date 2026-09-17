#pragma once

#include "FRenderPipeline.h"
#include "FTexture.h"
#include "Runtime/Core/FString.h"
#include "Runtime/Core/FName.h"
#include "Runtime/Core/PointerTypes.h"
#include "Runtime/Core/TMap.h"
#include "Vertices.h"
#include <d3d11.h>


class FRenderer;
class FRenderResourceLibrary;









class FMaterial final {
  friend class FRenderer;

public:
  FMaterial() = default;
  
  void SetPipeLine(const TSharedPtr<FRenderPipeline>& InPipeline);
  //void SetWireframePipeLine(const TSharedPtr<FRenderPipeline>& InPipeline);

  [[nodiscard]] TSharedPtr<FRenderPipeline> GetPipeline() const { return Pipeline; }

  void SetTexture(const TSharedPtr<FTexture>& InTexture);
  [[nodiscard]] TSharedPtr<FTexture> GetTexture() const { return Texture; }

  // 원본 머터리얼에서 텍스처 교체 함수
  bool SetTextureByName(const FName& InTextureName);


  FName MaterialId{"None"};
private:
  void BindResources(ID3D11DeviceContext &Context) const;

  TSharedPtr<FRenderPipeline> Pipeline;
  TSharedPtr<FRenderPipeline> WireframePipeline;
  // TODO: 텍스처를 여러 개 쓰게 되면 TArray로 바꾸고 슬롯 단위로 바인딩
  TSharedPtr<FTexture> Texture;
};

struct FMaterialDesc {
  FWString VertexShaderFileName;
  FWString PixelShaderFileName;
  bool bEnableDepthTest = true;
};
