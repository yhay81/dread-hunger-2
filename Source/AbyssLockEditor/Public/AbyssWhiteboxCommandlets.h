#pragma once

#include "Commandlets/Commandlet.h"
#include "CoreMinimal.h"
#include "AbyssWhiteboxCommandlets.generated.h"

UCLASS()
class ABYSSLOCKEDITOR_API UCreateIcebreakerWhiteboxCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UCreateIcebreakerWhiteboxCommandlet();

    virtual int32 Main(const FString& Params) override;
};

UCLASS()
class ABYSSLOCKEDITOR_API UValidateIcebreakerWhiteboxCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UValidateIcebreakerWhiteboxCommandlet();

    virtual int32 Main(const FString& Params) override;
};
