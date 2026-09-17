#pragma once

#include "UMeshComponent.h"

class UStaticMeshComponent : public UMeshComponent {
    GENERATED_BODY()
    DECLARE_UCLASS(UStaticMeshComponent, UMeshComponent)

public:
    void Initialize() override;

protected:
    UStaticMeshComponent() = default;
};
