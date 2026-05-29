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

UCLASS()
class ABYSSLOCKEDITOR_API UCreateFrostwakeVisualPocCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UCreateFrostwakeVisualPocCommandlet();

    virtual int32 Main(const FString& Params) override;
};

UCLASS()
class ABYSSLOCKEDITOR_API UValidateFrostwakeVisualPocCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UValidateFrostwakeVisualPocCommandlet();

    virtual int32 Main(const FString& Params) override;
};

UCLASS()
class ABYSSLOCKEDITOR_API UImportAmbientCgVisualPocAssetsCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UImportAmbientCgVisualPocAssetsCommandlet();

    virtual int32 Main(const FString& Params) override;
};

UCLASS()
class ABYSSLOCKEDITOR_API UValidateAmbientCgVisualPocAssetsCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UValidateAmbientCgVisualPocAssetsCommandlet();

    virtual int32 Main(const FString& Params) override;
};
