#include "FMaterial.h"
#include "FRenderer.h"
#include "FRenderResourceLibrary.h"
#include "Runtime/Core/Log.h"
#include <d3d11.h>
#include <algorithm>

void FMaterial::SetPipeLine(const TSharedPtr<FRenderPipeline>& InPipeline)
{
    Pipeline = InPipeline;
}


void FMaterial::SetTexture(const TSharedPtr<FTexture>& InTexture)
{
    Texture = InTexture;
}


bool FMaterial::SetTextureByName(const FName& InTextureName)
{
    auto& lib = FRenderResourceLibrary::Get();

    auto it = lib.GetTexture(InTextureName);
    if (it == nullptr)
    {
        UE_LOG("[Material] Texture '%s' not found in texture map.", InTextureName.ToString().c_str());
        return false;
    }

    SetTexture(it);
    return true;
}

void FMaterial::BindResources(ID3D11DeviceContext& Context) const
{
    // 텍스처가 없어도 반드시 바인딩한다.
    // D3D 상태는 끈끈해서, 건너뛰면 이전 드로우의 SRV가 슬롯에 남는다.
    ID3D11ShaderResourceView* SRVs[1] = { Texture ? Texture->GetSRV() : nullptr };
    Context.PSSetShaderResources(0u, 1u, SRVs);
}
