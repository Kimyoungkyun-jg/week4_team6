#pragma once

#include "Editor/Core/FEditor.h"
#include "Editor/EditorViewport/FEditorViewport.h"
#include "Editor/Grid/FGrid.h"
#include "Runtime/Input/FCameraInputController.h"
#include "Runtime/Rendering/FPreviewRenderTarget.h"
#include "Runtime/CoreUObject/UStaticMesh.h"
#include "ThirdParty/Imgui/imgui.h"

// 프리뷰 및 세부 속성 편집 창
class FImguiPreviewEditorWindow final
{
public:
	FImguiPreviewEditorWindow();
	~FImguiPreviewEditorWindow() = default;

	FImguiPreviewEditorWindow(const FImguiPreviewEditorWindow&) = delete;
	FImguiPreviewEditorWindow& operator=(const FImguiPreviewEditorWindow&) = delete;


	void Open(UStaticMesh* InMesh, ImGuiID InDockID = 0);
	void Close() { bIsOpen = false; }
	void BringToFront();

	[[nodiscard]] bool IsOpen() const { return bIsOpen; }
	[[nodiscard]] UStaticMesh* GetTargetMesh() const { return TargetMesh.Get(); }


	void Process(FEditor& Editor, float DeltaTime);
	void Render(class FRenderView& RenderView);


	void FocusOnMesh();
	FPreviewRenderTarget& GetRenderTarget() { return RenderTarget; }
	FEditorViewport& GetPreviewViewport() { return PreviewViewport; }
	[[nodiscard]] const FString& GetTitleString() const { return TitleString; }
	[[nodiscard]] ImGuiID GetInitialDockID() const { return InitialDockID; }

	// 뷰포트 해상도
	uint32 PreviewWidth = 512;
	uint32 PreviewHeight = 512;
	// 그리드 및 카메라 설정
	bool bShowGrid = true;
private:

	void ProcessViewportInput(FEditor& Editor, const ImVec2& ViewportPos, const ImVec2& ViewportSize, float DeltaTime);

	void DrawDetailsPanel();

	bool bIsOpen = false;
	bool bFocusRequested = false;
	bool bNeedInitialDock = false;
	ImGuiID InitialDockID = 0;
	TWeakObjectPtr<UStaticMesh> TargetMesh;


	FEditorViewport PreviewViewport;
	FCameraInputController CameraController;

	// 프리뷰 렌더타겟
	FPreviewRenderTarget RenderTarget;


	float CameraSpeed = 3.0f;

	FVector MeshCenter = { 0.0f, 0.0f, 0.0f };
	float MeshExtent = 5.0f;



	// 캐싱된 창 제목 문자열
	FString TitleString;
};
