#pragma once

#include "AActor.h"
#include "Runtime/CoreUObject/UPrimitiveComponent.h"

// 마스터이 액터 정의
class AMasterYi : public AActor
{
	DECLARE_UCLASS(AMasterYi, AActor)
	GENERATED_BODY()

public:
	explicit AMasterYi();

	UPrimitiveComponent* GetPrimitiveComponent() const;
};
