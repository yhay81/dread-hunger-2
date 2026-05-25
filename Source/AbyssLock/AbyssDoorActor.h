#pragma once

#include "CoreMinimal.h"
#include "AbyssInteractableActor.h"
#include "AbyssDoorActor.generated.h"

UCLASS(Blueprintable)
class ABYSSLOCK_API AAbyssDoorActor : public AAbyssInteractableActor
{
    GENERATED_BODY()

public:
    AAbyssDoorActor();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual bool CanInteract(const APawn* InstigatorPawn) const override;
    virtual bool Interact(APawn* InstigatorPawn) override;

    UFUNCTION(BlueprintCallable, Category = "Abyss|Door")
    FName GetDoorId() const { return DoorId; }

    UFUNCTION(BlueprintCallable, Category = "Abyss|Door")
    bool IsOpen() const { return bIsOpen; }

    UFUNCTION(BlueprintCallable, Category = "Abyss|Door")
    bool IsLocked() const { return bIsLocked; }

    UFUNCTION(BlueprintCallable, Category = "Abyss|Door")
    void SetLocked(bool bNewLocked);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abyss|Door")
    bool LockForSabotage(APawn* InstigatorPawn);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abyss|Door")
    bool UnlockFromRepair(APawn* InstigatorPawn);

    UFUNCTION(BlueprintCallable, Category = "Abyss|Door")
    void ConfigureDoorByName(FName InDoorId);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abyss|Door")
    TObjectPtr<class UStaticMeshComponent> DoorPanelMesh;

    UPROPERTY(ReplicatedUsing = OnRep_DoorState, EditAnywhere, BlueprintReadOnly, Category = "Abyss|Door")
    FName DoorId;

    UPROPERTY(ReplicatedUsing = OnRep_DoorState, BlueprintReadOnly, Category = "Abyss|Door")
    bool bIsOpen;

    UPROPERTY(ReplicatedUsing = OnRep_DoorState, BlueprintReadOnly, Category = "Abyss|Door")
    bool bIsLocked;

    UFUNCTION()
    void OnRep_DoorState();

private:
    void ApplyDoorState();
    bool IsInstigatorAliveAndAssigned(const APawn* InstigatorPawn) const;
    bool IsInstigatorSaboteur(const APawn* InstigatorPawn) const;
    void EmitDoorLockChangedEvent(const APawn* InstigatorPawn, bool bNewLocked, const TCHAR* Reason) const;
    void EmitDoorToggledEvent(const APawn* InstigatorPawn) const;
};
