#include "AbyssInteractableActor.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"

AAbyssInteractableActor::AAbyssInteractableActor()
{
    bReplicates = true;
    SetReplicateMovement(false);

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    InteractionBounds = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionBounds"));
    InteractionBounds->SetupAttachment(SceneRoot);
    InteractionBounds->SetSphereRadius(90.0f);
    InteractionBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionBounds->SetCollisionObjectType(ECC_WorldDynamic);
    InteractionBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionBounds->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    InteractionText = FText::FromString(TEXT("Interact"));
}

bool AAbyssInteractableActor::CanInteract(const APawn* InstigatorPawn) const
{
    return IsValid(InstigatorPawn);
}

bool AAbyssInteractableActor::Interact(APawn* InstigatorPawn)
{
    return CanInteract(InstigatorPawn);
}
