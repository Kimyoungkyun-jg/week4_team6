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

#if IS_OBJ_VIEWER
  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = nullptr; // ini 파일 읽기/쓰기 비활성화

  // DisplaySize 명시적 초기화 (Assert 방지)
  RECT Rect;
  if (GetClientRect(Window, &Rect))
  {
      io.DisplaySize = ImVec2(static_cast<float>(Rect.right - Rect.left),
          static_cast<float>(Rect.bottom - Rect.top));
  }
  else
  {
      io.DisplaySize = ImVec2(1200.0f, 800.0f);
  }
#else
#endif

}

void FEditorApplication::Initialize_Runtime(USceneManager *SceneManager, FRenderView *RenderView) {

  this->RenderView = RenderView;
  this->SceneManager = SceneManager;
  this->CurrentScene = SceneManager->CurrentScene;

  Editor.Initialize(SceneManager);

#if IS_OBJ_VIEWER

#else
  FEditorViewport PerspViewport;
  PerspViewport.TopLeftUV = {0.5f, 0.0f};
  PerspViewport.LengthUV = {0.5f, 0.5f};
  PerspViewport.ViewportCamera.Projection.ProjectionType =
      EProjectionType::Perspective;
  Editor.AddViewport(PerspViewport);

  FEditorViewport TopViewport;
  TopViewport.TopLeftUV = {0.0f, 0.0f};
  TopViewport.LengthUV = {0.5f, 0.5f};
  TopViewport.ViewportCamera.Position = {0.0f, 0.0f, 20.0f};
  TopViewport.ViewportCamera.Pitch = -89.9f;
  TopViewport.ViewportCamera.Yaw = 0.0f;
  TopViewport.ViewportCamera.Projection.ProjectionType =
      EProjectionType::Orthographic;
  TopViewport.ViewportCamera.Projection.Height = 10.0f;
  Editor.AddViewport(TopViewport);

  FEditorViewport FrontViewport;
  FrontViewport.TopLeftUV = {0.0f, 0.5f};
  FrontViewport.LengthUV = {0.5f, 0.5f};
  FrontViewport.ViewportCamera.Position = {-20.0f, 0.0f, 0.0f};
  FrontViewport.ViewportCamera.Pitch = 0.0f;
  FrontViewport.ViewportCamera.Yaw = 0.0f;
  FrontViewport.ViewportCamera.Projection.ProjectionType =
      EProjectionType::Orthographic;
  FrontViewport.ViewportCamera.Projection.Height = 10.0f;
  Editor.AddViewport(FrontViewport);

  FEditorViewport SideViewport;
  SideViewport.TopLeftUV = {0.5f, 0.5f};
  SideViewport.LengthUV = {0.5f, 0.5f};
  SideViewport.ViewportCamera.Position = {0.0f, -20.0f, 0.0f};
  SideViewport.ViewportCamera.Pitch = 0.0f;
  SideViewport.ViewportCamera.Yaw = 90.0f;
  SideViewport.ViewportCamera.Projection.ProjectionType =
      EProjectionType::Orthographic;
  SideViewport.ViewportCamera.Projection.Height = 10.0f;
  Editor.AddViewport(SideViewport);

  // 원근 뷰포트를 활성화하고 상태 복원
  Editor.SetActiveViewportIndex(0);
  Editor.LoadState();

#endif
}

void FEditorApplication::Shutdown() { Editor.Shutdown(); }

void FEditorApplication::Update(float DeltaTime) {
  BeginFrame();
  Tick(DeltaTime);
}

void FEditorApplication::BeginFrame() { ImguiManager.NewFrame(); }

void FEditorApplication::Tick(float DeltaTime) {
#if IS_OBJ_VIEWER
    static bool bFirstInit = true;
    if (bFirstInit)
    {
        bFirstInit = false;
        UStaticMesh* Mesh = FRenderResourceLibrary::Get().GetUStaticMesh("Cube");
        OpenPreviewWindow(Mesh);
    }

    for (const auto& Window : PreviewWindows)
    {
        if (Window && Window->IsOpen())
        {
            // 메인 ImGui 뷰포트 영역(작업 영역) 전체 크기 가져오기
            const ImGuiViewport* MainViewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(MainViewport->WorkPos);
            ImGui::SetNextWindowSize(MainViewport->WorkSize);

            // 프리뷰 창 UI 처리 (
            Window->Process(Editor, DeltaTime);
        }
    }
#else
    ToolBar.Process(Editor, ConsoleWindow, ControlPanelWindow, PropertyWindow);
    EditorViewportWindow.Process(Editor, DeltaTime);
    WorldOutliner.Process(Editor);
    ControlPanelWindow.Process(Editor);
    PropertyWindow.Process(Editor);
    ConsoleWindow.Process(Editor);
    ContentsDrawer.Process(Editor);

    for (const auto& Window : PreviewWindows)
    {
        if (Window && Window->IsOpen())
        {
            Window->Process(Editor, DeltaTime);
        }
    }
#endif

    Editor.Process();
}

#include "ThirdParty/Imgui/imgui.h"
#include "ThirdParty/Imgui/imgui_internal.h"
#include <Runtime\CoreUObject\UMeshComponent.h>

void FEditorApplication::OpenPreviewWindow(UStaticMesh* InMesh) {
    if (!InMesh)
    {
        return;
    }

    // 이미 열려있는 창이면 최상단으로 포커스
    for (const auto& Window : PreviewWindows)
    {
        if (Window && Window->GetTargetMesh() == InMesh)
        {
            Window->BringToFront();
            return;
        }
    }

    ImGuiID TargetDockID = 0;

#if !IS_OBJ_VIEWER
    // 기존에 열려 있는 프리뷰 창의 도크 노드 탐색
    for (const auto& Window : PreviewWindows)
    {
        if (Window && Window->IsOpen())
        {
            if (ImGuiWindow* Win = ImGui::FindWindowByName(Window->GetTitleString().c_str()))
            {
                if (Win->DockId != 0)
                {
                    TargetDockID = Win->DockId;
                    break;
                }
            }
            if (TargetDockID == 0 && Window->GetInitialDockID() != 0)
            {
                TargetDockID = Window->GetInitialDockID();
                break;
            }
        }
    }

    // 첫 번째 프리뷰 창일 경우 메인 뷰포트와 분리된 독립 플로팅 도크 노드 생성
    if (TargetDockID == 0)
    {
        TargetDockID = ImGui::DockBuilderAddNode(0, 0);
        const ImGuiViewport* MainViewport = ImGui::GetMainViewport();
        const ImVec2 DefaultPos = MainViewport ? ImVec2(MainViewport->WorkPos.x + 150.0f, MainViewport->WorkPos.y + 80.0f) : ImVec2(200.0f, 100.0f);
        ImGui::DockBuilderSetNodePos(TargetDockID, DefaultPos);
        ImGui::DockBuilderSetNodeSize(TargetDockID, ImVec2(900.0f, 650.0f));
        ImGui::DockBuilderFinish(TargetDockID);
    }
#endif

    // 새 프리뷰 창 생성 및 연결 (OBJ Viewer 모드에서는 TargetDockID가 0으로 들어가 전체화면 모드로 동작)
    auto NewWindow = MakeShared<FImguiPreviewEditorWindow>();
    NewWindow->Open(InMesh, TargetDockID);
    PreviewWindows.push_back(NewWindow);
}

void FEditorApplication::Render() {

#if IS_OBJ_VIEWER
    for (const auto& Window : PreviewWindows)
    {
        if (Window && Window->IsOpen())
        {
            RenderView->RenderPreviewScene(Window->GetRenderTarget(), Window->GetPreviewViewport().ViewportCamera,
                Window->GetTargetMesh(), Window->PreviewWidth, Window->PreviewHeight, Window->bShowGrid);
        }
    }

    // 필수: ImGui 렌더링을 닫고 백버퍼에 그려야 다음 프레임 NewFrame이 동작함
    RenderView->GetRenderer().BindBackBufferWithDepth();
    ImguiManager.RenderUI();

#else
    const TArray<FEditorViewport>& EditorViewports = Editor.GetViewports();
    if (EditorViewports.empty())
        return;

    const int StartIdx = 0;
    const int EndIdx =
        Editor.bIsViewportSplit ? static_cast<int>(EditorViewports.size()) : 1;

    for (int i = StartIdx; i < EndIdx; ++i) {
        const auto& EditorViewport = EditorViewports[i];
        // 뷰포트 렌더링 명세 구성
        FSceneView sceneview{
            .Camera = EditorViewport.ViewportCamera,
            .ViewProj = EditorViewport.ViewportCamera.CreateViewProjectionMatrix(),
            .TopLeftUV = EditorViewport.TopLeftUV,
            .LengthUV = EditorViewport.LengthUV,
            .ViewMode = EditorViewport.ViewMode,
            .ShowFlags = EditorViewport.ShowFlags,
            .LightConstants = Editor.GlobalLight };

        // 에디터 렌더링 컨텍스트 구성
        FEditorRenderContext EditorCtx;
        EditorCtx.SelectedActor = Editor.GetSelectedActor();
        EditorCtx.SelectedTransform = Editor.SelectedTransform;
        EditorCtx.Gizmo = Editor.ObjectSelected() ? &Editor.GetGizmo() : nullptr;
        EditorCtx.TextComp =
            Editor.ObjectSelected() ? Editor.GetTextcomp() : nullptr;
        EditorCtx.Grid = &Editor.GetGrid();
        EditorCtx.VisualizerRegistry = &VisualizerRegistry;

        if (EditorCtx.SelectedActor) {
            if (USceneComponent* RootComp =
                EditorCtx.SelectedActor->GetRootComponent()) {
                EditorCtx.SelectedMeshComp = RootComp->Cast<UMeshComponent>();
            }
        }

        // 뷰포트 렌더링 일괄 수행
        RenderView->RenderView(sceneview, *SceneManager->CurrentScene, EditorCtx);
    }


    // 스태틱 메시 프리뷰 렌더링
    for (const auto& Window : PreviewWindows)
    {
        if (Window && Window->IsOpen())
        {
            RenderView->RenderPreviewScene(Window->GetRenderTarget(), Window->GetPreviewViewport().ViewportCamera,
                Window->GetTargetMesh(), Window->PreviewWidth, Window->PreviewHeight, Window->bShowGrid);
        }
    }

    RenderView->GetRenderer().BindBackBufferWithDepth();
    ImguiManager.RenderUI();
#endif



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
