#pragma once

#include "UPrimitiveComponent.h"
#include "Runtime/Engine/UScene.h"

class USphereComp : public UPrimitiveComponent
{
	DECLARE_UCLASS(USphereComp, UPrimitiveComponent)
	GENERATED_BODY()

protected:
	explicit USphereComp() = default;

public:
	void Initialize() override;
};
