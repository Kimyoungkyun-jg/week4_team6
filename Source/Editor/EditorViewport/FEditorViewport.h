#pragma once
#include "Runtime/Math/FVector2.h"
#include "Runtime/Engine/FCamera.h"

#include "Runtime/Engine/ShowFlags.h"

class FEditorViewport final {
	bool bFocused = false;
	bool bHovered = false;
public:
	FCamera ViewportCamera;
	// 전체 클라이언트 영역 기준 고정 UV: 좌상단 (0,0), 우하단 (1,1).
	// 픽셀 위치/크기는 사용할 때 클라이언트 크기를 곱해 계산한다.
	FVector2 TopLeftUV = { 0.0f, 0.0f };
	FVector2 LengthUV = { 1.0f, 1.0f };

	// 뷰포트 렌더 모드 및 쇼 플래그
	EViewModeIndex ViewMode = EViewModeIndex::VMI_Lit;
	uint64 ShowFlags = static_cast<uint64>(EEngineShowFlags::SF_Primitives) |
	                   static_cast<uint64>(EEngineShowFlags::SF_BillboardText);

	void UpdateFocusedAndHovered(bool bFocused, bool bHovered);

	[[nodiscard]] bool HasShowFlag(EEngineShowFlags Flag) const {
		return (ShowFlags & static_cast<uint64>(Flag)) != 0;
	}
	
	void ToggleShowFlag(EEngineShowFlags Flag) {
		ShowFlags ^= static_cast<uint64>(Flag);
	}


	[[nodiscard]] bool IsFocused() const { return bFocused; }
	[[nodiscard]] bool IsHovered() const { return bHovered; }
};