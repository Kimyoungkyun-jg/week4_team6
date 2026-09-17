#pragma once

#include "UPrimitiveComponent.h"
#include "Runtime/Engine/UScene.h"

class UCubeComp : public UPrimitiveComponent
{
	DECLARE_UCLASS(UCubeComp, UPrimitiveComponent)
	GENERATED_BODY()

protected:
	explicit UCubeComp() = default;

public:
	void Initialize() override;
};
