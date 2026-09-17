#pragma once

#include "FFont.h"
#include "FInstanceBatchKey.h"
#include "FMaterial.h"
#include "FMesh.h"
#include "FRenderPipeline.h"
#include "Runtime/Core/FString.h"
#include "Runtime/Core/FName.h"
#include "Runtime/Core/PointerTypes.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/Core/TMap.h"
#include "Vertices.h"
#include "FFont.h"

class FRenderer;
class FTexture;
struct FTextVertex {
  FVector Pos;
  float u, v;
};

class FRenderResourceLibrary final {
public:
  // 전역 싱글톤 접근자
  static FRenderResourceLibrary &Get();

  bool Initialize(FRenderer &Renderer);

  // 파이프라인 보관 맵
  TMap<FName, TSharedPtr<FRenderPipeline>> AllPipelineMap;
  // 메쉬 보관 맵
  TMap<FName, TSharedPtr<FMesh>> AllMeshMap;
  // 머티리얼 보관 맵 (FName 기반)
  TMap<FName, TSharedPtr<FMaterial>> AllMaterialMap;
  // 텍스쳐 보관 맵 (FName 기반)
  TMap<FName, TSharedPtr<FTexture>> AllTextureMap;
  // 폰트 보관 맵
  TMap<FName, TSharedPtr<FFont>> AllFontMap;

  // 에디터용 아이콘 텍스쳐 보관 맵
  TMap<FString, TSharedPtr<FTexture>> AllEditorTextureMap;

  // 인스턴싱 배치 배열 맵
  TMap<FInstanceBatchKey, TArray<FInstanceData>> AllInstancingArrayMap;

  // 인스턴싱 배열 조회
  TArray<FInstanceData>& GetInstancingArray(const FName& MatId, const FName& MeshId) {
    return AllInstancingArrayMap[{MatId, MeshId}];
  }

  // 파이프라인 조회
  [[nodiscard]] TSharedPtr<FRenderPipeline> GetPipeline(const FName& Id) const {
    auto it = AllPipelineMap.find(Id);
    if (it != AllPipelineMap.end())
      return it->second;
    return nullptr;
  }

  // 머티리얼 조회
  [[nodiscard]] TSharedPtr<FMaterial> GetMaterial(const FName& Id) const {
    auto it = AllMaterialMap.find(Id);
    if (it != AllMaterialMap.end())
      return it->second;
    return nullptr;
  }

  // 편집용 머티리얼 조회
  [[nodiscard]] TSharedPtr<FMaterial> GetEditMaterial(const FName& Id) const {
      auto it = AllMaterialMap.find(Id);
      if (it != AllMaterialMap.end())
          return it->second;
      return nullptr;
  }

  // 메쉬 조회
  TSharedPtr<FMesh> GetMesh(const FName &ID) const {
    auto it = AllMeshMap.find(ID);
    if (it != AllMeshMap.end())
      return it->second;
    return nullptr;
  }

  // 메쉬 등록
  TSharedPtr<FMesh> RegisterMesh(const FName &ID, TSharedPtr<FMesh> inMesh) {
    inMesh->MeshId = ID;
    AllMeshMap[ID] = inMesh;
    return inMesh;
  }

  // 개별 메쉬 접근자
  [[nodiscard]] TSharedPtr<FMesh> GetCubeMesh() const {
    return GetMesh(FName("Cube"));
  }
  [[nodiscard]] TSharedPtr<FMesh> GetCylinderMesh() const {
    return GetMesh(FName("Cylinder"));
  }
  [[nodiscard]] TSharedPtr<FMesh> GetConeMesh() const {
    return GetMesh(FName("Cone"));
  }
  [[nodiscard]] TSharedPtr<FMesh> GetSpotlightConeMesh() const {
    return GetMesh(FName("SpotlightCone"));
  }
  [[nodiscard]] TSharedPtr<FMesh> GetArrowMesh() const {
    return GetMesh(FName("Arrow"));
  }
  [[nodiscard]] TSharedPtr<FMesh> GetCircleMesh() const {
    return GetMesh(FName("Circle"));
  }
  [[nodiscard]] TSharedPtr<FMesh> GetRotationGizmoMesh() const {
    return GetMesh(FName("RotGizmo"));
  }
  [[nodiscard]] TSharedPtr<FMesh> GetSquareArrowMesh() const {
    return GetMesh(FName("SquareArrow"));
  }
  [[nodiscard]] TSharedPtr<FMesh> GetGridMesh() const {
    return GetMesh(FName("Grid"));
  }
  [[nodiscard]] TSharedPtr<FMesh> GetSphereMesh() const {
    return GetMesh(FName("Sphere"));
  }
  [[nodiscard]] TSharedPtr<FMesh> GetLineMesh() const {
    return GetMesh(FName("Line"));
  }
  [[nodiscard]] TSharedPtr<FMesh> GetPlaneMesh() const {
    return GetMesh(FName("Plane"));
  }
  [[nodiscard]] TSharedPtr<FMesh> GetRectMesh() const {
    return GetMesh(FName("Rect"));
  }
  [[nodiscard]] TSharedPtr<FMesh> GetTextMesh() const {
    return GetMesh(FName("TextMesh"));
  }
  [[nodiscard]] TSharedPtr<FMesh> GetMasterYiMesh() const {
    return GetMesh(FName("MasterYi"));
  }
  [[nodiscard]] TSharedPtr<FMesh> GetMasteryMesh() const {
    return GetMesh(FName("MasterYi"));
  }

  // 머티리얼 등록
  TSharedPtr<FMaterial> RegisterMaterial(const FName& Id, TSharedPtr<FMaterial> inMaterial);

  void RegisterTexture(const FName &name, TSharedPtr<FTexture> texture) {
    AllTextureMap[name] = texture;
  }

  void RegisterEditTexture(const FString &name, TSharedPtr<FTexture> texture) {
    AllEditorTextureMap[name] = texture;
  }

  // 텍스처 조회
  [[nodiscard]] TSharedPtr<FTexture> GetTexture(const FName &name) const {
    auto it = AllTextureMap.find(name);
    if (it != AllTextureMap.end())
      return it->second;
    return nullptr;
  }

  [[nodiscard]] TSharedPtr<FTexture> GetEditTexture(const FString &name) const {
    auto it = AllEditorTextureMap.find(name);
    if (it != AllEditorTextureMap.end())
      return it->second;
    return nullptr;
  }

  // 메쉬 전체 해제
  void DestroyAllMeshes() { AllMeshMap.clear(); }

  // 머티리얼 전체 해제
  void DestroyAllMaterials() { AllMaterialMap.clear(); }

  // 파이프라인 전체 해제
  void DestroyAllPipelines() { AllPipelineMap.clear(); }

  void DestroyAllInstancingArray() { AllInstancingArrayMap.clear(); }

  // 전체 머티리얼 맵 조회
  const TMap<FName, TSharedPtr<FMaterial>> &GetAllMaterials() const {
    return AllMaterialMap;
  }

  // 렌더러 참조 조회
  FRenderer *GetRenderer() const { return RendererRef; }

  // 정점 배열 메쉬 캐싱 생성
  TSharedPtr<FMesh> GetOrCreateMesh(const FName &ID,
                                    const TArray<FVertexData> &vertices);

  

  [[nodiscard]] TSharedPtr<FFont> GetFont(const FName& InName) const {
      auto it = AllFontMap.find(InName);
      if (it != AllFontMap.end())
          return it->second;
      return nullptr;
  }


private:
  bool InitializePipelines(FRenderer &Renderer);
  bool CreateSolidWireframePipeline(FRenderer &Renderer);
  bool CreateOutlinePipeline(FRenderer &Renderer);
  bool CreatePostProcessPipeline(FRenderer &Renderer);

  bool CreateCubeMesh(FRenderer &Renderer);
  bool CreateCylinderMesh(FRenderer &Renderer, float Height, uint32 SliceCount,
                          float TopRadius, float BottomRadius);
  bool CreateConeMesh(FRenderer &Renderer);
  bool CreateSpotlightConeMesh(FRenderer &Renderer);
  bool CreateArrowMesh(FRenderer &Renderer);
  bool CreateCircleMesh(FRenderer &Renderer);
  bool CreateRotationGizmoMesh(FRenderer &Renderer);
  bool CreateSquareArrowMesh(FRenderer &Renderer);
  bool CreateGridMesh(FRenderer &Renderer);
  bool CreateSphereMesh(FRenderer &Renderer);
  bool CreateLineMesh(FRenderer &Renderer);
  bool CreatePlaneMesh(FRenderer &Renderer);
  bool CreateRectMesh(FRenderer &Renderer);
  bool CreateMasterYiMesh(FRenderer &Renderer);
  bool CreateMasteryMesh(FRenderer &Renderer) { return CreateMasterYiMesh(Renderer); }

  bool CreateInstancingArrayMap();
  bool CreateOutlinePipeline(); //아웃라인용

  // 텍스처 및 머티리얼 일괄 초기화
  bool CreateTextures(FRenderer &Renderer);
  bool InitializeMaterials(FRenderer &Renderer);
  bool CreateEditTextures(FRenderer &Renderer);

  // 폰트 일괄 초기화
  bool CreateFonts(FRenderer& Renderer);

  FRenderer *RendererRef = nullptr;
};
