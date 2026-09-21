#pragma once

#include "FImguiStatFps.h"
#include "FImguiStatMemory.h"

struct FImguiOverlayStat
{
	FImguiStatFps StatFps;
	FImguiStatMemory StatMemory;

	void Process(FEditor InEditor, float InDeltaTime, FVector2 WindowSize)
	{
		StatFps.Process(InEditor, InDeltaTime, WindowSize);
		StatMemory.Process(InEditor);
	}
};