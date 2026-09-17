#pragma once

#include "UPrimitiveComponent.h"
#include "Runtime/Engine/UScene.h"

class UCylinderComp : public UPrimitiveComponent
{
	DECLARE_UCLASS(UCylinderComp, UPrimitiveComponent)
	GENERATED_BODY()

protected:
	explicit UCylinderComp() = default;

public:
	void Initialize() override;
};
