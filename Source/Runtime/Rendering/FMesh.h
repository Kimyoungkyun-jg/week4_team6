#pragma once

#include "Vertices.h"
#include "Runtime/Math/FVector.h"
#include "Runtime/Core/IntTypes.h"
#include "Runtime/Core/FName.h"
#include <d3d11.h>
#include <wrl/client.h>
#include "Runtime/Core/TArray.h"
#include "Runtime/Geometry/FAxisAlignedBoundingBox.h"

class FRenderer;

class FStaticMesh final
{
	friend class FRenderer;

public:
	[[nodiscard]] bool HasIndices() const { return IndexCount > 0; }
	[[nodiscard]] uint32 GetVertexCount() const { return VertexCount; }
	[[nodiscard]] uint32 GetIndexCount() const { return IndexCount; }
	[[nodiscard]] const TArray<FVector>& GetPositions() const { return Positions; }
	[[nodiscard]] const TArray<uint32>& GetIndices() const { return Indices; }
	[[nodiscard]] const FAxisAlignedBoundingBox& GetLocalBounds() const { return LocalBounds; }

	// 버퍼 데이터 갱신
	bool UpdateBuffers(ID3D11Device* Device, ID3D11DeviceContext* Context, const struct FMeshDesc& Desc);
	FName MeshId{"None"};
	FName DefaultTextureId{"None"};

	FString PathFileName;

private:

	void BindResources(ID3D11DeviceContext& Context) const;

	Microsoft::WRL::ComPtr<ID3D11Buffer> VertexBuffer;
	uint32 VertexCount = 0u;
	uint32 VertexStride = 0u;
	uint32 VertexBufferSize = 0u;

	Microsoft::WRL::ComPtr<ID3D11Buffer> IndexBuffer;
	uint32 IndexCount = 0u;
	uint32 IndexBufferSize = 0u;

	TArray<FVector> Positions;
	TArray<uint32> Indices;


	FString MtlFileName;
	TArray<FString> DiffuseTexturePaths;

	D3D11_PRIMITIVE_TOPOLOGY Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	FAxisAlignedBoundingBox LocalBounds = {};
};

struct FMeshDesc
{
	const void* VertexData = nullptr;
	uint32 VertexDataSize = 0u;
	uint32 VertexStride = sizeof(FVertexData);
	uint32 VertexCount = 0u;

	const void* IndexData = nullptr;
	uint32 IndexDataSize = 0u;
	uint32 IndexCount = 0u;

	bool bIsLine = false;
};
