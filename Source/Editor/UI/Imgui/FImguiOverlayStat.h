#pragma once

#include <string>

#include "Source/ThirdParty/Imgui/imgui.h"
#include "Source/Editor/Core/FEditor.h"

struct FImguiOverlayStat
{

};

struct FImguiStatFps
{
	float DeltaTime; // ms

	void SetDeltaTime(float InDeltaTime) // second
	{
		DeltaTime = InDeltaTime * 1000.f;
	}
	float GetFPS()
	{
		return 1000.f / DeltaTime;
	}
	void Process(FEditor InEditor, float InDeltaTime, FVector2 WindowSize)
	{
		const FVector2 TopLeft = InEditor.GetViewports()[0].TopLeftUV;
		const FVector2 Length = InEditor.GetViewports()[0].LengthUV;

		FVector2 PosNDC = TopLeft;
		PosNDC.X += Length.X;
		PosNDC.Y += Length.Y * 0.2f;

		FVector2 PosPixel = PosNDC * WindowSize;
		PosPixel.X -= 90.f;

		float RowMargin = 15.f;

		SetDeltaTime(InDeltaTime);
		char FpsBuf[16];
		char DeltaTimeBuf[16];
		std::snprintf(FpsBuf, sizeof(FpsBuf), "%.2f FPS", GetFPS());
		std::snprintf(DeltaTimeBuf, sizeof(DeltaTimeBuf), "%.2f ms", DeltaTime);

		ImDrawList* DrawList = ImGui::GetForegroundDrawList();
		DrawList->AddText(ImVec2(PosPixel.X, PosPixel.Y), IM_COL32(0, 255, 0, 255), FpsBuf);
		DrawList->AddText(ImVec2(PosPixel.X, PosPixel.Y + RowMargin), IM_COL32(0, 255, 0, 255), DeltaTimeBuf);
	}
};

struct FImguiStatMemory
{

	void Process(FEditor InEditor)
	{

	}
};