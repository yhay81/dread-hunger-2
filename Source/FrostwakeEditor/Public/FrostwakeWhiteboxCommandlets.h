#pragma once

#include "Commandlets/Commandlet.h"
#include "CoreMinimal.h"
#include "FrostwakeWhiteboxCommandlets.generated.h"

UCLASS()
class FROSTWAKEEDITOR_API UCreateIcebreakerWhiteboxCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UCreateIcebreakerWhiteboxCommandlet();

    virtual int32 Main(const FString& Params) override;
};

UCLASS()
class FROSTWAKEEDITOR_API UValidateIcebreakerWhiteboxCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UValidateIcebreakerWhiteboxCommandlet();

    virtual int32 Main(const FString& Params) override;
};

UCLASS()
class FROSTWAKEEDITOR_API UCreateFrostwakeVisualPocCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UCreateFrostwakeVisualPocCommandlet();

    virtual int32 Main(const FString& Params) override;
};

UCLASS()
class FROSTWAKEEDITOR_API UValidateFrostwakeVisualPocCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UValidateFrostwakeVisualPocCommandlet();

    virtual int32 Main(const FString& Params) override;
};

UCLASS()
class FROSTWAKEEDITOR_API UImportAmbientCgVisualPocAssetsCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UImportAmbientCgVisualPocAssetsCommandlet();

    virtual int32 Main(const FString& Params) override;
};

UCLASS()
class FROSTWAKEEDITOR_API UValidateAmbientCgVisualPocAssetsCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UValidateAmbientCgVisualPocAssetsCommandlet();

    virtual int32 Main(const FString& Params) override;
};

UCLASS()
class FROSTWAKEEDITOR_API UCreateMainMenuCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UCreateMainMenuCommandlet();

    virtual int32 Main(const FString& Params) override;
};

UCLASS()
class FROSTWAKEEDITOR_API UImportPolyhavenCc0AssetsCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UImportPolyhavenCc0AssetsCommandlet();

    virtual int32 Main(const FString& Params) override;
};

UCLASS()
class FROSTWAKEEDITOR_API UImportNotoCjkFontAssetsCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UImportNotoCjkFontAssetsCommandlet();

    virtual int32 Main(const FString& Params) override;
};
