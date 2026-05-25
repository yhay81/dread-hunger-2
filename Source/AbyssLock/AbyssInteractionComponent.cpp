#include "AbyssInteractionComponent.h"
#include "AbyssInteractableActor.h"
#include "AbyssLockLog.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

UAbyssInteractionComponent::UAbyssInteractionComponent()
{
    SetIsReplicatedByDefault(true);
    InteractionDistance = 250.0f;
    InteractionRadius = 30.0f;
}

void UAbyssInteractionComponent::TryInteract()
{
    AAbyssInteractableActor* Target = FindBestInteractable();
    if (!Target)
    {
        return;
    }

    ServerTryInteract(Target);
}

AAbyssInteractableActor* UAbyssInteractionComponent::FindBestInteractable() const
{
    const APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn)
    {
        return nullptr;
    }

    FVector ViewLocation;
    FRotator ViewRotation;
    if (const AController* Controller = OwnerPawn->GetController())
    {
        Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
    }
    else
    {
        ViewLocation = OwnerPawn->GetActorLocation();
        ViewRotation = OwnerPawn->GetActorRotation();
    }

    const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * InteractionDistance;

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AbyssInteractionTrace), false, OwnerPawn);
    FCollisionObjectQueryParams ObjectQueryParams;
    ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);

    TArray<FHitResult> HitResults;
    const bool bHit = GetWorld()->SweepMultiByObjectType(
        HitResults,
        ViewLocation,
        TraceEnd,
        FQuat::Identity,
        ObjectQueryParams,
        FCollisionShape::MakeSphere(InteractionRadius),
        QueryParams);

    if (!bHit)
    {
        return nullptr;
    }

    HitResults.Sort([&ViewLocation](const FHitResult& Left, const FHitResult& Right) {
        return FVector::DistSquared(ViewLocation, Left.ImpactPoint) < FVector::DistSquared(ViewLocation, Right.ImpactPoint);
    });

    for (const FHitResult& HitResult : HitResults)
    {
        AAbyssInteractableActor* Candidate = Cast<AAbyssInteractableActor>(HitResult.GetActor());
        if (Candidate && Candidate->CanInteract(OwnerPawn))
        {
            return Candidate;
        }
    }

    return nullptr;
}

void UAbyssInteractionComponent::ServerTryInteract_Implementation(AAbyssInteractableActor* Target)
{
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn || !Target)
    {
        return;
    }

    if (!IsTargetInRange(Target) || FindBestInteractable() != Target || !Target->CanInteract(OwnerPawn))
    {
        UE_LOG(LogAbyssGameplay, Verbose, TEXT("Rejected interaction. Pawn=%s Target=%s"), *GetNameSafe(OwnerPawn), *GetNameSafe(Target));
        return;
    }

    const bool bSucceeded = Target->Interact(OwnerPawn);
    UE_LOG(LogAbyssGameplay, Log, TEXT("interaction_request pawn=%s target=%s result=%s"), *GetNameSafe(OwnerPawn), *GetNameSafe(Target), bSucceeded ? TEXT("success") : TEXT("failed"));
}

bool UAbyssInteractionComponent::IsTargetInRange(const AAbyssInteractableActor* Target) const
{
    const AActor* OwnerActor = GetOwner();
    if (!OwnerActor || !Target)
    {
        return false;
    }

    const float MaxDistance = InteractionDistance + InteractionRadius + 25.0f;
    return FVector::DistSquared(OwnerActor->GetActorLocation(), Target->GetActorLocation()) <= FMath::Square(MaxDistance);
}
