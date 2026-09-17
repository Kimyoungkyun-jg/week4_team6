#include "FEditorApplication.h"

#include "Runtime/CoreUObject/FGarbageCollector.h"
#include "Runtime/CoreUObject/FReferenceCollector.h"
#include "Runtime/CoreUObject/UAnimatedBillboardComp.h"
#include "Runtime/CoreUObject/UBillBoardComp.h"
#include "Runtime/CoreUObject/UCubeComp.h"
#include "Runtime/CoreUObject/UCylinderComp.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/CoreUObject/UPrimitiveComponent.h"
#include "Runtime/CoreUObject/USpotLightComponent.h"
#include "Runtime/Engine/FRayCastingManager.h"
#include "Runtime/Geometry/FAxisAlignedBoundingBox.h"
#include "Runtime/Math/FMatrix.h"
#include "Runtime/Rendering/FMesh.h"
#include <Windows.h>

#include "Runtime/Engine/FSceneView.h"

#include "Runtime/Actors/AActor.h"
#include "Runtime/Actors/AInstancingActor.h"
#include "Runtime/Actors/TestTextActor.h"
#include "Runtime/CoreUObject/UPlaneComp.h"
#include "Runtime/CoreUObject/USphereComp.h"

#include "Editor/Visualizer/IVisualizer.h"

void FEditorApplication::Initialize_ImguiWin32DX11(
    HWND &Window, ID3D11Device *Device, ID3D11DeviceContext *Context) {
  ImguiManager.Initialize_ImplWin32DX11(Window, Device, Context);
}

void FEditorApplication::Initialize_Runtime(USceneManager *SceneManager,
                                            FRenderView *RenderView) {
  this->RenderView = RenderView;
  this->SceneManager = SceneManager;
  this->CurrentScene = SceneManager->CurrentScene;

  Editor.Initialize(SceneManager);
  Editor.AddViewport(FEditorViewport{});
  Editor.LoadState();
}

void FEditorApplication::Shutdown() { Editor.Shutdown(); }

void FEditorApplication::Update(float DeltaTime) {
  BeginFrame();
  Tick(DeltaTime);
}

void FEditorApplication::BeginFrame() { ImguiManager.NewFrame(); }

void FEditorApplication::Tick(float DeltaTime) {
  ToolBar.Process(Editor, ConsoleWindow, ControlPanelWindow, PropertyWindow);
  EditorViewportWindow.Process(Editor, DeltaTime);
  WorldOutliner.Process(Editor);
  ControlPanelWindow.Process(Editor);
  PropertyWindow.Process(Editor);
  ConsoleWindow.Process(Editor);
  ContentsDrawer.Process(Editor);
  Editor.Process();
}

void FEditorApplication::Render() {
  const TArray<FEditorViewport> &EditorViewports = Editor.GetViewports();

  for (auto &EditorViewport : EditorViewports) {
    // 뷰포트 렌더링 명세 구성
    FSceneView sceneview{
        .Camera = EditorViewport.ViewportCamera,
        .ViewProj = EditorViewport.ViewportCamera.CreateViewProjectionMatrix(),
        .TopLeftUV = EditorViewport.TopLeftUV,
        .LengthUV = EditorViewport.LengthUV,
        .ViewMode = EditorViewport.ViewMode,
        .ShowFlags = EditorViewport.ShowFlags,
        .LightConstants = Editor.GlobalLight
    };

    // 에디터 렌더링 컨텍스트 구성
    FEditorRenderContext EditorCtx;
    EditorCtx.SelectedActor     = Editor.GetSelectedActor();
    EditorCtx.SelectedTransform = Editor.SelectedTransform;
    EditorCtx.Gizmo             = Editor.ObjectSelected() ? &Editor.GetGizmo() : nullptr;
    EditorCtx.TextComp          = Editor.ObjectSelected() ? Editor.GetTextcomp() : nullptr;
    EditorCtx.Grid               = &Editor.GetGrid();
    EditorCtx.VisualizerRegistry = &VisualizerRegistry;

    if (EditorCtx.SelectedActor) {
        if (USceneComponent* RootComp = EditorCtx.SelectedActor->GetRootComponent()) {
            EditorCtx.SelectedPrimitive = RootComp->Cast<UPrimitiveComponent>();
        }
    }

    // 뷰포트 렌더링 일괄 수행
    RenderView->RenderView(sceneview, *SceneManager->CurrentScene, EditorCtx);
  }
  ImguiManager.RenderUI();
}

void FEditorApplication::OnWindowSize(UINT Width, UINT Height) {
  // 뷰포트 종횡비 갱신
  for (auto &Viewport : Editor.GetViewports()) {
    const FVector2 SizePixels =
        Viewport.LengthUV *
        FVector2{static_cast<float>(Width), static_cast<float>(Height)};

    auto &Camera = Viewport.ViewportCamera;
    Camera.Projection.Aspect = SizePixels.X / SizePixels.Y;
  }
}

void FEditorApplication::CollectGarbage() {
  FGarbageCollector::Get().CollectGarbage();
}
