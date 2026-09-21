#include "FImguiPreviewEditorWindow.h"
#include "Runtime/Engine/FRenderView.h"
#include "Runtime/Rendering/FRenderer.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Rendering/ShaderConstants.h"
#include "Runtime/Input/FInputManager.h"
#include "ThirdParty/Imgui/imgui.h"
#include "ThirdParty/Imgui/imgui_internal.h"
#include "Editor/UI/Imgui/FImguiDragDrop.h"
#include <algorithm>
#include <filesystem>

FImguiPreviewEditorWindow::FImguiPreviewEditorWindow()
{
	PreviewViewport.ViewportCamera.Projection.ProjectionType = EProjectionType::Perspective;
	PreviewViewport.ViewportCamera.Projection.FOV = 60.0f;
	PreviewViewport.ViewportCamera.Projection.Aspect = 1.0f;

	CameraController.CameraMoveSpeed = 5.0f;
	CameraController.CameraRotateSpeed = 0.5f;
}

void FImguiPreviewEditorWindow::OpenPreview(UStaticMesh* InMesh, ImGuiID InDockID, EPrevType type)
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
	prevType = type;

#if IS_OBJ_VIEWER
	TitleString += "OBJ_Viewer###PreviewEditor";
#else
	switch (type)
	{
	case EPrevType::Mesh:
		TitleString = TargetMesh->MeshId.ToString();
		TitleString += "###PreviewMeshEditor";
		TitleString += TargetMesh->MeshId.ToString();
		break;
	case EPrevType::Material:
	{
		const FString MatName = InMesh->Materials.empty() ? "Material" : InMesh->Materials[0];
		TitleString = MatName + "###PreviewMaterialEditor_" + MatName;
		break;
	}
	default:
		break;
	}
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

	constexpr float MinDistance = 2.0f;
	constexpr float MaxDistance = 100.0f;
	const float CalculatedDistance = (MeshExtent > 0.1f) ? (MeshExtent * 1.5f) : 5.0f;
	const float Distance = std::clamp(CalculatedDistance, MinDistance, MaxDistance);

	PreviewViewport.ViewportCamera.Pitch = -20.0f;
	PreviewViewport.ViewportCamera.Yaw = 45.0f;

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
	ImGui::SetNextWindowSize(ImVec2(850.0f, 600.0f), ImGuiCond_FirstUseEver);

	// 첫 프레임 생성 시 도크 노드에 강제 바인딩 (탭 중첩 보장)
	if (bNeedInitialDock && InitialDockID != 0)
	{
		ImGui::SetNextWindowDockID(InitialDockID, ImGuiCond_Always);
		ImGui::DockBuilderDockWindow(TitleString.c_str(), InitialDockID);
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

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.12f, 0.14f, 1.0f));
	const bool bWindowVisible = ImGui::Begin(TitleString.c_str(), &bIsOpen, ImGuiWindowFlags_NoCollapse);
	ImGui::PopStyleColor();
#endif

	if (bWindowVisible)
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
					FocusOnMesh();
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
		switch (prevType)
		{
		case EPrevType::Mesh:
			ImGui::BeginChild("MeshDetailsPanel", ImVec2(DetailsWidth, ViewHeight), true);
			DrawMeshDetailsPanel();
			ImGui::EndChild();
			break;
		case EPrevType::Material:
			ImGui::BeginChild("MaterialDetailsPanel", ImVec2(DetailsWidth, ViewHeight), true);
			DrawMaterialDetailsPanel();
			ImGui::EndChild();
			break;
		default:
			break;
		}
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

	if (bHovered && FInputManager::Get().IsKeyJustPressed('F'))
	{
		FocusOnMesh();
		return;
	}

	if (ImGui::IsMouseDown(ImGuiMouseButton_Right) && (bHovered || ImGui::IsWindowFocused()))
	{
		const float Wheel = ImGui::GetIO().MouseWheel;
		if (Wheel != 0.0f)
		{
			CameraSpeed += Wheel * 0.1f;
			CameraSpeed = std::clamp(CameraSpeed, 1.0f, 15.0f);
		}

		CameraController.CameraRotateSpeed = Editor.State.GetCameraSensitivity();
		CameraController.CameraMoveSpeed = CameraSpeed;

		CameraController.UpdateMouseInput(PreviewViewport.ViewportCamera);
		CameraController.UpdateKeyInput(PreviewViewport.ViewportCamera, DeltaTime);
	}
}

void FImguiPreviewEditorWindow::DrawMeshDetailsPanel()
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

	// ---------------- Materials 편집 섹션 ----------------
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "Materials");

	const auto& AllMaterialMap = FRenderResourceLibrary::Get().GetAllMaterials();
	TArray<FString> AvailableMaterials;
	AvailableMaterials.reserve(AllMaterialMap.size());
	for (const auto& [MatKey, _] : AllMaterialMap)
	{
		AvailableMaterials.push_back(MatKey);
	}
	std::sort(AvailableMaterials.begin(), AvailableMaterials.end());

	for (int SlotIdx = 0; SlotIdx < static_cast<int>(TargetMesh->Materials.size()); ++SlotIdx)
	{
		ImGui::PushID(SlotIdx);
		FString& CurrentSlotMat = TargetMesh->Materials[SlotIdx];

		ImGui::TextDisabled("Slot [%d]", SlotIdx);

		// 머티리얼 썸네일 SRV 조회
		ID3D11ShaderResourceView* ThumbnailSRV = nullptr;
		if (auto MatTex = FRenderResourceLibrary::Get().GetMaterialThumbnail(CurrentSlotMat))
		{
			ThumbnailSRV = MatTex->GetSRV();
		}

		constexpr float ThumbWidth = 72.0f;
		constexpr float ThumbHeight = 72.0f;
		const ImTextureID TexId = reinterpret_cast<ImTextureID>(ThumbnailSRV);

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 2.0f));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
		ImGui::ImageButton("##MatThumb", TexId, ImVec2(ThumbWidth, ThumbHeight), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();

		// 머티리얼 드롭 수신
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(ContentDragPayloadType))
			{
				const auto* DragData = static_cast<const FContentDragPayload*>(Payload->Data);
				if (DragData && DragData->Kind == FContentDragPayload::EKind::Material)
				{
					CurrentSlotMat = DragData->Key;
				}
			}
			ImGui::EndDragDropTarget();
		}

		// 하단 머티리얼 인디케이터 (초록색 바)
		const ImVec2 Min = ImGui::GetItemRectMin();
		const ImVec2 Max = ImGui::GetItemRectMax();
		constexpr float LineHeight = 3.5f;

		ImGui::GetWindowDrawList()->AddRectFilled(
			ImVec2(Min.x + 2.0f, Max.y - LineHeight - 2.0f),
			ImVec2(Max.x - 2.0f, Max.y - 2.0f),
			IM_COL32(46, 204, 113, 255)
		);

		ImGui::SameLine();

		const float YOffset = (ThumbHeight - ImGui::GetFrameHeight()) * 0.5f;
		if (YOffset > 0.0f)
		{
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + YOffset);
		}

		ImGui::SetNextItemWidth(-1.0f);
		if (ImGui::BeginCombo("##MatCombo", CurrentSlotMat.c_str()))
		{
			for (const FString& MatName : AvailableMaterials)
			{
				const bool bMatSelected = (CurrentSlotMat == MatName);
				if (ImGui::Selectable(MatName.c_str(), bMatSelected))
				{
					CurrentSlotMat = MatName;
				}

				if (bMatSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		ImGui::Spacing();
		ImGui::PopID();
	}

	// ---------------- 카메라 정보 ----------------
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

void FImguiPreviewEditorWindow::DrawMaterialDetailsPanel()
{
	if (!TargetMesh.IsValid() || TargetMesh->Materials.empty())
	{
		ImGui::TextDisabled("No material selected");
		return;
	}

	const FString& MatName = TargetMesh->Materials[0];
	auto Material = FRenderResourceLibrary::Get().GetMaterial(MatName);

	ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "Material Details");
	ImGui::Separator();
	ImGui::Text("Material Name: %s", MatName.c_str());

	if (!Material)
	{
		ImGui::TextDisabled("Material resource not found");
		return;
	}

	ImGui::Spacing();
	ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Texture Parameters");
	ImGui::TextDisabled("Base Color / Diffuse");

	// 1. 머티리얼에서 현재 FTexture 객체 및 SRV 가져오기
	auto DiffuseTex = Material->GetDiffuseMap();
	ID3D11ShaderResourceView* DiffuseSRV = DiffuseTex ? DiffuseTex->GetSRV() : nullptr;

	constexpr float ThumbWidth = 72.0f;
	constexpr float ThumbHeight = 72.0f;
	const ImTextureID TexId = reinterpret_cast<ImTextureID>(DiffuseSRV);

	ImGui::PushID("DiffuseSlot");

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 2.0f));
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
	ImGui::ImageButton("##DiffuseThumb", TexId, ImVec2(ThumbWidth, ThumbHeight), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
	ImGui::PopStyleColor();
	ImGui::PopStyleVar();

	// 텍스처 드래그 앤 드롭 수신
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(ContentDragPayloadType))
		{
			const auto* DragData = static_cast<const FContentDragPayload*>(Payload->Data);
			if (DragData && DragData->Kind == FContentDragPayload::EKind::Texture)
			{
				// 1. stem 추출 후 소문자 변환
				std::filesystem::path FilePath(DragData->Path);
				std::string CleanKey = FilePath.stem().string();
				std::transform(CleanKey.begin(), CleanKey.end(), CleanKey.begin(), [](unsigned char c) {
					return static_cast<char>(std::tolower(c));
					});

				// 2. 라이브러리 캐시 조회
				auto NewTex = FRenderResourceLibrary::Get().GetTexture(CleanKey);

				// 3. 미캐시 시 디스크에서 즉시 로드 후 라이브러리 등록
				if (!NewTex)
				{
					if (auto Renderer = FRenderResourceLibrary::Get().GetRenderer())
					{
						NewTex = Renderer->CreateTexture(FilePath.wstring().c_str());
						if (NewTex)
						{
							FRenderResourceLibrary::Get().RegisterTexture(CleanKey, NewTex);
						}
					}
				}

				// 4. 머티리얼에 디퓨즈 맵 반영
				if (NewTex)
				{
					Material->SetDiffuseMap(NewTex);
				}
			}
		}
		ImGui::EndDragDropTarget();
	}

	// 텍스처 인디케이터 바 (주황색)
	const ImVec2 Min = ImGui::GetItemRectMin();
	const ImVec2 Max = ImGui::GetItemRectMax();
	constexpr float LineHeight = 3.5f;

	ImGui::GetWindowDrawList()->AddRectFilled(
		ImVec2(Min.x + 2.0f, Max.y - LineHeight - 2.0f),
		ImVec2(Max.x - 2.0f, Max.y - 2.0f),
		IM_COL32(230, 126, 34, 255)
	);

	ImGui::SameLine();
	const float YOffset = (ThumbHeight - ImGui::GetFrameHeight()) * 0.5f;
	if (YOffset > 0.0f)
	{
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + YOffset);
	}

	if (DiffuseTex)
	{
		ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "Texture Bound");
	}
	else
	{
		ImGui::TextDisabled("None (Drop Texture Here)");
	}

	ImGui::PopID();

	// ---------------- 카메라 조작 안내 ----------------
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::TextColored(ImVec4(0.3f, 0.7f, 1.0f, 1.0f), "Camera (Free Flight)");
	ImGui::Text("Pos: %.1f, %.1f, %.1f",
		PreviewViewport.ViewportCamera.Position.X,
		PreviewViewport.ViewportCamera.Position.Y,
		PreviewViewport.ViewportCamera.Position.Z);

	if (ImGui::Button("Focus Mesh (F)", ImVec2(-1.0f, 25.0f)))
	{
		FocusOnMesh();
	}
}