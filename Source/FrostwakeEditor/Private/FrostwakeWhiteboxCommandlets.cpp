#include "FrostwakeWhiteboxCommandlets.h"

#include "FrostwakeDoorActor.h"
#include "FrostwakeHeatSourceActor.h"
#include "FrostwakeItemPickupActor.h"
#include "FrostwakeMenuGameMode.h"
#include "FrostwakeShipTaskActor.h"
#include "AssetToolsModule.h"
#include "AutomatedAssetImportData.h"
#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/PointLight.h"
#include "Engine/TextureCube.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/SkyLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/Texture.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "MaterialEditingLibrary.h"
#include "Factories/MaterialFactoryNew.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "Framework/Application/SlateApplication.h"
#include "Materials/Material.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "LevelEditorSubsystem.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "StandaloneRenderer.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogFrostwakeWhiteboxCommandlet, Log, All);

namespace FrostwakeWhitebox
{
const FString MapPackage = TEXT("/Game/Maps/L_IcebreakerWhitebox");
const FString MapAsset = TEXT("/Game/Maps/L_IcebreakerWhitebox.L_IcebreakerWhitebox");
const FName WhiteboxTag = TEXT("FrostwakeWhitebox");

struct FCubeSpec
{
    const TCHAR* Label;
    FVector Location;
    FVector Size;
};

struct FPlayerStartSpec
{
    int32 Index;
    FVector Location;
    float Yaw;
};

struct FTargetSpec
{
    const TCHAR* Label;
    FVector Location;
};

struct FTaskSpec
{
    const TCHAR* Label;
    const TCHAR* System;
    bool bSabotage;
    FVector Location;
    float Delta;
    bool bSingleUse;
    bool bSaboteurOnly;
    bool bSetOffline;
};

struct FDoorSpec
{
    const TCHAR* Label;
    const TCHAR* DoorId;
    FVector Location;
    float Yaw;
};

struct FTaskExpectation
{
    const TCHAR* Label;
    const TCHAR* System;
    const TCHAR* Mode;
};

bool Fail(FString& Error, const FString& Message)
{
    Error = Message;
    UE_LOG(LogFrostwakeWhiteboxCommandlet, Error, TEXT("%s"), *Message);
    return false;
}

UWorld* CurrentEditorWorld()
{
    return GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
}

bool IsWhiteboxActor(const AActor* Actor)
{
    if (!Actor)
    {
        return false;
    }

    const FString Label = Actor->GetActorLabel();
    if (Label.StartsWith(TEXT("WB_")) || Label.StartsWith(TEXT("PS_Crew_")) || Label.StartsWith(TEXT("TP_")) ||
        Label.StartsWith(TEXT("TASK_")) || Label.StartsWith(TEXT("DOOR_")) || Label.StartsWith(TEXT("L_Key_")) ||
        Label.StartsWith(TEXT("L_Sky_")))
    {
        return true;
    }

    return Actor->Tags.Contains(WhiteboxTag);
}

bool LoadOrCreateMap(UWorld*& OutWorld, FString& Error)
{
    if (FPackageName::DoesPackageExist(MapPackage))
    {
        OutWorld = UEditorLoadingAndSavingUtils::LoadMap(MapPackage);
    }
    else
    {
        OutWorld = UEditorLoadingAndSavingUtils::NewBlankMap(false);
    }

    if (!OutWorld)
    {
        return Fail(Error, FString::Printf(TEXT("Could not load or create %s."), *MapPackage));
    }
    return true;
}

bool LoadMapForValidation(UWorld*& OutWorld, FString& Error)
{
    if (!FPackageName::DoesPackageExist(MapPackage))
    {
        return Fail(Error, FString::Printf(TEXT("Missing map asset: %s"), *MapPackage));
    }

    OutWorld = UEditorLoadingAndSavingUtils::LoadMap(MapPackage);
    if (!OutWorld)
    {
        return Fail(Error, FString::Printf(TEXT("Could not load map: %s"), *MapPackage));
    }
    return true;
}

bool ClearExistingWhitebox(UWorld* World, FString& Error)
{
    if (!World)
    {
        return Fail(Error, TEXT("No editor world is loaded."));
    }

    TArray<AActor*> ActorsToDestroy;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (IsWhiteboxActor(*It))
        {
            ActorsToDestroy.Add(*It);
        }
    }

    for (AActor* Actor : ActorsToDestroy)
    {
        if (Actor && !World->EditorDestroyActor(Actor, true))
        {
            return Fail(Error, FString::Printf(TEXT("Could not remove whitebox actor %s."), *GetNameSafe(Actor)));
        }
    }
    return true;
}

AActor* SpawnActor(UClass* ActorClass, const TCHAR* Label, const FVector& Location, const FRotator& Rotation, FString& Error)
{
    UEditorActorSubsystem* ActorSubsystem = GEditor ? GEditor->GetEditorSubsystem<UEditorActorSubsystem>() : nullptr;
    AActor* Actor = ActorSubsystem ? ActorSubsystem->SpawnActorFromClass(TSubclassOf<AActor>(ActorClass), Location, Rotation, false) : nullptr;
    if (!Actor)
    {
        Fail(Error, FString::Printf(TEXT("Could not spawn actor %s."), Label));
        return nullptr;
    }

    Actor->SetActorLabel(Label);
    Actor->Tags.AddUnique(WhiteboxTag);
    return Actor;
}

// Build a simple PBR material from an already-imported CC0 ambientCG texture set (BaseColor +
// tiling + constant roughness/metallic) and save it as a reusable asset. No-op if it already exists;
// logs + returns gracefully if the source texture is missing (surface then keeps the engine default).
void GetOrCreateAmbientCgMaterial(const TCHAR* MaterialName, const TCHAR* AssetId, float Tiling, float Roughness, float Metallic)
{
    const FString PackagePath = FString::Printf(TEXT("/Game/ThirdParty/Quarantine/ambientCG/Materials/%s"), MaterialName);
    if (LoadObject<UMaterialInterface>(nullptr, *PackagePath))
    {
        return; // created in a prior run
    }

    const FString ColorTexPath = FString::Printf(TEXT("/Game/ThirdParty/Quarantine/ambientCG/%s/%s_1K-JPG_Color"), AssetId, AssetId);
    UTexture* ColorTex = LoadObject<UTexture>(nullptr, *ColorTexPath);
    if (!ColorTex)
    {
        UE_LOG(LogFrostwakeWhiteboxCommandlet, Warning, TEXT("CC0 material %s: color texture missing (%s); keeping engine default."), MaterialName, *ColorTexPath);
        return;
    }

    UPackage* Package = CreatePackage(*PackagePath);
    UMaterialFactoryNew* Factory = NewObject<UMaterialFactoryNew>();
    UMaterial* Material = Cast<UMaterial>(Factory->FactoryCreateNew(UMaterial::StaticClass(), Package, FName(MaterialName), RF_Public | RF_Standalone, nullptr, GWarn));
    if (!Material)
    {
        UE_LOG(LogFrostwakeWhiteboxCommandlet, Warning, TEXT("CC0 material %s: creation failed."), MaterialName);
        return;
    }

    UMaterialExpressionTextureCoordinate* TexCoord = Cast<UMaterialExpressionTextureCoordinate>(
        UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionTextureCoordinate::StaticClass(), -500, 0));
    if (TexCoord)
    {
        TexCoord->UTiling = Tiling;
        TexCoord->VTiling = Tiling;
    }

    UMaterialExpressionTextureSample* ColorSample = Cast<UMaterialExpressionTextureSample>(
        UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionTextureSample::StaticClass(), -250, 0));
    if (ColorSample)
    {
        ColorSample->Texture = ColorTex;
        ColorSample->SamplerType = SAMPLERTYPE_Color;
        if (TexCoord)
        {
            UMaterialEditingLibrary::ConnectMaterialExpressions(TexCoord, FString(), ColorSample, TEXT("UVs"));
        }
        UMaterialEditingLibrary::ConnectMaterialProperty(ColorSample, FString(), MP_BaseColor);
    }

    UMaterialExpressionConstant* RoughExpr = Cast<UMaterialExpressionConstant>(
        UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionConstant::StaticClass(), -250, 250));
    if (RoughExpr)
    {
        RoughExpr->R = Roughness;
        UMaterialEditingLibrary::ConnectMaterialProperty(RoughExpr, FString(), MP_Roughness);
    }

    if (Metallic > 0.0f)
    {
        UMaterialExpressionConstant* MetalExpr = Cast<UMaterialExpressionConstant>(
            UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionConstant::StaticClass(), -250, 400));
        if (MetalExpr)
        {
            MetalExpr->R = Metallic;
            UMaterialEditingLibrary::ConnectMaterialProperty(MetalExpr, FString(), MP_Metallic);
        }
    }

    UMaterialEditingLibrary::RecompileMaterial(Material);

    Package->MarkPackageDirty();
    const FString Filename = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(Package, Material, *Filename, SaveArgs);
    UE_LOG(LogFrostwakeWhiteboxCommandlet, Display, TEXT("Created CC0 material %s from %s (tiling %.0f)."), MaterialName, AssetId, Tiling);
}

bool SpawnCube(const FCubeSpec& Spec, UStaticMesh* CubeMesh, FString& Error)
{
    AStaticMeshActor* Actor = Cast<AStaticMeshActor>(SpawnActor(AStaticMeshActor::StaticClass(), Spec.Label, Spec.Location, FRotator::ZeroRotator, Error));
    if (!Actor)
    {
        return false;
    }

    Actor->SetActorScale3D(FVector(Spec.Size.X / 100.0, Spec.Size.Y / 100.0, Spec.Size.Z / 100.0));
    UStaticMeshComponent* MeshComponent = Actor->GetStaticMeshComponent();
    if (!MeshComponent)
    {
        return Fail(Error, FString::Printf(TEXT("StaticMeshComponent missing on %s."), Spec.Label));
    }
    MeshComponent->SetStaticMesh(CubeMesh);
    return true;
}

bool SpawnPlayerStart(const FPlayerStartSpec& Spec, FString& Error)
{
    const FString Label = FString::Printf(TEXT("PS_Crew_%02d"), Spec.Index);
    APlayerStart* Start = Cast<APlayerStart>(SpawnActor(APlayerStart::StaticClass(), *Label, Spec.Location, FRotator(0.0, Spec.Yaw, 0.0), Error));
    if (!Start)
    {
        return false;
    }

    Start->PlayerStartTag = FName(*FString::Printf(TEXT("Crew%02d"), Spec.Index));
    return true;
}

bool SpawnTarget(const FTargetSpec& Spec, FString& Error)
{
    return SpawnActor(ATargetPoint::StaticClass(), Spec.Label, Spec.Location, FRotator::ZeroRotator, Error) != nullptr;
}

bool SpawnShipTask(const FTaskSpec& Spec, FString& Error)
{
    AFrostwakeShipTaskActor* Task = Cast<AFrostwakeShipTaskActor>(SpawnActor(AFrostwakeShipTaskActor::StaticClass(), Spec.Label, Spec.Location, FRotator::ZeroRotator, Error));
    if (!Task)
    {
        return false;
    }
    if (!Task->ConfigureTaskByName(FName(Spec.System), Spec.bSabotage, Spec.Delta, Spec.bSingleUse, Spec.bSaboteurOnly, Spec.bSetOffline))
    {
        return Fail(Error, FString::Printf(TEXT("Could not configure ship task %s for system %s."), Spec.Label, Spec.System));
    }
    return true;
}

bool SpawnBulkheadDoor(const FDoorSpec& Spec, FString& Error)
{
    AFrostwakeDoorActor* Door = Cast<AFrostwakeDoorActor>(SpawnActor(AFrostwakeDoorActor::StaticClass(), Spec.Label, Spec.Location, FRotator(0.0, Spec.Yaw, 0.0), Error));
    if (!Door)
    {
        return false;
    }
    Door->ConfigureDoorByName(FName(Spec.DoorId));
    return true;
}

struct FPropSpec
{
    const TCHAR* Slug;
    const TCHAR* Label;
    FVector Location;
    float Yaw;
    float Scale;
};

UStaticMesh* LoadPolyhavenMesh(const FString& Slug)
{
    const FString Base = FString::Printf(TEXT("/Game/ThirdParty/Quarantine/polyhaven/models/%s/%s"), *Slug, *Slug);
    UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Base);
    if (!Mesh)
    {
        const FString Alt = Base + TEXT("_1k");
        Mesh = LoadObject<UStaticMesh>(nullptr, *Alt);
    }
    return Mesh;
}

bool SpawnProp(const FPropSpec& Spec, FString& Error)
{
    UStaticMesh* Mesh = LoadPolyhavenMesh(Spec.Slug);
    if (!Mesh)
    {
        // Non-fatal: skip a missing prop so regeneration stays robust if CC0 assets are not imported.
        UE_LOG(LogFrostwakeWhiteboxCommandlet, Warning, TEXT("Prop mesh not found for %s; skipping."), Spec.Slug);
        return true;
    }

    AStaticMeshActor* Actor = Cast<AStaticMeshActor>(SpawnActor(AStaticMeshActor::StaticClass(), Spec.Label, Spec.Location, FRotator(0.0, Spec.Yaw, 0.0), Error));
    if (!Actor)
    {
        return false;
    }

    Actor->SetActorScale3D(FVector(Spec.Scale));
    if (UStaticMeshComponent* MeshComponent = Actor->GetStaticMeshComponent())
    {
        MeshComponent->SetStaticMesh(Mesh);
        MeshComponent->SetMobility(EComponentMobility::Movable);
    }
    return true;
}

bool SpawnItemPickup(const TCHAR* ItemId, const TCHAR* Label, const FVector& Location, FString& Error)
{
    AFrostwakeItemPickupActor* Pickup = Cast<AFrostwakeItemPickupActor>(SpawnActor(AFrostwakeItemPickupActor::StaticClass(), Label, Location, FRotator::ZeroRotator, Error));
    if (!Pickup)
    {
        return false;
    }
    Pickup->ConfigureItem(FName(ItemId));
    return true;
}

bool SpawnHeatSource(const TCHAR* Label, const FVector& Location, FString& Error)
{
    return SpawnActor(AFrostwakeHeatSourceActor::StaticClass(), Label, Location, FRotator::ZeroRotator, Error) != nullptr;
}

bool CreateWhitebox(FString& Error)
{
    UWorld* World = nullptr;
    if (!LoadOrCreateMap(World, Error) || !ClearExistingWhitebox(World, Error))
    {
        return false;
    }

    UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!CubeMesh)
    {
        return Fail(Error, TEXT("Could not load /Engine/BasicShapes/Cube.Cube."));
    }

    // Single-deck ship interior: a central spine corridor flanked by walled rooms, with a bow ice
    // deck open to the sky. Rooms are HOLLOW (thin wall segments with doorway gaps) instead of the
    // old solid blocks (the "white box" problem). Required labels (validator) ride on each room's
    // representative wall. Walls: 40cm thick, 340cm tall, centered Z=150 (run Z -20..320).
    const TArray<FCubeSpec> Cubes = {
        // Deck plate + outer hull shell + interior ceiling (ice deck left open to the HDRI sky).
        {TEXT("WB_Hull_MainDeck"), FVector(0, 0, -20), FVector(4300, 1450, 40)},
        {TEXT("WB_Hull_PortWall"), FVector(-50, -720, 150), FVector(4300, 40, 340)},
        {TEXT("WB_Hull_StarboardWall"), FVector(-50, 720, 150), FVector(4300, 40, 340)},
        {TEXT("WB_Hull_SternWall"), FVector(-2150, 0, 150), FVector(40, 1480, 340)},
        {TEXT("WB_Hull_BowRail"), FVector(2150, 0, 60), FVector(40, 1480, 160)},
        {TEXT("WB_Ceiling_Interior"), FVector(-450, 0, 320), FVector(3200, 1450, 30)},
        // Central spine corridor (segmented so the spawn cluster + doorways stay open).
        {TEXT("WB_Corridor_PortWall_Fwd"), FVector(700, -130, 150), FVector(1200, 40, 340)},
        {TEXT("WB_Corridor_PortWall_Aft"), FVector(-1000, -130, 150), FVector(1400, 40, 340)},
        {TEXT("WB_Corridor_StbdWall_Fwd"), FVector(700, 130, 150), FVector(1200, 40, 340)},
        {TEXT("WB_Corridor_StbdWall_Aft"), FVector(-1000, 130, 150), FVector(1400, 40, 340)},
        // Bridge (port-fwd) + Radio (stbd-fwd) — required labels on the aft walls.
        {TEXT("WB_Bridge"), FVector(700, -380, 150), FVector(40, 460, 340)},
        {TEXT("WB_Bridge_FwdWall"), FVector(1300, -380, 150), FVector(40, 460, 340)},
        {TEXT("WB_Bridge_Console"), FVector(1150, -560, 110), FVector(360, 200, 200)},
        {TEXT("WB_RadioRoom"), FVector(700, 380, 150), FVector(40, 460, 340)},
        {TEXT("WB_Radio_FwdWall"), FVector(1300, 380, 150), FVector(40, 460, 340)},
        {TEXT("WB_Radio_Rack"), FVector(1150, 560, 130), FVector(300, 200, 240)},
        // Crew quarters (port-mid, =QuartermasterStore) + Infirmary (stbd-mid) — required labels on fwd walls.
        {TEXT("WB_QuartermasterStore"), FVector(100, -380, 150), FVector(40, 460, 340)},
        {TEXT("WB_Quarters_Bunks"), FVector(-150, -560, 100), FVector(520, 200, 180)},
        {TEXT("WB_Infirmary"), FVector(100, 380, 150), FVector(40, 460, 340)},
        {TEXT("WB_Infirmary_Cots"), FVector(-150, 560, 100), FVector(520, 200, 160)},
        // Engine (port-aft) + Battery/Power (stbd-aft) — required labels on fwd walls.
        {TEXT("WB_EngineRoom"), FVector(-700, -380, 150), FVector(40, 460, 340)},
        {TEXT("WB_Engine_Block"), FVector(-1050, -520, 140), FVector(520, 280, 260)},
        {TEXT("WB_BatteryRoom"), FVector(-700, 380, 150), FVector(40, 460, 340)},
        {TEXT("WB_Battery_RackA"), FVector(-1050, 480, 150), FVector(520, 160, 260)},
        // Fuel bay (aft, full width with a centerline passage) + bow exterior ice deck.
        {TEXT("WB_FuelBay"), FVector(-1500, -360, 150), FVector(40, 700, 340)},
        {TEXT("WB_Fuel_FwdWall_Stbd"), FVector(-1500, 360, 150), FVector(40, 700, 340)},
        {TEXT("WB_Fuel_DrumCluster"), FVector(-1880, 0, 100), FVector(360, 700, 180)},
        // Frozen sea: a vast ground plane the ship sits on, extending past the bow into the fog.
        // This is the FIELD canvas — a Megascans snow/ice surface material + scattered ice rocks
        // dress it next (the plane is the permanent ground; only its material changes).
        {TEXT("WB_Field_FrozenSea"), FVector(2400, 0, -40), FVector(20000, 16000, 40)},
        {TEXT("WB_Ice_DeckPlate"), FVector(1800, 0, -10), FVector(700, 1450, 60)},
        {TEXT("WB_Ice_RailPort"), FVector(1800, -640, 110), FVector(700, 40, 180)},
        {TEXT("WB_Ice_RailStbd"), FVector(1800, 640, 110), FVector(700, 40, 180)},
        {TEXT("WB_Ice_Winch"), FVector(1650, 420, 100), FVector(300, 220, 160)},
        {TEXT("WB_IcePressureGate"), FVector(1980, 0, 110), FVector(90, 1100, 180)},
    };
    for (const FCubeSpec& Cube : Cubes)
    {
        if (!SpawnCube(Cube, CubeMesh, Error))
        {
            return false;
        }
    }

    // --- CC0 surface materials (ambientCG, already imported; no download needed) ---
    // Build the reusable materials (once), then assign them to the right surfaces by label so the
    // greybox reads as a real snow field + metal ship. Tiling is baked per-purpose (world size).
    GetOrCreateAmbientCgMaterial(TEXT("M_Field_Snow"), TEXT("Snow015"), 140.0f, 0.90f, 0.0f);
    GetOrCreateAmbientCgMaterial(TEXT("M_Deck_Snow"), TEXT("Snow015"), 12.0f, 0.90f, 0.0f);
    GetOrCreateAmbientCgMaterial(TEXT("M_Deck_Metal"), TEXT("DiamondPlate009"), 24.0f, 0.45f, 0.85f);
    GetOrCreateAmbientCgMaterial(TEXT("M_Wall_Metal"), TEXT("PaintedMetal004"), 10.0f, 0.50f, 0.60f);
    GetOrCreateAmbientCgMaterial(TEXT("M_Ice"), TEXT("Ice003"), 5.0f, 0.20f, 0.0f);

    TMap<FString, AStaticMeshActor*> CubeByLabel;
    for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
    {
        CubeByLabel.Add(It->GetActorLabel(), *It);
    }
    auto ApplyMaterial = [&CubeByLabel](const TCHAR* Label, const TCHAR* MaterialName)
    {
        AStaticMeshActor** Found = CubeByLabel.Find(Label);
        if (!Found || !*Found)
        {
            return;
        }
        UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("/Game/ThirdParty/Quarantine/ambientCG/Materials/%s"), MaterialName));
        if (!Mat)
        {
            return; // material absent (e.g. texture missing) → keep engine default
        }
        if (UStaticMeshComponent* MeshComponent = (*Found)->GetStaticMeshComponent())
        {
            MeshComponent->SetMaterial(0, Mat);
        }
    };

    ApplyMaterial(TEXT("WB_Field_FrozenSea"), TEXT("M_Field_Snow"));
    ApplyMaterial(TEXT("WB_Ice_DeckPlate"), TEXT("M_Deck_Snow"));
    ApplyMaterial(TEXT("WB_Hull_MainDeck"), TEXT("M_Deck_Metal"));
    ApplyMaterial(TEXT("WB_Ceiling_Interior"), TEXT("M_Wall_Metal"));
    ApplyMaterial(TEXT("WB_IcePressureGate"), TEXT("M_Ice"));

    const TCHAR* WallLabels[] = {
        TEXT("WB_Hull_PortWall"), TEXT("WB_Hull_StarboardWall"), TEXT("WB_Hull_SternWall"), TEXT("WB_Hull_BowRail"),
        TEXT("WB_Corridor_PortWall_Fwd"), TEXT("WB_Corridor_PortWall_Aft"), TEXT("WB_Corridor_StbdWall_Fwd"), TEXT("WB_Corridor_StbdWall_Aft"),
        TEXT("WB_Bridge"), TEXT("WB_Bridge_FwdWall"), TEXT("WB_RadioRoom"), TEXT("WB_Radio_FwdWall"),
        TEXT("WB_QuartermasterStore"), TEXT("WB_Infirmary"), TEXT("WB_EngineRoom"), TEXT("WB_BatteryRoom"),
        TEXT("WB_FuelBay"), TEXT("WB_Fuel_FwdWall_Stbd"),
    };
    for (const TCHAR* WallLabel : WallLabels)
    {
        ApplyMaterial(WallLabel, TEXT("M_Wall_Metal"));
    }

    const TCHAR* MetalDetailLabels[] = {
        TEXT("WB_Bridge_Console"), TEXT("WB_Radio_Rack"), TEXT("WB_Engine_Block"),
        TEXT("WB_Battery_RackA"), TEXT("WB_Fuel_DrumCluster"), TEXT("WB_Ice_Winch"),
    };
    for (const TCHAR* DetailLabel : MetalDetailLabels)
    {
        ApplyMaterial(DetailLabel, TEXT("M_Deck_Metal"));
    }

    // Spawn cluster in the open central gap (between the fwd/aft corridor wall segments), on the deck.
    const TArray<FPlayerStartSpec> PlayerStarts = {
        {1, FVector(-280, -200, 110), 20},
        {2, FVector(-280, 0, 110), 0},
        {3, FVector(-280, 200, 110), -20},
        {4, FVector(-150, -200, 110), 20},
        {5, FVector(-150, 0, 110), 0},
        {6, FVector(-150, 200, 110), -20},
        {7, FVector(0, -100, 110), 10},
        {8, FVector(0, 100, 110), -10},
    };
    for (const FPlayerStartSpec& PlayerStart : PlayerStarts)
    {
        if (!SpawnPlayerStart(PlayerStart, Error))
        {
            return false;
        }
    }

    const TArray<FTargetSpec> Targets = {
        {TEXT("TP_RouteControl"), FVector(1150, -430, 200)},
        {TEXT("TP_RadioRepair"), FVector(1150, 430, 200)},
        {TEXT("TP_MedicalRescue"), FVector(350, 430, 200)},
        {TEXT("TP_FuelContamination"), FVector(-1850, -300, 200)},
        {TEXT("TP_PowerRepair"), FVector(-1050, 360, 200)},
        {TEXT("TP_EngineRepair"), FVector(-1050, -360, 200)},
        {TEXT("TP_IcePressureObjective"), FVector(1980, 0, 200)},
        {TEXT("TP_HullFlooding"), FVector(350, -300, 200)},
    };
    for (const FTargetSpec& Target : Targets)
    {
        if (!SpawnTarget(Target, Error))
        {
            return false;
        }
    }

    // Positions updated to the single-deck rooms (Z~70 on the deck). Labels/systems/modes/flags
    // are LOCKED by ValidateTaskConfig — do not change those, only the locations.
    const TArray<FTaskSpec> Tasks = {
        {TEXT("TASK_Repair_Radio"), TEXT("Radio"), false, FVector(1150, 430, 70), 0.40f, false, false, false},
        {TEXT("TASK_Sabotage_Radio"), TEXT("Radio"), true, FVector(1050, 620, 70), 0.35f, false, true, true},
        {TEXT("TASK_Repair_Power"), TEXT("Power"), false, FVector(-1050, 360, 70), 0.40f, false, false, false},
        {TEXT("TASK_Sabotage_Power"), TEXT("Power"), true, FVector(-1050, 600, 70), 0.35f, false, true, true},
        {TEXT("TASK_Repair_Engine"), TEXT("Engine"), false, FVector(-1050, -360, 70), 0.30f, false, false, false},
        {TEXT("TASK_Sabotage_Fuel"), TEXT("Fuel"), true, FVector(-1850, -300, 70), 0.35f, false, true, false},
        {TEXT("TASK_Repair_HullFlooding"), TEXT("Flooding"), false, FVector(350, 300, 70), 0.35f, false, false, false},
        {TEXT("TASK_Sabotage_HullFlooding"), TEXT("Flooding"), true, FVector(350, -300, 70), 0.35f, false, true, false},
        {TEXT("TASK_Repair_Fuel"), TEXT("Fuel"), false, FVector(-1850, 0, 70), 0.5f, false, false, false},
    };
    for (const FTaskSpec& Task : Tasks)
    {
        if (!SpawnShipTask(Task, Error))
        {
            return false;
        }
    }

    // Doors sit in the corridor/room wall gaps on the single deck (Z=10; the panel auto-fills above).
    const TArray<FDoorSpec> Doors = {
        {TEXT("DOOR_RadioBulkhead"), TEXT("RadioBulkhead"), FVector(1000, 150, 10), 0},
        {TEXT("DOOR_EngineBulkhead"), TEXT("EngineBulkhead"), FVector(-700, 0, 10), 90},
        {TEXT("DOOR_HoldFloodBulkhead"), TEXT("HoldFloodBulkhead"), FVector(-1500, 0, 10), 90},
    };
    for (const FDoorSpec& Door : Doors)
    {
        if (!SpawnBulkheadDoor(Door, Error))
        {
            return false;
        }
    }

    AActor* KeyLight = SpawnActor(ADirectionalLight::StaticClass(), TEXT("L_Key_ArcticSun"), FVector(-900, -1200, 2200), FRotator(-38, -35, 0), Error);
    if (!KeyLight)
    {
        return false;
    }
    if (USceneComponent* KeyRoot = KeyLight->GetRootComponent())
    {
        // Movable so the greybox is fully dynamically lit at runtime: no lighting bake, no
        // "LIGHTING NEEDS TO BE REBUILT" warning, always visible for debugging.
        KeyRoot->SetMobility(EComponentMobility::Movable);
    }

    AActor* SkyLight = SpawnActor(ASkyLight::StaticClass(), TEXT("L_Sky_Overcast"), FVector(0, 0, 1200), FRotator::ZeroRotator, Error);
    if (!SkyLight)
    {
        return false;
    }
    if (USceneComponent* SkyRoot = SkyLight->GetRootComponent())
    {
        SkyRoot->SetMobility(EComponentMobility::Movable);
    }
    if (USkyLightComponent* SkyComp = SkyLight->FindComponentByClass<USkyLightComponent>())
    {
        // Light the scene with the imported CC0 overcast HDRI when available (graceful fallback to
        // the dynamic captured-scene skylight if the HDRI did not import as a cubemap).
        if (UTextureCube* HdriCube = LoadObject<UTextureCube>(nullptr, TEXT("/Game/ThirdParty/Quarantine/polyhaven/hdri/blaubeuren_outskirts_1k")))
        {
            SkyComp->SourceType = ESkyLightSourceType::SLS_SpecifiedCubemap;
            SkyComp->Cubemap = HdriCube;
            SkyComp->RecaptureSky();
            UE_LOG(LogFrostwakeWhiteboxCommandlet, Display, TEXT("SkyLight using imported HDRI cubemap."));
        }
        else
        {
            UE_LOG(LogFrostwakeWhiteboxCommandlet, Warning, TEXT("Imported HDRI cubemap not found; keeping dynamic skylight."));
        }
    }

    // Interior point lights (Movable, no bake) so the enclosed rooms read at runtime; the arctic sun
    // only reaches the open bow ice deck. WB_ prefix + auto WhiteboxTag → cleared on regenerate.
    struct FRoomLightSpec { const TCHAR* Label; FVector Location; FLinearColor Color; float Intensity; float Radius; };
    const TArray<FRoomLightSpec> RoomLights = {
        {TEXT("WB_Light_Corridor"), FVector(0, 0, 280), FLinearColor(0.95f, 0.96f, 1.0f), 3200.0f, 850.0f},
        {TEXT("WB_Light_Bridge"), FVector(1150, -430, 280), FLinearColor(0.95f, 0.97f, 1.0f), 3000.0f, 750.0f},
        {TEXT("WB_Light_Radio"), FVector(1150, 430, 280), FLinearColor(0.95f, 0.97f, 1.0f), 3000.0f, 750.0f},
        {TEXT("WB_Light_Quarters"), FVector(-150, -430, 280), FLinearColor(1.0f, 0.92f, 0.78f), 2600.0f, 750.0f},
        {TEXT("WB_Light_Infirmary"), FVector(-150, 430, 280), FLinearColor(0.9f, 0.95f, 1.0f), 2800.0f, 750.0f},
        {TEXT("WB_Light_Engine"), FVector(-1050, -360, 280), FLinearColor(1.0f, 0.85f, 0.62f), 3000.0f, 800.0f},
        {TEXT("WB_Light_Battery"), FVector(-1050, 360, 280), FLinearColor(1.0f, 0.9f, 0.72f), 2800.0f, 800.0f},
        {TEXT("WB_Light_Fuel"), FVector(-1850, 0, 280), FLinearColor(1.0f, 0.85f, 0.62f), 2600.0f, 800.0f},
    };
    for (const FRoomLightSpec& RoomLight : RoomLights)
    {
        AActor* PointLight = SpawnActor(APointLight::StaticClass(), RoomLight.Label, RoomLight.Location, FRotator::ZeroRotator, Error);
        if (!PointLight)
        {
            return false;
        }
        if (USceneComponent* PointRoot = PointLight->GetRootComponent())
        {
            PointRoot->SetMobility(EComponentMobility::Movable);
        }
        if (UPointLightComponent* PointComp = PointLight->FindComponentByClass<UPointLightComponent>())
        {
            PointComp->SetIntensity(RoomLight.Intensity);
            PointComp->SetAttenuationRadius(RoomLight.Radius);
            PointComp->SetLightColor(RoomLight.Color);
        }
    }

    // CC0 prop dressing (Poly Haven, provenanced in docs/asset-ledger-candidates.csv; imported in
    // cycle 88). Skipped gracefully if not imported. Scale/placement are first-pass — refine after
    // owner screenshot review (Poly Haven fbx are metric; verify they read at the right size).
    const TArray<FPropSpec> Props = {
        {TEXT("Barrel_01"), TEXT("WB_Prop_Barrel_A"), FVector(1650, 520, 5), 20.0f, 1.0f},
        {TEXT("Barrel_02"), TEXT("WB_Prop_Barrel_B"), FVector(1720, 600, 5), -35.0f, 1.0f},
        {TEXT("can_rusted"), TEXT("WB_Prop_Can_A"), FVector(1600, 560, 5), 0.0f, 1.0f},
        {TEXT("cardboard_box_01"), TEXT("WB_Prop_Crate_A"), FVector(-1850, -520, 5), 10.0f, 1.0f},
        {TEXT("cardboard_box_01"), TEXT("WB_Prop_Crate_B"), FVector(-1780, -560, 5), -25.0f, 1.0f},
        {TEXT("Lantern_01"), TEXT("WB_Prop_Lantern_A"), FVector(-150, -560, 185), 0.0f, 1.0f},
    };
    for (const FPropSpec& Prop : Props)
    {
        if (!SpawnProp(Prop, Error))
        {
            return false;
        }
    }

    // Atmosphere: light exponential height fog for cold polar depth (rendering-only; null-RHI smoke unaffected).
    if (AExponentialHeightFog* Fog = Cast<AExponentialHeightFog>(SpawnActor(AExponentialHeightFog::StaticClass(), TEXT("WB_Atmo_Fog"), FVector(0, 0, 120), FRotator::ZeroRotator, Error)))
    {
        if (UExponentialHeightFogComponent* FogComp = Fog->FindComponentByClass<UExponentialHeightFogComponent>())
        {
            FogComp->SetFogDensity(0.012f);
            FogComp->SetStartDistance(300.0f);
            FogComp->SetFogInscatteringColor(FLinearColor(0.42f, 0.5f, 0.6f));
        }
    }

    // Pickable items near the spawn so the player can pick up + scroll-select them (HUD hotbar).
    SpawnItemPickup(TEXT("Wrench"), TEXT("WB_Item_Wrench"), FVector(250, 60, 30), Error);
    SpawnItemPickup(TEXT("FuelDrum"), TEXT("WB_Item_FuelDrum"), FVector(250, -60, 30), Error);
    SpawnItemPickup(TEXT("Fuse"), TEXT("WB_Item_Fuse"), FVector(430, 60, 30), Error);
    SpawnItemPickup(TEXT("Headlamp"), TEXT("WB_Item_Headlamp"), FVector(430, -60, 30), Error);

    // Survival loop: a ration restores Food (F to eat the selected item); heat sources restore Warmth (E).
    SpawnItemPickup(TEXT("Ration"), TEXT("WB_Item_Ration"), FVector(120, 0, 30), Error);
    SpawnHeatSource(TEXT("WB_HeatSource_A"), FVector(600, 0, 40), Error);
    SpawnHeatSource(TEXT("WB_HeatSource_B"), FVector(-900, -300, 40), Error);

    World->MarkPackageDirty();
    if (!UEditorLoadingAndSavingUtils::SaveMap(World, MapPackage))
    {
        return Fail(Error, FString::Printf(TEXT("Could not save map %s."), *MapPackage));
    }

    UE_LOG(LogFrostwakeWhiteboxCommandlet, Display, TEXT("Created and saved %s."), *MapPackage);
    return true;
}

FString ShipSystemName(EFrostwakeShipSystem System)
{
    switch (System)
    {
    case EFrostwakeShipSystem::Hull:
        return TEXT("hull");
    case EFrostwakeShipSystem::Fuel:
        return TEXT("fuel");
    case EFrostwakeShipSystem::Engine:
        return TEXT("engine");
    case EFrostwakeShipSystem::Power:
        return TEXT("power");
    case EFrostwakeShipSystem::Radio:
        return TEXT("radio");
    case EFrostwakeShipSystem::Route:
        return TEXT("route");
    case EFrostwakeShipSystem::Heat:
        return TEXT("heat");
    case EFrostwakeShipSystem::Flooding:
        return TEXT("flooding");
    default:
        return TEXT("unknown");
    }
}

FString TaskModeName(EFrostwakeShipTaskMode Mode)
{
    return Mode == EFrostwakeShipTaskMode::Sabotage ? TEXT("sabotage") : TEXT("repair");
}

bool ValidateDefaultMaps(FString& Error)
{
    const FString ConfigPath = FPaths::ProjectConfigDir() / TEXT("DefaultEngine.ini");
    FString Text;
    if (!FFileHelper::LoadFileToString(Text, *ConfigPath))
    {
        return Fail(Error, FString::Printf(TEXT("Missing DefaultEngine.ini: %s"), *ConfigPath));
    }

    TMap<FString, FString> Values;
    TArray<FString> Lines;
    Text.ParseIntoArrayLines(Lines);
    for (FString Line : Lines)
    {
        Line.TrimStartAndEndInline();
        if (Line.IsEmpty() || Line.StartsWith(TEXT("[")) || !Line.Contains(TEXT("=")))
        {
            continue;
        }

        FString Key;
        FString Value;
        if (Line.Split(TEXT("="), &Key, &Value))
        {
            Values.Add(Key.TrimStartAndEnd(), Value.TrimStartAndEnd());
        }
    }

    // Boot goes through the dedicated front-end map (cycle 85); the whitebox is reached from it via
    // 一人モード/solo. So the engine boot maps must point at L_MainMenu, not the whitebox itself.
    const FString MenuMapAsset = TEXT("/Game/Maps/L_MainMenu.L_MainMenu");
    if (Values.FindRef(TEXT("EditorStartupMap")) != MenuMapAsset)
    {
        return Fail(Error, FString::Printf(TEXT("EditorStartupMap is not %s: %s"), *MenuMapAsset, *Values.FindRef(TEXT("EditorStartupMap"))));
    }
    if (Values.FindRef(TEXT("GameDefaultMap")) != MenuMapAsset)
    {
        return Fail(Error, FString::Printf(TEXT("GameDefaultMap is not %s: %s"), *MenuMapAsset, *Values.FindRef(TEXT("GameDefaultMap"))));
    }
    return true;
}

TMap<FString, AActor*> ActorsByLabel(UWorld* World)
{
    TMap<FString, AActor*> Result;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        Result.Add(It->GetActorLabel(), *It);
    }
    return Result;
}

bool ValidateRequiredLabels(const TMap<FString, AActor*>& Actors, FString& Error)
{
    const TArray<const TCHAR*> RequiredLabels = {
        TEXT("WB_Hull_MainDeck"),
        TEXT("WB_Bridge"),
        TEXT("WB_RadioRoom"),
        TEXT("WB_Infirmary"),
        TEXT("WB_QuartermasterStore"),
        TEXT("WB_EngineRoom"),
        TEXT("WB_FuelBay"),
        TEXT("WB_BatteryRoom"),
        TEXT("WB_IcePressureGate"),
        TEXT("TP_RouteControl"),
        TEXT("TP_RadioRepair"),
        TEXT("TP_FuelContamination"),
        TEXT("TP_PowerRepair"),
        TEXT("TP_EngineRepair"),
        TEXT("TP_HullFlooding"),
        TEXT("TASK_Repair_Radio"),
        TEXT("TASK_Sabotage_Radio"),
        TEXT("TASK_Repair_Power"),
        TEXT("TASK_Sabotage_Power"),
        TEXT("TASK_Repair_Engine"),
        TEXT("TASK_Sabotage_Fuel"),
        TEXT("TASK_Repair_HullFlooding"),
        TEXT("TASK_Sabotage_HullFlooding"),
        TEXT("TASK_Repair_Fuel"),
        TEXT("DOOR_RadioBulkhead"),
        TEXT("DOOR_EngineBulkhead"),
        TEXT("DOOR_HoldFloodBulkhead"),
    };

    for (const TCHAR* Label : RequiredLabels)
    {
        if (!Actors.Contains(Label))
        {
            return Fail(Error, FString::Printf(TEXT("Missing required whitebox label: %s"), Label));
        }
    }

    for (int32 Index = 1; Index <= 8; ++Index)
    {
        const FString Label = FString::Printf(TEXT("PS_Crew_%02d"), Index);
        if (!Actors.Contains(Label))
        {
            return Fail(Error, FString::Printf(TEXT("Missing crew player start: %s"), *Label));
        }
    }
    return true;
}

bool ValidateTaskConfig(const TMap<FString, AActor*>& Actors, FString& Error)
{
    const TArray<FTaskExpectation> Expectations = {
        {TEXT("TASK_Repair_Radio"), TEXT("radio"), TEXT("repair")},
        {TEXT("TASK_Sabotage_Radio"), TEXT("radio"), TEXT("sabotage")},
        {TEXT("TASK_Repair_Power"), TEXT("power"), TEXT("repair")},
        {TEXT("TASK_Sabotage_Power"), TEXT("power"), TEXT("sabotage")},
        {TEXT("TASK_Repair_Engine"), TEXT("engine"), TEXT("repair")},
        {TEXT("TASK_Sabotage_Fuel"), TEXT("fuel"), TEXT("sabotage")},
        {TEXT("TASK_Repair_HullFlooding"), TEXT("flooding"), TEXT("repair")},
        {TEXT("TASK_Sabotage_HullFlooding"), TEXT("flooding"), TEXT("sabotage")},
        {TEXT("TASK_Repair_Fuel"), TEXT("fuel"), TEXT("repair")},
    };

    for (const FTaskExpectation& Expectation : Expectations)
    {
        AActor* const* Found = Actors.Find(Expectation.Label);
        AFrostwakeShipTaskActor* Task = Found ? Cast<AFrostwakeShipTaskActor>(*Found) : nullptr;
        if (!Task)
        {
            return Fail(Error, FString::Printf(TEXT("%s is not an FrostwakeShipTaskActor."), Expectation.Label));
        }

        const FString ActualSystem = ShipSystemName(Task->GetTargetSystem());
        const FString ActualMode = TaskModeName(Task->GetTaskMode());
        if (ActualSystem != Expectation.System)
        {
            return Fail(Error, FString::Printf(TEXT("%s target system is not %s: %s"), Expectation.Label, Expectation.System, *ActualSystem));
        }
        if (ActualMode != Expectation.Mode)
        {
            return Fail(Error, FString::Printf(TEXT("%s task mode is not %s: %s"), Expectation.Label, Expectation.Mode, *ActualMode));
        }
    }
    return true;
}

bool ValidateDoorConfig(const TMap<FString, AActor*>& Actors, FString& Error)
{
    const TArray<const TCHAR*> DoorLabels = {
        TEXT("DOOR_RadioBulkhead"),
        TEXT("DOOR_EngineBulkhead"),
        TEXT("DOOR_HoldFloodBulkhead"),
    };

    for (const TCHAR* Label : DoorLabels)
    {
        AActor* const* Found = Actors.Find(Label);
        if (!Found || !Cast<AFrostwakeDoorActor>(*Found))
        {
            return Fail(Error, FString::Printf(TEXT("%s is not an FrostwakeDoorActor."), Label));
        }
    }
    return true;
}

bool ValidateWhitebox(FString& Error)
{
    UWorld* World = nullptr;
    if (!LoadMapForValidation(World, Error))
    {
        return false;
    }

    const TMap<FString, AActor*> Actors = ActorsByLabel(World);
    if (!ValidateRequiredLabels(Actors, Error) || !ValidateDefaultMaps(Error) || !ValidateTaskConfig(Actors, Error) || !ValidateDoorConfig(Actors, Error))
    {
        return false;
    }

    UE_LOG(LogFrostwakeWhiteboxCommandlet, Display, TEXT("%s passed validation with %d actors."), *MapPackage, Actors.Num());
    return true;
}
}

UCreateIcebreakerWhiteboxCommandlet::UCreateIcebreakerWhiteboxCommandlet()
{
    IsClient = false;
    IsEditor = true;
    IsServer = false;
    LogToConsole = true;
}

int32 UCreateIcebreakerWhiteboxCommandlet::Main(const FString& Params)
{
    FString Error;
    return FrostwakeWhitebox::CreateWhitebox(Error) ? 0 : 1;
}

UValidateIcebreakerWhiteboxCommandlet::UValidateIcebreakerWhiteboxCommandlet()
{
    IsClient = false;
    IsEditor = true;
    IsServer = false;
    LogToConsole = true;
}

int32 UValidateIcebreakerWhiteboxCommandlet::Main(const FString& Params)
{
    FString Error;
    return FrostwakeWhitebox::ValidateWhitebox(Error) ? 0 : 1;
}

namespace FrostwakeVisualPOC
{
const FString MapPackage = TEXT("/Game/Maps/L_FrostwakeVisualPOC");
const FName VisualPocTag = TEXT("FrostwakeVisualPOC");
const FString AmbientCgSourceRoot = TEXT("Content/ThirdParty/Quarantine/ambientCG");
const FString AmbientCgDestinationRoot = TEXT("/Game/ThirdParty/Quarantine/ambientCG");

struct FMaterialSpec
{
    const TCHAR* Name;
};

struct FVisualCubeSpec
{
    const TCHAR* Label;
    FVector Location;
    FVector Size;
    const TCHAR* MaterialName;
};

struct FVisualPointSpec
{
    const TCHAR* Label;
    FVector Location;
};

struct FAmbientCgAssetSpec
{
    const TCHAR* AssetId;
    const TCHAR* RequiredColorAsset;
};

bool Fail(FString& Error, const FString& Message)
{
    Error = Message;
    UE_LOG(LogFrostwakeWhiteboxCommandlet, Error, TEXT("%s"), *Message);
    return false;
}

bool IsVisualPocActor(const AActor* Actor)
{
    if (!Actor)
    {
        return false;
    }

    const FString Label = Actor->GetActorLabel();
    return Label.StartsWith(TEXT("POC_")) || Label.StartsWith(TEXT("CC0_")) || Actor->Tags.Contains(VisualPocTag);
}

bool LoadOrCreateMap(UWorld*& OutWorld, FString& Error)
{
    if (FPackageName::DoesPackageExist(MapPackage))
    {
        OutWorld = UEditorLoadingAndSavingUtils::LoadMap(MapPackage);
    }
    else
    {
        OutWorld = UEditorLoadingAndSavingUtils::NewBlankMap(false);
    }

    if (!OutWorld)
    {
        return Fail(Error, FString::Printf(TEXT("Could not load or create %s."), *MapPackage));
    }
    return true;
}

bool ClearExistingVisualPoc(UWorld* World, FString& Error)
{
    TArray<AActor*> ActorsToDestroy;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (IsVisualPocActor(*It))
        {
            ActorsToDestroy.Add(*It);
        }
    }

    for (AActor* Actor : ActorsToDestroy)
    {
        if (Actor && !World->EditorDestroyActor(Actor, true))
        {
            return Fail(Error, FString::Printf(TEXT("Could not remove visual POC actor %s."), *GetNameSafe(Actor)));
        }
    }
    return true;
}

UMaterial* CreateOrUpdateMaterial(const FMaterialSpec& Spec, FString& Error)
{
    UMaterial* Material = LoadObject<UMaterial>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (!Material)
    {
        Fail(Error, FString::Printf(TEXT("Could not load placeholder material for %s."), Spec.Name));
        return nullptr;
    }
    return Material;
}

AActor* SpawnActor(UClass* ActorClass, const TCHAR* Label, const FVector& Location, const FRotator& Rotation, FString& Error)
{
    UEditorActorSubsystem* ActorSubsystem = GEditor ? GEditor->GetEditorSubsystem<UEditorActorSubsystem>() : nullptr;
    AActor* Actor = ActorSubsystem ? ActorSubsystem->SpawnActorFromClass(TSubclassOf<AActor>(ActorClass), Location, Rotation, false) : nullptr;
    if (!Actor)
    {
        Fail(Error, FString::Printf(TEXT("Could not spawn actor %s."), Label));
        return nullptr;
    }

    Actor->SetActorLabel(Label);
    Actor->Tags.AddUnique(VisualPocTag);
    return Actor;
}

bool SpawnCube(const FVisualCubeSpec& Spec, UStaticMesh* CubeMesh, const TMap<FString, UMaterial*>& Materials, FString& Error)
{
    AStaticMeshActor* Actor = Cast<AStaticMeshActor>(SpawnActor(AStaticMeshActor::StaticClass(), Spec.Label, Spec.Location, FRotator::ZeroRotator, Error));
    if (!Actor)
    {
        return false;
    }

    Actor->SetActorScale3D(FVector(Spec.Size.X / 100.0, Spec.Size.Y / 100.0, Spec.Size.Z / 100.0));
    UStaticMeshComponent* MeshComponent = Actor->GetStaticMeshComponent();
    if (!MeshComponent)
    {
        return Fail(Error, FString::Printf(TEXT("StaticMeshComponent missing on %s."), Spec.Label));
    }
    MeshComponent->SetStaticMesh(CubeMesh);
    if (UMaterial* const* Material = Materials.Find(Spec.MaterialName))
    {
        MeshComponent->SetMaterial(0, *Material);
    }
    return true;
}

bool SpawnPoint(const FVisualPointSpec& Spec, FString& Error)
{
    return SpawnActor(ATargetPoint::StaticClass(), Spec.Label, Spec.Location, FRotator::ZeroRotator, Error) != nullptr;
}

bool CreateVisualPoc(FString& Error)
{
    UWorld* World = nullptr;
    if (!LoadOrCreateMap(World, Error) || !ClearExistingVisualPoc(World, Error))
    {
        return false;
    }

    UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!CubeMesh)
    {
        return Fail(Error, TEXT("Could not load /Engine/BasicShapes/Cube.Cube."));
    }

    const TArray<FMaterialSpec> MaterialSpecs = {
        {TEXT("M_POC_DiamondPlate009_Bulkhead")},
        {TEXT("M_POC_PaintedMetal004_Warning")},
        {TEXT("M_POC_MetalWalkway014_Rust")},
        {TEXT("M_POC_Snow015_IceDeck")},
        {TEXT("M_POC_Ice003_WindowFrost")},
        {TEXT("M_POC_EmergencyRed")},
        {TEXT("M_POC_RadioGreen")},
    };

    TMap<FString, UMaterial*> Materials;
    for (const FMaterialSpec& MaterialSpec : MaterialSpecs)
    {
        UMaterial* Material = CreateOrUpdateMaterial(MaterialSpec, Error);
        if (!Material)
        {
            return false;
        }
        Materials.Add(MaterialSpec.Name, Material);
    }

    const TArray<FVisualCubeSpec> Cubes = {
        {TEXT("POC_Zone_CentralCorridor_Deck"), FVector(0, 0, 0), FVector(2200, 420, 36), TEXT("M_POC_DiamondPlate009_Bulkhead")},
        {TEXT("POC_Zone_CentralCorridor_PortBulkhead"), FVector(0, -240, 190), FVector(2200, 40, 360), TEXT("M_POC_DiamondPlate009_Bulkhead")},
        {TEXT("POC_Zone_CentralCorridor_StarboardBulkhead"), FVector(0, 240, 190), FVector(2200, 40, 360), TEXT("M_POC_DiamondPlate009_Bulkhead")},
        {TEXT("POC_Central_SightBreak_RadioRack"), FVector(-260, -130, 150), FVector(220, 80, 260), TEXT("M_POC_RadioGreen")},
        {TEXT("POC_Central_SightBreak_BatteryLocker"), FVector(290, 130, 145), FVector(260, 90, 250), TEXT("M_POC_MetalWalkway014_Rust")},
        {TEXT("POC_Central_WarningStripe_A"), FVector(-820, 0, 40), FVector(220, 430, 18), TEXT("M_POC_PaintedMetal004_Warning")},
        {TEXT("POC_Central_WarningStripe_B"), FVector(820, 0, 40), FVector(220, 430, 18), TEXT("M_POC_PaintedMetal004_Warning")},
        {TEXT("POC_Zone_BatteryBay_Deck"), FVector(-1500, 0, -10), FVector(880, 760, 36), TEXT("M_POC_DiamondPlate009_Bulkhead")},
        {TEXT("POC_BatteryBay_BatteryRack_A"), FVector(-1620, -230, 150), FVector(520, 100, 260), TEXT("M_POC_MetalWalkway014_Rust")},
        {TEXT("POC_BatteryBay_BatteryRack_B"), FVector(-1620, 230, 150), FVector(520, 100, 260), TEXT("M_POC_MetalWalkway014_Rust")},
        {TEXT("POC_BatteryBay_PowerBus"), FVector(-1220, 0, 185), FVector(80, 560, 110), TEXT("M_POC_EmergencyRed")},
        {TEXT("POC_BatteryBay_ServicePanel_Repair"), FVector(-1140, -280, 135), FVector(110, 90, 190), TEXT("M_POC_RadioGreen")},
        {TEXT("POC_BatteryBay_ServicePanel_Sabotage"), FVector(-1140, 280, 135), FVector(110, 90, 190), TEXT("M_POC_EmergencyRed")},
        {TEXT("POC_Zone_ExteriorIceDeck"), FVector(1500, 0, -20), FVector(1050, 860, 42), TEXT("M_POC_Snow015_IceDeck")},
        {TEXT("POC_Exterior_Rail_Port"), FVector(1500, -450, 100), FVector(1060, 40, 170), TEXT("M_POC_DiamondPlate009_Bulkhead")},
        {TEXT("POC_Exterior_Rail_Starboard"), FVector(1500, 450, 100), FVector(1060, 40, 170), TEXT("M_POC_DiamondPlate009_Bulkhead")},
        {TEXT("POC_Exterior_AirlockDoor"), FVector(950, 0, 160), FVector(80, 360, 280), TEXT("M_POC_PaintedMetal004_Warning")},
        {TEXT("POC_Exterior_Winch"), FVector(1660, 220, 105), FVector(300, 180, 170), TEXT("M_POC_MetalWalkway014_Rust")},
        {TEXT("POC_Exterior_FrostedWindow"), FVector(1110, -250, 185), FVector(40, 180, 140), TEXT("M_POC_Ice003_WindowFrost")},
        {TEXT("CC0_Pending_DiamondPlate009_Surface"), FVector(0, 330, 100), FVector(260, 36, 160), TEXT("M_POC_DiamondPlate009_Bulkhead")},
        {TEXT("CC0_Pending_Snow015_Surface"), FVector(1640, -245, 42), FVector(220, 220, 26), TEXT("M_POC_Snow015_IceDeck")},
        {TEXT("CC0_Pending_Ice003_Surface"), FVector(1640, -40, 42), FVector(220, 170, 24), TEXT("M_POC_Ice003_WindowFrost")},
    };
    for (const FVisualCubeSpec& Cube : Cubes)
    {
        if (!SpawnCube(Cube, CubeMesh, Materials, Error))
        {
            return false;
        }
    }

    const TArray<FVisualPointSpec> Points = {
        {TEXT("POC_Acceptance_CentralCorridor"), FVector(0, 0, 180)},
        {TEXT("POC_Acceptance_BatteryBay"), FVector(-1500, 0, 180)},
        {TEXT("POC_Acceptance_ExteriorIceDeck"), FVector(1500, 0, 180)},
    };
    for (const FVisualPointSpec& Point : Points)
    {
        if (!SpawnPoint(Point, Error))
        {
            return false;
        }
    }

    if (!SpawnActor(ADirectionalLight::StaticClass(), TEXT("POC_Light_WhiteoutSun"), FVector(600, -900, 2100), FRotator(-48, -25, 0), Error))
    {
        return false;
    }
    if (!SpawnActor(ASkyLight::StaticClass(), TEXT("POC_Light_ColdSkylight"), FVector(0, 0, 1200), FRotator::ZeroRotator, Error))
    {
        return false;
    }

    World->MarkPackageDirty();
    if (!UEditorLoadingAndSavingUtils::SaveMap(World, MapPackage))
    {
        return Fail(Error, FString::Printf(TEXT("Could not save map %s."), *MapPackage));
    }

    UE_LOG(LogFrostwakeWhiteboxCommandlet, Display, TEXT("Created and saved %s."), *MapPackage);
    return true;
}

bool ValidateVisualPoc(FString& Error)
{
    if (!FPackageName::DoesPackageExist(MapPackage))
    {
        return Fail(Error, FString::Printf(TEXT("Missing map asset: %s"), *MapPackage));
    }

    UWorld* World = UEditorLoadingAndSavingUtils::LoadMap(MapPackage);
    if (!World)
    {
        return Fail(Error, FString::Printf(TEXT("Could not load map: %s"), *MapPackage));
    }

    TSet<FString> Labels;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        Labels.Add(It->GetActorLabel());
    }

    const TArray<const TCHAR*> RequiredLabels = {
        TEXT("POC_Zone_CentralCorridor_Deck"),
        TEXT("POC_Central_SightBreak_RadioRack"),
        TEXT("POC_Zone_BatteryBay_Deck"),
        TEXT("POC_BatteryBay_ServicePanel_Repair"),
        TEXT("POC_Zone_ExteriorIceDeck"),
        TEXT("POC_Exterior_AirlockDoor"),
        TEXT("CC0_Pending_DiamondPlate009_Surface"),
        TEXT("CC0_Pending_Snow015_Surface"),
        TEXT("CC0_Pending_Ice003_Surface"),
    };
    for (const TCHAR* RequiredLabel : RequiredLabels)
    {
        if (!Labels.Contains(RequiredLabel))
        {
            return Fail(Error, FString::Printf(TEXT("Missing visual POC label: %s"), RequiredLabel));
        }
    }

    UE_LOG(LogFrostwakeWhiteboxCommandlet, Display, TEXT("%s passed validation with %d actors."), *MapPackage, Labels.Num());
    return true;
}

TArray<FAmbientCgAssetSpec> AmbientCgAssets()
{
    return {
        {TEXT("DiamondPlate009"), TEXT("DiamondPlate009_1K-JPG_Color")},
        {TEXT("PaintedMetal004"), TEXT("PaintedMetal004_1K-JPG_Color")},
        {TEXT("MetalWalkway014"), TEXT("MetalWalkway014_1K-JPG_Color")},
        {TEXT("Snow015"), TEXT("Snow015_1K-JPG_Color")},
        {TEXT("Ice003"), TEXT("Ice003_1K-JPG_Color")},
    };
}

bool CollectAmbientCgSourceFiles(const FAmbientCgAssetSpec& Spec, TArray<FString>& OutFiles, FString& Error)
{
    const FString SourceDir = FPaths::Combine(FPaths::ProjectDir(), AmbientCgSourceRoot, Spec.AssetId, TEXT("1K-JPG"));
    if (!FPaths::DirectoryExists(SourceDir))
    {
        return Fail(Error, FString::Printf(TEXT("Missing ambientCG source directory: %s"), *SourceDir));
    }

    IFileManager::Get().FindFilesRecursive(OutFiles, *SourceDir, TEXT("*.jpg"), true, false, false);
    if (OutFiles.IsEmpty())
    {
        return Fail(Error, FString::Printf(TEXT("No JPG files found in ambientCG source directory: %s"), *SourceDir));
    }
    return true;
}

bool ImportAmbientCgVisualPocAssets(FString& Error)
{
    if (!FSlateApplication::IsInitialized())
    {
        FSlateApplication::InitializeAsStandaloneApplication(GetStandardStandaloneRenderer());
    }

    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
    int32 ImportedCount = 0;

    for (const FAmbientCgAssetSpec& Spec : AmbientCgAssets())
    {
        TArray<FString> SourceFiles;
        if (!CollectAmbientCgSourceFiles(Spec, SourceFiles, Error))
        {
            return false;
        }

        UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
        ImportData->Filenames = SourceFiles;
        ImportData->DestinationPath = FString::Printf(TEXT("%s/%s"), *AmbientCgDestinationRoot, Spec.AssetId);
        ImportData->bReplaceExisting = true;
        ImportData->bSkipReadOnly = true;

        const TArray<UObject*> ImportedObjects = AssetTools.ImportAssetsAutomated(ImportData);
        ImportedCount += ImportedObjects.Num();
        for (UObject* ImportedObject : ImportedObjects)
        {
            if (!ImportedObject || !ImportedObject->GetPackage())
            {
                return Fail(Error, FString::Printf(TEXT("Imported ambientCG object for %s has no package."), Spec.AssetId));
            }

            UPackage* Package = ImportedObject->GetPackage();
            const FString PackageFilename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
            FSavePackageArgs SaveArgs;
            SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
            SaveArgs.Error = GWarn;
            if (!UPackage::SavePackage(Package, ImportedObject, *PackageFilename, SaveArgs))
            {
                return Fail(Error, FString::Printf(TEXT("Could not save imported ambientCG package: %s"), *PackageFilename));
            }
        }
    }

    UE_LOG(LogFrostwakeWhiteboxCommandlet, Display, TEXT("Imported %d ambientCG source files into %s."), ImportedCount, *AmbientCgDestinationRoot);
    return ImportedCount > 0 || Fail(Error, TEXT("No ambientCG assets were imported."));
}

bool ValidateAmbientCgVisualPocAssets(FString& Error)
{
    for (const FAmbientCgAssetSpec& Spec : AmbientCgAssets())
    {
        const FString ObjectPath = FString::Printf(
            TEXT("%s/%s/%s.%s"),
            *AmbientCgDestinationRoot,
            Spec.AssetId,
            Spec.RequiredColorAsset,
            Spec.RequiredColorAsset);
        UObject* Asset = LoadObject<UObject>(nullptr, *ObjectPath);
        if (!Asset)
        {
            return Fail(Error, FString::Printf(TEXT("Missing imported ambientCG asset: %s"), *ObjectPath));
        }
    }

    UE_LOG(LogFrostwakeWhiteboxCommandlet, Display, TEXT("ambientCG visual POC assets passed validation."));
    return true;
}
}

UCreateFrostwakeVisualPocCommandlet::UCreateFrostwakeVisualPocCommandlet()
{
    IsClient = false;
    IsEditor = true;
    IsServer = false;
    LogToConsole = true;
}

int32 UCreateFrostwakeVisualPocCommandlet::Main(const FString& Params)
{
    FString Error;
    return FrostwakeVisualPOC::CreateVisualPoc(Error) ? 0 : 1;
}

UValidateFrostwakeVisualPocCommandlet::UValidateFrostwakeVisualPocCommandlet()
{
    IsClient = false;
    IsEditor = true;
    IsServer = false;
    LogToConsole = true;
}

int32 UValidateFrostwakeVisualPocCommandlet::Main(const FString& Params)
{
    FString Error;
    return FrostwakeVisualPOC::ValidateVisualPoc(Error) ? 0 : 1;
}

UImportAmbientCgVisualPocAssetsCommandlet::UImportAmbientCgVisualPocAssetsCommandlet()
{
    IsClient = false;
    IsEditor = true;
    IsServer = false;
    LogToConsole = true;
}

int32 UImportAmbientCgVisualPocAssetsCommandlet::Main(const FString& Params)
{
    FString Error;
    return FrostwakeVisualPOC::ImportAmbientCgVisualPocAssets(Error) ? 0 : 1;
}

UValidateAmbientCgVisualPocAssetsCommandlet::UValidateAmbientCgVisualPocAssetsCommandlet()
{
    IsClient = false;
    IsEditor = true;
    IsServer = false;
    LogToConsole = true;
}

int32 UValidateAmbientCgVisualPocAssetsCommandlet::Main(const FString& Params)
{
    FString Error;
    return FrostwakeVisualPOC::ValidateAmbientCgVisualPocAssets(Error) ? 0 : 1;
}

UCreateMainMenuCommandlet::UCreateMainMenuCommandlet()
{
    IsClient = false;
    IsEditor = true;
    IsServer = false;
    LogToConsole = true;
}

int32 UCreateMainMenuCommandlet::Main(const FString& Params)
{
    const FString MapPackage = TEXT("/Game/Maps/L_MainMenu");

    UWorld* World = FPackageName::DoesPackageExist(MapPackage)
        ? UEditorLoadingAndSavingUtils::LoadMap(MapPackage)
        : UEditorLoadingAndSavingUtils::NewBlankMap(false);
    if (!World)
    {
        UE_LOG(LogFrostwakeWhiteboxCommandlet, Error, TEXT("Could not load or create %s."), *MapPackage);
        return 1;
    }

    if (AWorldSettings* WorldSettings = World->GetWorldSettings())
    {
        WorldSettings->DefaultGameMode = AFrostwakeMenuGameMode::StaticClass();
    }

    if (!UEditorLoadingAndSavingUtils::SaveMap(World, MapPackage))
    {
        UE_LOG(LogFrostwakeWhiteboxCommandlet, Error, TEXT("Could not save %s."), *MapPackage);
        return 1;
    }

    UE_LOG(LogFrostwakeWhiteboxCommandlet, Display, TEXT("Created main menu map %s with menu GameMode."), *MapPackage);
    return 0;
}

namespace FrostwakePolyhaven
{
const FString SourceRoot = TEXT("Content/ThirdParty/Quarantine/polyhaven");
const FString DestRoot = TEXT("/Game/ThirdParty/Quarantine/polyhaven");

bool Fail(FString& Error, const FString& Message)
{
    Error = Message;
    UE_LOG(LogFrostwakeWhiteboxCommandlet, Error, TEXT("%s"), *Message);
    return false;
}

bool IsImportable(const FString& File)
{
    const FString Ext = FPaths::GetExtension(File).ToLower();
    return Ext == TEXT("fbx") || Ext == TEXT("hdr") || Ext == TEXT("png") || Ext == TEXT("jpg") || Ext == TEXT("exr");
}

int32 ImportDir(IAssetTools& AssetTools, const FString& AbsSourceDir, const FString& DestPath)
{
    if (!FPaths::DirectoryExists(AbsSourceDir))
    {
        return 0;
    }

    TArray<FString> Files;
    IFileManager::Get().FindFilesRecursive(Files, *AbsSourceDir, TEXT("*.*"), true, false, false);
    TArray<FString> Importable;
    for (const FString& File : Files)
    {
        if (IsImportable(File))
        {
            Importable.Add(File);
        }
    }
    if (Importable.IsEmpty())
    {
        return 0;
    }

    UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
    ImportData->Filenames = Importable;
    ImportData->DestinationPath = DestPath;
    ImportData->bReplaceExisting = true;
    ImportData->bSkipReadOnly = true;

    const TArray<UObject*> Imported = AssetTools.ImportAssetsAutomated(ImportData);
    for (UObject* Object : Imported)
    {
        if (!Object || !Object->GetPackage())
        {
            continue;
        }
        UPackage* Package = Object->GetPackage();
        const FString PackageFilename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        SaveArgs.Error = GWarn;
        UPackage::SavePackage(Package, Object, *PackageFilename, SaveArgs);
    }
    UE_LOG(LogFrostwakeWhiteboxCommandlet, Display, TEXT("Imported %d asset(s) from %s into %s."), Imported.Num(), *AbsSourceDir, *DestPath);
    return Imported.Num();
}

bool ImportPolyhaven(FString& Error)
{
    if (!FSlateApplication::IsInitialized())
    {
        FSlateApplication::InitializeAsStandaloneApplication(GetStandardStandaloneRenderer());
    }
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();

    const FString Base = FPaths::Combine(FPaths::ProjectDir(), SourceRoot);
    if (!FPaths::DirectoryExists(Base))
    {
        return Fail(Error, FString::Printf(TEXT("Missing Poly Haven quarantine dir: %s"), *Base));
    }

    int32 Count = 0;
    Count += ImportDir(AssetTools, FPaths::Combine(Base, TEXT("hdri")), DestRoot + TEXT("/hdri"));

    const FString ModelsBase = FPaths::Combine(Base, TEXT("models"));
    TArray<FString> ModelDirs;
    IFileManager::Get().FindFiles(ModelDirs, *(ModelsBase / TEXT("*")), false, true);
    for (const FString& Slug : ModelDirs)
    {
        Count += ImportDir(AssetTools, FPaths::Combine(ModelsBase, Slug), DestRoot + TEXT("/models/") + Slug);
    }

    UE_LOG(LogFrostwakeWhiteboxCommandlet, Display, TEXT("Imported %d Poly Haven CC0 asset(s) into %s."), Count, *DestRoot);
    return Count > 0 ? true : Fail(Error, TEXT("No Poly Haven assets were imported."));
}
}

UImportPolyhavenCc0AssetsCommandlet::UImportPolyhavenCc0AssetsCommandlet()
{
    IsClient = false;
    IsEditor = true;
    IsServer = false;
    LogToConsole = true;
}

int32 UImportPolyhavenCc0AssetsCommandlet::Main(const FString& Params)
{
    FString Error;
    return FrostwakePolyhaven::ImportPolyhaven(Error) ? 0 : 1;
}

namespace FrostwakeFonts
{
// Quarantined OFL Noto Sans CJK fonts (fetched by Tools/assets/fetch_noto_cjk_ofl.ps1).
const FString FontDestRoot = TEXT("/Game/ThirdParty/Quarantine/fonts");

bool ImportOneFont(IAssetTools& AssetTools, const FString& AbsTtf, const FString& DestPath, FString& Error)
{
    if (!FPaths::FileExists(AbsTtf))
    {
        Error = FString::Printf(TEXT("Missing font file: %s (run Tools/assets/fetch_noto_cjk_ofl.ps1)"), *AbsTtf);
        return false;
    }

    UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
    ImportData->Filenames = { AbsTtf };
    ImportData->DestinationPath = DestPath;
    ImportData->bReplaceExisting = true;
    ImportData->bSkipReadOnly = true;

    const TArray<UObject*> Imported = AssetTools.ImportAssetsAutomated(ImportData);
    if (Imported.Num() == 0)
    {
        Error = FString::Printf(TEXT("Font import produced no asset for: %s"), *AbsTtf);
        return false;
    }

    for (UObject* Object : Imported)
    {
        UPackage* Package = Object ? Object->GetPackage() : nullptr;
        if (!Package)
        {
            Error = TEXT("Imported font asset has no package.");
            return false;
        }
        const FString PackageFilename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        SaveArgs.Error = GWarn;
        if (!UPackage::SavePackage(Package, Object, *PackageFilename, SaveArgs))
        {
            Error = FString::Printf(TEXT("Could not save imported font package: %s"), *PackageFilename);
            return false;
        }
        UE_LOG(LogFrostwakeWhiteboxCommandlet, Display, TEXT("Imported font asset %s (%s)"), *Object->GetPathName(), *Object->GetClass()->GetName());
    }
    return true;
}

bool ImportNotoCjkFonts(FString& Error)
{
    if (!FSlateApplication::IsInitialized())
    {
        FSlateApplication::InitializeAsStandaloneApplication(GetStandardStandaloneRenderer());
    }
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
    const FString Base = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("ThirdParty/Quarantine/fonts"));

    if (!ImportOneFont(AssetTools, FPaths::Combine(Base, TEXT("notosansjp"), TEXT("NotoSansJP-VF.ttf")), FontDestRoot + TEXT("/NotoSansJP"), Error))
    {
        return false;
    }
    if (!ImportOneFont(AssetTools, FPaths::Combine(Base, TEXT("notosanssc"), TEXT("NotoSansSC-VF.ttf")), FontDestRoot + TEXT("/NotoSansSC"), Error))
    {
        return false;
    }

    UE_LOG(LogFrostwakeWhiteboxCommandlet, Display, TEXT("Imported Noto Sans JP + SC font faces into %s."), *FontDestRoot);
    return true;
}
}

UImportNotoCjkFontAssetsCommandlet::UImportNotoCjkFontAssetsCommandlet()
{
    IsClient = false;
    IsEditor = true;
    IsServer = false;
    LogToConsole = true;
}

int32 UImportNotoCjkFontAssetsCommandlet::Main(const FString& Params)
{
    FString Error;
    if (!FrostwakeFonts::ImportNotoCjkFonts(Error))
    {
        UE_LOG(LogFrostwakeWhiteboxCommandlet, Error, TEXT("ImportNotoCjkFonts failed: %s"), *Error);
        return 1;
    }
    return 0;
}
