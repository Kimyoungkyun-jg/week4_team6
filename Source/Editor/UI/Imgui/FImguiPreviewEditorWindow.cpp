#include "FImguiPreviewEditorWindow.h"
#include "Runtime/Engine/FRenderView.h"
#include "Runtime/Rendering/FRenderer.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Rendering/ShaderConstants.h"
#include "Runtime/Input/FInputManager.h"
#include "ThirdParty/Imgui/imgui.h"
#include "ThirdParty/Imgui/imgui_internal.h"
#include <algorithm>

FImguiPreviewEditorWindow::FImguiPreviewEditorWindow()
{
	PreviewViewport.ViewportCamera.Projection.ProjectionType = EProjectionType::Perspective;
	PreviewViewport.ViewportCamera.Projection.FOV = 60.0f;
	PreviewViewport.ViewportCamera.Projection.Aspect = 1.0f;


	CameraController.CameraMoveSpeed = 5.0f;
	CameraController.CameraRotateSpeed = 0.5f;
}

void FImguiPreviewEditorWindow::Open(UStaticMesh* InMesh, ImGuiID InDockID)
{
	if (!InMesh)
	{
		return;
	}

	TargetMesh = InMesh;
	bIsOpen = true;
	InitialDockID = InDockID;
	bFocusRequested = true;
	bNeedInitialDock = true;

#if IS_OBJ_VIEWER
	TitleString += "OBJ_Viewer###PreviewEditor";
#else
	// 스크린샷과 동일하게 탭에 메시 이름 표시
	TitleString = TargetMesh->MeshId.ToString();
	TitleString += "###PreviewEditor";
	TitleString += TargetMesh->MeshId.ToString();

#endif


	if (InitialDockID != 0)
	{
		ImGui::DockBuilderDockWindow(TitleString.c_str(), InitialDockID);
	}

	FocusOnMesh();

}

void FImguiPreviewEditorWindow::BringToFront()
{
	bIsOpen = true;
	bFocusRequested = true;
	FocusOnMesh();
}

void FImguiPreviewEditorWindow::FocusOnMesh()
{
	if (!TargetMesh.IsValid())
	{
		return;
	}

	const FAxisAlignedBoundingBox& Bounds = TargetMesh->GetBounds();
	MeshCenter = (Bounds.Min + Bounds.Max) * 0.5f;

	MeshExtent = (Bounds.Max - Bounds.Min).Size();

	// 최소 거리 2.0f, 최대 거리 500.0f로 클램핑 (필요에 따라 상한값 조절 가능)
	constexpr float MinDistance = 2.0f;
	constexpr float MaxDistance = 100.0f;
	const float CalculatedDistance = (MeshExtent > 0.1f) ? (MeshExtent * 1.5f) : 5.0f;
	const float Distance = std::clamp(CalculatedDistance, MinDistance, MaxDistance);

	// 메시를 비스듬히 내려다보도록 카메라 배치
	PreviewViewport.ViewportCamera.Pitch = -20.0f;
	PreviewViewport.ViewportCamera.Yaw = 45.0f;

	// 엔진 표준 회전 행렬로부터 전방 벡터 추출
	const FMatrix Rotation = FMatrix::MakeRotation(FVector(0.0f, PreviewViewport.ViewportCamera.Pitch, PreviewViewport.ViewportCamera.Yaw));
	const FVector Forward{ Rotation.M[0][0], Rotation.M[0][1], Rotation.M[0][2] };

	PreviewViewport.ViewportCamera.Position = MeshCenter - Forward * Distance;
}

void FImguiPreviewEditorWindow::Process(FEditor& Editor, float DeltaTime)
{
	if (!bIsOpen)
	{
		return;
	}

#if IS_OBJ_VIEWER
	// 뷰어 모드: 전체 화면 강제 고정 및 타이틀바/리사이즈/이동 비활성화
	const ImGuiViewport* MainViewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(MainViewport->WorkPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(MainViewport->WorkSize, ImGuiCond_Always);

	const ImGuiWindowFlags ViewerFlags = ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus;

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.12f, 0.14f, 1.0f));
	const bool bWindowVisible = ImGui::Begin(TitleString.c_str(), nullptr, ViewerFlags);
	ImGui::PopStyleColor();

	if (bWindowVisible)
#else
	// 기존 에디터 도킹 로직 유지
	ImGui::SetNextWindowSize(ImVec2(850.0f, 600.0f), ImGuiCond_FirstUseEver);

	if (bNeedInitialDock && InitialDockID != 0)
	{
		ImGui::SetNextWindowDockID(InitialDockID, ImGuiCond_Always);
		bNeedInitialDock = false;
	}
	else if (InitialDockID != 0)
	{
		ImGui::SetNextWindowDockID(InitialDockID, ImGuiCond_FirstUseEver);
	}

	if (bFocusRequested)
	{
		ImGui::SetNextWindowFocus();
		bFocusRequested = false;
	}

	// 불투명 배경색 적용
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.12f, 0.14f, 1.0f));
	const bool bWindowVisible = ImGui::Begin(TitleString.c_str(), &bIsOpen, ImGuiWindowFlags_NoCollapse);
	ImGui::PopStyleColor();

	if (bWindowVisible)
#endif
	{
		// 상단 툴바
		ImGui::Checkbox("Grid", &bShowGrid);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(120.0f);
		ImGui::SliderFloat("Speed", &CameraSpeed, 1.0f, 100.0f, "%.2f");
		ImGui::SameLine();
		if (ImGui::Button("Focus (F)"))
		{
			FocusOnMesh();
		}

		ImGui::SameLine();
		ImGui::SetNextItemWidth(200.0f);

		const std::string CurrentMeshName = TargetMesh.IsValid() ? TargetMesh->MeshId.ToString() : "Select Mesh";
		if (ImGui::BeginCombo("##MeshSelectCombo", CurrentMeshName.c_str()))
		{
			const auto& MeshMap = FRenderResourceLibrary::Get().GetAllUStaticMeshMap();
			for (const auto& [Key, MeshPtr] : MeshMap)
			{
				const std::string ItemName = Key;
				const bool bIsSelected = (TargetMesh.IsValid() && TargetMesh->MeshId == Key);

				if (ImGui::Selectable(ItemName.c_str(), bIsSelected))
				{
					TargetMesh = MeshPtr;
					FocusOnMesh(); // 메시 교체 후 카메라 초점 재정렬
				}

				if (bIsSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		ImGui::Separator();

		const float DetailsWidth = 260.0f;
		const ImVec2 Avail = ImGui::GetContentRegionAvail();

		const float ViewWidth = std::max(100.0f, Avail.x - DetailsWidth - 10.0f);
		const float ViewHeight = std::max(100.0f, Avail.y);

		const uint32 NewWidth = static_cast<uint32>(ViewWidth);
		const uint32 NewHeight = static_cast<uint32>(ViewHeight);

		if (NewWidth != PreviewWidth || NewHeight != PreviewHeight)
		{
			PreviewWidth = NewWidth;
			PreviewHeight = NewHeight;

			if (PreviewHeight > 0)
			{
				PreviewViewport.ViewportCamera.Projection.Aspect = ViewWidth / ViewHeight;
			}
		}

		// 좌측 뷰포트
		ImGui::BeginChild("PreviewViewport", ImVec2(ViewWidth, ViewHeight), false,
			ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

		const ImVec2 ViewportPos = ImGui::GetCursorScreenPos();
		if (RenderTarget.IsValid())
		{
			ImGui::Image(reinterpret_cast<ImTextureID>(RenderTarget.ShaderResourceView.Get()),
				ImVec2(ViewWidth, ViewHeight));
		}

		ProcessViewportInput(Editor, ViewportPos, ImVec2(ViewWidth, ViewHeight), DeltaTime);

		ImGui::EndChild();

		ImGui::SameLine();

		// 우측 세부 정보 패널
		ImGui::BeginChild("MeshDetailsPanel", ImVec2(DetailsWidth, ViewHeight), true);
		DrawDetailsPanel();
		ImGui::EndChild();
	}
	ImGui::End();
}

void FImguiPreviewEditorWindow::ProcessViewportInput(FEditor& Editor, const ImVec2& ViewportPos, const ImVec2& ViewportSize, float DeltaTime)
{
	const ImVec2 MousePos = ImGui::GetMousePos();
	const bool bHovered = (MousePos.x >= ViewportPos.x && MousePos.x <= ViewportPos.x + ViewportSize.x &&
		MousePos.y >= ViewportPos.y && MousePos.y <= ViewportPos.y + ViewportSize.y);

	if (!bHovered && !ImGui::IsWindowFocused())
	{
		return;
	}

	// 포커스 단축키
	if (bHovered && FInputManager::Get().IsKeyJustPressed('F'))
	{
		FocusOnMesh();
		return;
	}

	// 우클릭 자유 시점 제어
	if (ImGui::IsMouseDown(ImGuiMouseButton_Right) && (bHovered || ImGui::IsWindowFocused()))
	{
		// 우클릭 상태에서 마우스 휠로 속도 촘촘하게 동적 조절
		const float Wheel = ImGui::GetIO().MouseWheel;
		if (Wheel != 0.0f)
		{
			CameraSpeed += Wheel * 0.1f;
			// 휠 조절 속도 상한
			CameraSpeed = std::clamp(CameraSpeed, 1.0f, 15.0f);
		}

		CameraController.CameraRotateSpeed = Editor.State.GetCameraSensitivity();
		CameraController.CameraMoveSpeed = CameraSpeed;

		CameraController.UpdateMouseInput(PreviewViewport.ViewportCamera);
		CameraController.UpdateKeyInput(PreviewViewport.ViewportCamera, DeltaTime);
	}
}

void FImguiPreviewEditorWindow::DrawDetailsPanel()
{
	if (!TargetMesh.IsValid())
	{
		ImGui::TextDisabled("No mesh selected");
		return;
	}

	ImGui::TextColored(ImVec4(0.3f, 0.7f, 1.0f, 1.0f), "Static Mesh Details");
	ImGui::Separator();

	ImGui::Text("Name: %s", TargetMesh->MeshId.ToString().c_str());

	const TSharedPtr<FStaticMesh> MeshAsset = TargetMesh->GetStaticMeshAsset();
	if (MeshAsset)
	{
		ImGui::Spacing();
		ImGui::Text("Vertices: %u", MeshAsset->GetVertexCount());
		ImGui::Text("Triangles: %u", MeshAsset->GetIndexCount() / 3);

		const FAxisAlignedBoundingBox& Bounds = MeshAsset->GetLocalBounds();
		const FVector Size = Bounds.Max - Bounds.Min;
		ImGui::Spacing();
		ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Bounding Box");
		ImGui::Text("Size: %.1f, %.1f, %.1f", Size.X, Size.Y, Size.Z);
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::TextColored(ImVec4(0.3f, 0.7f, 1.0f, 1.0f), "Camera (Free Flight)");
	ImGui::Text("Pos: %.1f, %.1f, %.1f",
		PreviewViewport.ViewportCamera.Position.X,
		PreviewViewport.ViewportCamera.Position.Y,
		PreviewViewport.ViewportCamera.Position.Z);
	ImGui::Text("Yaw: %.1f, Pitch: %.1f",
		PreviewViewport.ViewportCamera.Yaw,
		PreviewViewport.ViewportCamera.Pitch);

	ImGui::Spacing();
	ImGui::TextDisabled("RMB + WASD: Fly Camera");
	ImGui::TextDisabled("Key F: Focus Mesh");

	if (ImGui::Button("Focus Mesh (F)", ImVec2(-1.0f, 25.0f)))
	{
		FocusOnMesh();
	}
}
