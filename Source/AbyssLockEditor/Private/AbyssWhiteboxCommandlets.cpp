#include "AbyssWhiteboxCommandlets.h"

#include "AbyssDoorActor.h"
#include "AbyssMenuGameMode.h"
#include "AbyssShipTaskActor.h"
#include "AssetToolsModule.h"
#include "AutomatedAssetImportData.h"
#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
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

DEFINE_LOG_CATEGORY_STATIC(LogAbyssWhiteboxCommandlet, Log, All);

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
    UE_LOG(LogAbyssWhiteboxCommandlet, Error, TEXT("%s"), *Message);
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
    AAbyssShipTaskActor* Task = Cast<AAbyssShipTaskActor>(SpawnActor(AAbyssShipTaskActor::StaticClass(), Spec.Label, Spec.Location, FRotator::ZeroRotator, Error));
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
    AAbyssDoorActor* Door = Cast<AAbyssDoorActor>(SpawnActor(AAbyssDoorActor::StaticClass(), Spec.Label, Spec.Location, FRotator(0.0, Spec.Yaw, 0.0), Error));
    if (!Door)
    {
        return false;
    }
    Door->ConfigureDoorByName(FName(Spec.DoorId));
    return true;
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

    const TArray<FCubeSpec> Cubes = {
        {TEXT("WB_Hull_MainDeck"), FVector(0, 0, -20), FVector(4200, 1350, 40)},
        {TEXT("WB_Hull_LowerServiceDeck"), FVector(-250, 0, -260), FVector(2900, 950, 40)},
        {TEXT("WB_Foredeck_IceWorkArea"), FVector(1650, 0, 20), FVector(900, 1200, 40)},
        {TEXT("WB_AftDeck_MusterArea"), FVector(-1750, 0, 20), FVector(700, 1050, 40)},
        {TEXT("WB_PortHullWall"), FVector(0, -700, 150), FVector(4200, 60, 340)},
        {TEXT("WB_StarboardHullWall"), FVector(0, 700, 150), FVector(4200, 60, 340)},
        {TEXT("WB_BowBulkhead"), FVector(2120, 0, 150), FVector(60, 1350, 340)},
        {TEXT("WB_SternBulkhead"), FVector(-2120, 0, 150), FVector(60, 1350, 340)},
        {TEXT("WB_Bridge"), FVector(1100, 0, 320), FVector(620, 520, 260)},
        {TEXT("WB_RadioRoom"), FVector(450, -380, 170), FVector(520, 360, 260)},
        {TEXT("WB_Infirmary"), FVector(450, 380, 170), FVector(520, 360, 260)},
        {TEXT("WB_Wardroom"), FVector(-250, 380, 170), FVector(560, 360, 260)},
        {TEXT("WB_QuartermasterStore"), FVector(-250, -380, 170), FVector(560, 360, 260)},
        {TEXT("WB_EngineRoom"), FVector(-1050, 0, 170), FVector(720, 640, 300)},
        {TEXT("WB_FuelBay"), FVector(-1550, -360, 170), FVector(520, 380, 260)},
        {TEXT("WB_BatteryRoom"), FVector(-1550, 360, 170), FVector(520, 380, 260)},
        {TEXT("WB_Hold"), FVector(850, 0, -120), FVector(700, 760, 220)},
        {TEXT("WB_CentralCompanionway"), FVector(-150, 0, 40), FVector(2100, 280, 80)},
        {TEXT("WB_PortSidePassage"), FVector(-400, -520, 40), FVector(2600, 180, 80)},
        {TEXT("WB_StarboardSidePassage"), FVector(-400, 520, 40), FVector(2600, 180, 80)},
        {TEXT("WB_BridgeLadder"), FVector(900, 0, 160), FVector(220, 220, 280)},
        {TEXT("WB_LowerDeckLadder"), FVector(100, 0, -120), FVector(220, 220, 260)},
        {TEXT("WB_IcePressureGate"), FVector(1980, 0, 110), FVector(90, 1100, 180)},
        {TEXT("WB_FuelLine"), FVector(-1120, -560, 90), FVector(1120, 80, 100)},
        {TEXT("WB_PumpManifold"), FVector(-960, 560, 90), FVector(780, 80, 100)},
        {TEXT("WB_RadioMastBase"), FVector(650, -520, 240), FVector(160, 160, 360)},
        {TEXT("WB_DeckWinch"), FVector(1570, 420, 120), FVector(360, 220, 180)},
        {TEXT("WB_CargoCrates"), FVector(1350, -420, 120), FVector(420, 260, 180)},
    };
    for (const FCubeSpec& Cube : Cubes)
    {
        if (!SpawnCube(Cube, CubeMesh, Error))
        {
            return false;
        }
    }

    const TArray<FPlayerStartSpec> PlayerStarts = {
        {1, FVector(-1800, -320, 120), 15},
        {2, FVector(-1800, 0, 120), 0},
        {3, FVector(-1800, 320, 120), -15},
        {4, FVector(-1350, -510, 120), 35},
        {5, FVector(-1350, 510, 120), -35},
        {6, FVector(-650, -520, 120), 60},
        {7, FVector(-650, 520, 120), -60},
        {8, FVector(-80, 0, 120), 0},
    };
    for (const FPlayerStartSpec& PlayerStart : PlayerStarts)
    {
        if (!SpawnPlayerStart(PlayerStart, Error))
        {
            return false;
        }
    }

    const TArray<FTargetSpec> Targets = {
        {TEXT("TP_RouteControl"), FVector(1140, 0, 500)},
        {TEXT("TP_RadioRepair"), FVector(450, -380, 330)},
        {TEXT("TP_MedicalRescue"), FVector(450, 380, 330)},
        {TEXT("TP_FuelContamination"), FVector(-1550, -360, 330)},
        {TEXT("TP_PowerRepair"), FVector(-1550, 360, 330)},
        {TEXT("TP_EngineRepair"), FVector(-1050, 0, 360)},
        {TEXT("TP_IcePressureObjective"), FVector(1980, 0, 260)},
        {TEXT("TP_HullFlooding"), FVector(850, 0, 40)},
    };
    for (const FTargetSpec& Target : Targets)
    {
        if (!SpawnTarget(Target, Error))
        {
            return false;
        }
    }

    const TArray<FTaskSpec> Tasks = {
        {TEXT("TASK_Repair_Radio"), TEXT("Radio"), false, FVector(450, -220, 150), 0.40f, false, false, false},
        {TEXT("TASK_Sabotage_Radio"), TEXT("Radio"), true, FVector(450, -540, 150), 0.35f, false, true, true},
        {TEXT("TASK_Repair_Power"), TEXT("Power"), false, FVector(-1450, 420, 150), 0.40f, false, false, false},
        {TEXT("TASK_Sabotage_Power"), TEXT("Power"), true, FVector(-1450, 560, 150), 0.35f, false, true, true},
        {TEXT("TASK_Repair_Engine"), TEXT("Engine"), false, FVector(-1050, -160, 150), 0.30f, false, false, false},
        {TEXT("TASK_Sabotage_Fuel"), TEXT("Fuel"), true, FVector(-1550, -520, 150), 0.35f, false, true, false},
        {TEXT("TASK_Repair_HullFlooding"), TEXT("Flooding"), false, FVector(820, 180, 70), 0.35f, false, false, false},
        {TEXT("TASK_Sabotage_HullFlooding"), TEXT("Flooding"), true, FVector(820, -180, 70), 0.35f, false, true, false},
        {TEXT("TASK_Repair_Route"), TEXT("Route"), false, FVector(1140, 180, 440), 0.35f, false, false, false},
    };
    for (const FTaskSpec& Task : Tasks)
    {
        if (!SpawnShipTask(Task, Error))
        {
            return false;
        }
    }

    const TArray<FDoorSpec> Doors = {
        {TEXT("DOOR_RadioBulkhead"), TEXT("RadioBulkhead"), FVector(170, -380, 100), 0},
        {TEXT("DOOR_EngineBulkhead"), TEXT("EngineBulkhead"), FVector(-650, 0, 100), 90},
        {TEXT("DOOR_HoldFloodBulkhead"), TEXT("HoldFloodBulkhead"), FVector(520, 0, -80), 90},
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

    World->MarkPackageDirty();
    if (!UEditorLoadingAndSavingUtils::SaveMap(World, MapPackage))
    {
        return Fail(Error, FString::Printf(TEXT("Could not save map %s."), *MapPackage));
    }

    UE_LOG(LogAbyssWhiteboxCommandlet, Display, TEXT("Created and saved %s."), *MapPackage);
    return true;
}

FString ShipSystemName(EAbyssShipSystem System)
{
    switch (System)
    {
    case EAbyssShipSystem::Hull:
        return TEXT("hull");
    case EAbyssShipSystem::Fuel:
        return TEXT("fuel");
    case EAbyssShipSystem::Engine:
        return TEXT("engine");
    case EAbyssShipSystem::Power:
        return TEXT("power");
    case EAbyssShipSystem::Radio:
        return TEXT("radio");
    case EAbyssShipSystem::Route:
        return TEXT("route");
    case EAbyssShipSystem::Heat:
        return TEXT("heat");
    case EAbyssShipSystem::Flooding:
        return TEXT("flooding");
    default:
        return TEXT("unknown");
    }
}

FString TaskModeName(EAbyssShipTaskMode Mode)
{
    return Mode == EAbyssShipTaskMode::Sabotage ? TEXT("sabotage") : TEXT("repair");
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

    if (Values.FindRef(TEXT("EditorStartupMap")) != MapAsset)
    {
        return Fail(Error, FString::Printf(TEXT("EditorStartupMap is not %s: %s"), *MapAsset, *Values.FindRef(TEXT("EditorStartupMap"))));
    }
    if (Values.FindRef(TEXT("GameDefaultMap")) != MapAsset)
    {
        return Fail(Error, FString::Printf(TEXT("GameDefaultMap is not %s: %s"), *MapAsset, *Values.FindRef(TEXT("GameDefaultMap"))));
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
        TEXT("TASK_Repair_Route"),
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
        {TEXT("TASK_Repair_Route"), TEXT("route"), TEXT("repair")},
    };

    for (const FTaskExpectation& Expectation : Expectations)
    {
        AActor* const* Found = Actors.Find(Expectation.Label);
        AAbyssShipTaskActor* Task = Found ? Cast<AAbyssShipTaskActor>(*Found) : nullptr;
        if (!Task)
        {
            return Fail(Error, FString::Printf(TEXT("%s is not an AbyssShipTaskActor."), Expectation.Label));
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
        if (!Found || !Cast<AAbyssDoorActor>(*Found))
        {
            return Fail(Error, FString::Printf(TEXT("%s is not an AbyssDoorActor."), Label));
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

    UE_LOG(LogAbyssWhiteboxCommandlet, Display, TEXT("%s passed validation with %d actors."), *MapPackage, Actors.Num());
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
    UE_LOG(LogAbyssWhiteboxCommandlet, Error, TEXT("%s"), *Message);
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

    UE_LOG(LogAbyssWhiteboxCommandlet, Display, TEXT("Created and saved %s."), *MapPackage);
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

    UE_LOG(LogAbyssWhiteboxCommandlet, Display, TEXT("%s passed validation with %d actors."), *MapPackage, Labels.Num());
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

    UE_LOG(LogAbyssWhiteboxCommandlet, Display, TEXT("Imported %d ambientCG source files into %s."), ImportedCount, *AmbientCgDestinationRoot);
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

    UE_LOG(LogAbyssWhiteboxCommandlet, Display, TEXT("ambientCG visual POC assets passed validation."));
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
        UE_LOG(LogAbyssWhiteboxCommandlet, Error, TEXT("Could not load or create %s."), *MapPackage);
        return 1;
    }

    if (AWorldSettings* WorldSettings = World->GetWorldSettings())
    {
        WorldSettings->DefaultGameMode = AAbyssMenuGameMode::StaticClass();
    }

    if (!UEditorLoadingAndSavingUtils::SaveMap(World, MapPackage))
    {
        UE_LOG(LogAbyssWhiteboxCommandlet, Error, TEXT("Could not save %s."), *MapPackage);
        return 1;
    }

    UE_LOG(LogAbyssWhiteboxCommandlet, Display, TEXT("Created main menu map %s with menu GameMode."), *MapPackage);
    return 0;
}
