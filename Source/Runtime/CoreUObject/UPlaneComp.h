#pragma once

#include "UPrimitiveComponent.h"

class UPlaneComp : public UPrimitiveComponent
{
	DECLARE_UCLASS(UPlaneComp, UPrimitiveComponent)
	GENERATED_BODY()

protected:
	explicit UPlaneComp() = default;

public:
	void Initialize() override;
};
