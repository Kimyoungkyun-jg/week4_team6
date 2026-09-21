#pragma once
#include "Source/Runtime/CoreUObject/UObject.h"
#include "Source/Runtime/CoreUObject/FUObjectArray.h"
#include "Source/Runtime/Engine/FTimeManager.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"

#define STATS FStatRegistry::Get()

struct FStatRegistry
{
private:
	uint32 AllocationBytes = 0;
	uint32 AllocationCount = 0;
	uint32 UObjectArrayNum = 0;
	LARGE_INTEGER Frequancy = {};
	float DeltaTime = 0.f;
	uint32 OpaqueQNum = 0;
	uint32 TranslucentQNum = 0;
	uint32 TextQNum = 0;
	uint32 InstancingQNum = 0;
	uint32 StaticMeshByte = 0;
	uint32 DrawIndexCount = 0;
	uint32 DrawVertexCount = 0;
	FVector2 WindowSize = {};

	uint32 StaticIndexBufferSize = 0;
	uint32 StaticVertexBufferSize = 0;
	uint32 PipelineNum = 0;
	uint32 DrawCallNum = 0;
	uint32 LineBatchNum = 0;
	uint32 TextureByte = 0;
	uint32 EditorTextureByte = 0;
	uint32 MeshThumbnailByte = 0;

	bool bShowFps = false;
	bool bShowMemory = false;

	FStatRegistry() = default;
	~FStatRegistry() = default;
public:
	FStatRegistry(const FStatRegistry& Other) = delete;
	FStatRegistry& operator= (const FStatRegistry& Other) = delete;

	static FStatRegistry& Get()
	{
		static FStatRegistry Instance;
		return Instance;
	}

	void Initialize()
	{
		TextureByte = 0;
		for (auto It : FRenderResourceLibrary::Get().AllTextureMap) {
			TextureByte += It.second->GetWidth() * It.second->GetHeight() * 4;
		}
		EditorTextureByte = 0;
		for (auto It : FRenderResourceLibrary::Get().AllEditorTextureMap) {
			EditorTextureByte += It.second->GetWidth() * It.second->GetHeight() * 4;
		}
		MeshThumbnailByte = 0;
		for (auto It : FRenderResourceLibrary::Get().AllMeshThumbnailMap) {
			MeshThumbnailByte += It.second->GetWidth() * It.second->GetHeight() * 4;
		}
	}

	void Reset()
	{
		DrawIndexCount = 0;
		DrawVertexCount = 0;
		DrawCallNum = 0;
		LineBatchNum = 0;
	}
	bool IsStatFps() { return bShowFps; }
	void OnStatFPS() { bShowFps = true; }
	void OffStatFPS() { bShowFps = false; }
	bool IsStatMemory() { return bShowMemory; }
	void OnStatMemory() { bShowMemory = true; }
	void OffStatMemory() { bShowMemory = false; }

	uint32 GetAllocationBytes(){
		return AllocationBytes = static_cast<uint32>(UObject::GetTotalAllocationBytes());
	}
	uint32 GetAllocationCount(){
		return AllocationCount = static_cast<uint32>(UObject::GetTotalAllocationCount());
	}
	uint32 GetUObjectArrayNum(){
		return UObjectArrayNum = FUObjectArray::Get().GetNumObjects();
	}
	LARGE_INTEGER GetFrequency(){
		return Frequancy = FTimeManager::Get().GetFrequency();
	}
	float GetDeltaTime(){
		return DeltaTime = FTimeManager::Get().GetDeltaTime();
	}
	void UpdateRenderQueueNum(size_t InOpaqueNum, size_t InTranslucentNum, size_t InTextNum, size_t InInstancingNum) {
		OpaqueQNum = static_cast<uint32>(InOpaqueNum);
		TranslucentQNum = static_cast<uint32>(InTranslucentNum);
		TextQNum = static_cast<uint32>(InTextNum);
		InstancingQNum = static_cast<uint32>(InInstancingNum);
	}
	uint32 GetOpaqueQNum() { return OpaqueQNum; }
	uint32 GetTranslucentQNum() { return TranslucentQNum; }
	uint32 GetTextQNum() { return TextQNum; }
	uint32 GetInstancingQNum() { return InstancingQNum; }
	void AddStaticMeshByte(uint32 InIndexBufferSize, uint32 InVertexBufferSize) {
		StaticIndexBufferSize += InIndexBufferSize;
		StaticVertexBufferSize += InVertexBufferSize;
		StaticMeshByte += InIndexBufferSize + InVertexBufferSize;
	}
	uint32 GetStaticMeshMemory() { return StaticMeshByte; }
	uint32 GetStaticIndexBufferSize() { return StaticIndexBufferSize; }
	uint32 GetStaticVertexBufferSize() { return StaticVertexBufferSize; }

	uint32 GetPipelineNum() {
		return PipelineNum = FRenderResourceLibrary::Get().AllPipelineMap.size(); 
	}
	void UpdateDrawCallCount(uint32 InIndexCount, uint32 InVertexCount) { 
		DrawCallNum++; 
		DrawIndexCount += InIndexCount;
		DrawVertexCount += InVertexCount;
	}
	uint32 GetDrawCallNum() { return DrawCallNum; }
	uint32 GetDrawTriangleCount() { return DrawIndexCount / 3; }
	uint32 GetDrawVertexCount() { return DrawVertexCount; }

	void UpdateWindowSize(FVector2 InWindowSize) { WindowSize = InWindowSize; }
	FVector2 GetWindowSize() { return WindowSize; }

	uint32 GetInstanceNum() { 
		return static_cast<uint32>(FRenderResourceLibrary::Get().AllInstancingArrayMap.size()); 
	}

	void AddLineBatchNum(uint32 InNum) { LineBatchNum += InNum; }
	uint32 GetLineBatchNum() { return LineBatchNum; }
	uint32 GetLineBatchByte() { return LineBatchNum * sizeof(FVertexData); }

	uint32 GetTextureByte() { return TextureByte; }
	uint32 GetEditorTextureByte() { return EditorTextureByte; }
	uint32 GetMeshThumbnailByte() { return MeshThumbnailByte; }
};