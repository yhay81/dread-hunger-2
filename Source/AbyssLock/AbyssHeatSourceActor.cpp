#include "AbyssHeatSourceActor.h"

#include "AbyssLockCharacter.h"
#include "AbyssLockLog.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AAbyssHeatSourceActor::AAbyssHeatSourceActor()
{
    WarmthPerUse = 50.0f;
    InteractionText = FText::FromString(TEXT("Warm Up"));

    // Visible marker (a small "brazier"); the base InteractionBounds sphere handles the trace.
    DisplayMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DisplayMesh"));
    DisplayMesh->SetupAttachment(RootComponent);
    DisplayMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (CylinderMesh.Succeeded())
    {
        DisplayMesh->SetStaticMesh(CylinderMesh.Object);
        DisplayMesh->SetRelativeScale3D(FVector(0.6f, 0.6f, 0.8f));
    }
}

bool AAbyssHeatSourceActor::Interact(APawn* InstigatorPawn)
{
    // Reached via the server-authoritative interaction path; guard anyway per the interactable idiom.
    if (!HasAuthority() || !CanInteract(InstigatorPawn))
    {
        return false;
    }

    AAbyssLockCharacter* Character = Cast<AAbyssLockCharacter>(InstigatorPawn);
    if (!Character)
    {
        return false;
    }

    const bool bSucceeded = Character->ApplyWarmth(WarmthPerUse, this);
    UE_LOG(LogAbyssGameplay, Log, TEXT("heat_source_used source=%s pawn=%s result=%s"), *GetNameSafe(this), *GetNameSafe(InstigatorPawn), bSucceeded ? TEXT("success") : TEXT("failed"));
    return bSucceeded;
}
