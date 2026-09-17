#pragma once

#include "UPrimitiveComponent.h"
#include "Runtime/Engine/UScene.h"

class UConeComp : public UPrimitiveComponent
{
	DECLARE_UCLASS(UConeComp, UPrimitiveComponent)
	GENERATED_BODY()

protected:
	explicit UConeComp() = default;

public:
	void Initialize() override;
};
