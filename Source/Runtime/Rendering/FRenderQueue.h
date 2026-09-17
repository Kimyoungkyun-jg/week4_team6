#pragma once

#include "FMesh.h"
#include "FMaterial.h"
#include "FRenderResourceLibrary.h"
#include "ShaderConstants.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/Core/PointerTypes.h"

// 렌더링에 필요한 드로우 정보

enum class ERenderType
{
    Primitive,
    Texture,
    Text,
    Instancing,
    Spotlight,
    None
};

struct FRenderData
{
    FName MeshId{"None"};
    FName MaterialId{"None"};
    FName TextureId{"None"};
    FObjectConstants Constants;
    ERenderType type = ERenderType::Primitive;
    bool bSelected = false;
    TArray<FInstanceData> Instances;
};

// 한 프레임의 드로우 요청을 수집하는 큐
class FRenderQueue
{
public:
    // 아이템 추가
    void Push(const FRenderData& Data)
    {
        switch (Data.type)
        {
        case ERenderType::Primitive:
            primRenderQ.push_back(Data);
            break;
        case ERenderType::Texture:
            TextureRenderQ.push_back(Data);
            break;
        case ERenderType::Text:
            TextRenderQ.push_back(Data);
            break;
        case ERenderType::Instancing:
            InstancingRenderQ.push_back(Data);
            break;
        case ERenderType::Spotlight:
            SpotlightRenderQ.push_back(Data);
            break;
        default:
            break;
        }
    }


    // 수집된 아이템 조회
    const TArray<FRenderData>& GetPrimRenderQ() const { return primRenderQ; }
    const TArray<FRenderData>& GetTextureRenderQ() const { return TextureRenderQ; }
    const TArray<FRenderData>& GetTextRenderQ() const { return TextRenderQ; }
    const TArray<FRenderData>& GetInstancingRenderQ() const { return InstancingRenderQ; }
    const TArray<FRenderData>& GetSpotlightRenderQ() const { return SpotlightRenderQ; }

    // 프레임 끝에 호출
    void Clear() { 
        primRenderQ.clear();
        TextureRenderQ.clear();
        TextRenderQ.clear();
        InstancingRenderQ.clear();
        SpotlightRenderQ.clear();
    }

    bool IsPrimRQEmpty() const { return primRenderQ.empty(); }
    bool IsTextureRQEmpty() const { return TextureRenderQ.empty(); }
    bool IsTextRQEmpty() const { return TextRenderQ.empty(); }
    bool IsInstancingRQEmpty() const { return InstancingRenderQ.empty(); }
    bool IsSpotlightRQEmpty() const { return SpotlightRenderQ.empty(); }

private:
    TArray<FRenderData> primRenderQ;
    TArray<FRenderData> TextureRenderQ;
    TArray<FRenderData> TextRenderQ;
    TArray<FRenderData> InstancingRenderQ;
    TArray<FRenderData> SpotlightRenderQ;
};
