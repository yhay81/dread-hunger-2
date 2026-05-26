#include "AbyssWhiteboxCommandlets.h"

#include "AbyssDoorActor.h"
#include "AbyssShipTaskActor.h"
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
#include "GameFramework/PlayerStart.h"
#include "LevelEditorSubsystem.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Subsystems/EditorActorSubsystem.h"

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

    if (!SpawnActor(ADirectionalLight::StaticClass(), TEXT("L_Key_ArcticSun"), FVector(-900, -1200, 2200), FRotator(-38, -35, 0), Error))
    {
        return false;
    }
    if (!SpawnActor(ASkyLight::StaticClass(), TEXT("L_Sky_Overcast"), FVector(0, 0, 1200), FRotator::ZeroRotator, Error))
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
