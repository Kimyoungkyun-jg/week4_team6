#pragma once

#include "FMaterial.h"
#include "FMesh.h"
#include "FRenderPipeline.h"
#include "FRenderResourceLibrary.h"
#include "Runtime/Core/IntTypes.h"
#include "Runtime/Core/PointerTypes.h"
#include "Runtime/Core/TMap.h"
#include "Runtime/Math/FVector2.h"
#include "Runtime/Rendering/FLineBatcher.h"
#include "Runtime/Core/IntTypes.h"
#include "ShaderConstants.h"
#include "Vertices.h"

#include <Windows.h>
#include <d3d11.h>
#include <filesystem>
#include <wrl/client.h>

class FTexture;
struct FTextureDesc;
struct FCamera;
class UTextInstanceComponent;

inline FWString GetExecutableDirectory() {
  wchar_t Buffer[256];
  GetModuleFileNameW(nullptr, Buffer, 256);
  return std::filesystem::path(Buffer).parent_path();
}

#include "Runtime/Engine/ShowFlags.h"


class FRenderer final {
public:
  bool Initialize(HWND Window);
  void Shutdown();
  void BeginFrame();
  void BindEditorViewportRenderTargets();
  void SetViewportUV(FVector2 TopLeftUV, FVector2 LengthUV);
  void ClearDepth();
  void SwapBuffer();
  void OnWindowSize(UINT Width, UINT Height);

  EViewModeIndex GetRenderMode() const { return CurrentRenderMode; }
  void SetRenderMode(EViewModeIndex InMode) { CurrentRenderMode = InMode; }

  [[nodiscard]]
  TSharedPtr<FMesh> CreateMesh(const FMeshDesc &Desc);
  [[nodiscard]]
  TSharedPtr<FMesh> CreateDynamicMesh(const FMeshDesc &Desc); // 텍스트 렌더링용
  [[nodiscard]]
  TSharedPtr<FMaterial> CreateMaterial(const FMaterialDesc &Desc);

  void GetDeviceAndContext_ImplDX11(ID3D11Device *&DeviceOut,
                                    ID3D11DeviceContext *&ContextOut);
  [[nodiscard]] ID3D11Device *GetDevice() const { return Device.Get(); }
  [[nodiscard]] ID3D11DeviceContext *GetContext() const {
    return Context.Get();
  }

  [[nodiscard]]
  TSharedPtr<FRenderPipeline>
  CreateRenderPipeline(const FRenderPipelineDesc &Desc,
                       EViewModeIndex RenderMode = EViewModeIndex::VMI_Lit);
  [[nodiscard]]
  TSharedPtr<FTexture> CreateTexture(const wchar_t* path);
  // 파이프라인 조회
  [[nodiscard]]
  TSharedPtr<FRenderPipeline> GetPipeline(const FName& Id) const;

  FLineBatcher &GetLineBatcher() { return LineBatcher; }

  void UpdateLightConstants(const FLightConstants &Constants, const EViewModeIndex InMode);

  // 텍스트 인스턴싱
  void AddTextInstanceArray(const TArray<FInstanceData>& Instances, const FName& MeshId, const FName& MaterialId);
  void DrawInstances(const FCamera& Camera);
  void DrawTextInstances(const FCamera& Camera, const FName& MeshId, const FName& MaterialId);
  void ClearTextInstances();

  void RenderOutline();
  ID3D11RenderTargetView* GetBackBuffer() { return BackBufferRTV.Get(); }


private:
  bool InitializeDeviceAndSwapChain(HWND Window);
  bool InitializeBackBufferAndDepthStencil();
  bool InitializeConstantBuffers();

private:
  FLineBatcher LineBatcher;
  Microsoft::WRL::ComPtr<ID3D11Device> Device;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> Context;
  Microsoft::WRL::ComPtr<IDXGISwapChain> SwapChain;
  D3D11_VIEWPORT Viewport{};

  Microsoft::WRL::ComPtr<ID3D11RenderTargetView> BackBufferRTV;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> DepthStencilBuffer;
  Microsoft::WRL::ComPtr<ID3D11DepthStencilView> DepthStencilView;

  // b0에 바인딩되는 모든 상수 타입이 공유한다.
  static constexpr UINT ConstantBufferSize = 256u;
  Microsoft::WRL::ComPtr<ID3D11Buffer> b0ConstantBuffer;

  // b1뷰포트 단위. b0와 동시에 바인딩되므로 별도 버퍼가 필요하다.
  Microsoft::WRL::ComPtr<ID3D11Buffer> FrameConstantBuffer;

  // b2에 할당되는 lightbuffer
  Microsoft::WRL::ComPtr<ID3D11Buffer> LightConstantBuffer;



  Microsoft::WRL::ComPtr<ID3D11RenderTargetView> EditorViewPortRTV;
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> EditorViewPortSRV;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> renderTexture;
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> DepthStencilSRV;

  bool InitializeEditorViewportRenderTarget();

  // 텍스트 인스턴싱 버퍼

  Microsoft::WRL::ComPtr<ID3D11Buffer> InstanceBuffer;
  UINT TextInstanceBufferSize = 0;

  EViewModeIndex CurrentRenderMode = EViewModeIndex::VMI_Lit;
  
public:
  template <typename TConstants>
  void FlushLineBatch(
      const TConstants &Constants,
      const FName& PipelineId = FName("Simple_Line")
  ) {
    UpdateBuffer(Constants);
    LineBatcher.Flush(*Context.Get(), GetPipeline(PipelineId));
  }

  // bApplyViewMode=false면 뷰모드(와이어프레임) 오버라이드를 건너뛴다
  template <typename TConstants>
  void Draw(
      const FMesh &Mesh,
      const FMaterial &Material,
      const TConstants &Constants,
      uint32 Slot = 0,
      bool bApplyViewMode = true
  )
  {
    UpdateBuffer(Constants, Slot);

    TSharedPtr<FRenderPipeline> Pipeline = Material.Pipeline;
    if (bApplyViewMode && CurrentRenderMode == EViewModeIndex::VMI_Wireframe) {
      Pipeline = GetPipeline(FName("Simple_Wireframe"));
    }
    if (Pipeline) {
      Pipeline->Bind(*Context.Get());
    }

    Material.BindResources(*Context.Get());
    Mesh.BindResources(*Context.Get());

    if (Mesh.HasIndices()) {
      Context->DrawIndexed(Mesh.IndexCount, 0, 0);
    } else {
      Context->Draw(Mesh.VertexCount, 0);
    }
  }


private:
  // 어느 상수 타입이든 b0 버퍼 하나에 써 넣는다.
  // 크기가 맞는지는 컴파일 타임에 검사한다.
  template <typename TConstants>
  void UpdateBuffer(const TConstants &Constants, uint32 Slot = 0u) {
    static_assert(sizeof(TConstants) <= ConstantBufferSize);
    static_assert(sizeof(TConstants) % 16 == 0);

    // 언리얼 Clip -> D3D Clip 좌표 변환.
    // MVP를 가진 상수 타입에만 적용한다(없는 타입은 그대로 통과).
    TConstants ShaderConstants = Constants;
    if constexpr (requires { ShaderConstants.MVP; }) {
      static const FMatrix UnrealClipToD3DClip{
          FVector{0.0f, 0.0f, 1.0f}, FVector{1.0f, 0.0f, 0.0f},
          FVector{0.0f, 1.0f, 0.0f}, FVector{0.0f, 0.0f, 0.0f}};
      ShaderConstants.MVP *= UnrealClipToD3DClip;
    }

    // 언리얼 Clip -> D3D Clip 좌표 변환.
    // MVP를 가진 상수 타입에만 적용한다(없는 타입은 그대로 통과).
    if constexpr (requires { ShaderConstants.VP; }) {
      static const FMatrix UnrealClipToD3DClip{
          FVector{0.0f, 0.0f, 1.0f}, FVector{1.0f, 0.0f, 0.0f},
          FVector{0.0f, 1.0f, 0.0f}, FVector{0.0f, 0.0f, 0.0f}};
      ShaderConstants.VP *= UnrealClipToD3DClip;
    }

    D3D11_MAPPED_SUBRESOURCE Mapped{};
    if (FAILED(Context->Map(b0ConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD,
                            0, &Mapped))) {
      return;
    }
    std::memcpy(Mapped.pData, &ShaderConstants, sizeof(TConstants));
    Context->Unmap(b0ConstantBuffer.Get(), 0);

    Context->VSSetConstantBuffers(Slot, 1u, b0ConstantBuffer.GetAddressOf());
    Context->PSSetConstantBuffers(Slot, 1u, b0ConstantBuffer.GetAddressOf());
  }
};
