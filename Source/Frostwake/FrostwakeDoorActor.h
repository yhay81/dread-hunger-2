#pragma once

#include "CoreMinimal.h"
#include "FrostwakeInteractableActor.h"
#include "FrostwakeDoorActor.generated.h"

UCLASS(Blueprintable)
class FROSTWAKE_API AFrostwakeDoorActor : public AFrostwakeInteractableActor
{
    GENERATED_BODY()

public:
    AFrostwakeDoorActor();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual bool CanInteract(const APawn* InstigatorPawn) const override;
    virtual bool Interact(APawn* InstigatorPawn) override;

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Door")
    FName GetDoorId() const { return DoorId; }

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Door")
    bool IsOpen() const { return bIsOpen; }

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Door")
    bool IsLocked() const { return bIsLocked; }

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Door")
    void SetLocked(bool bNewLocked);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Frostwake|Door")
    bool LockForSabotage(APawn* InstigatorPawn);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Frostwake|Door")
    bool UnlockFromRepair(APawn* InstigatorPawn);

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Door")
    void ConfigureDoorByName(FName InDoorId);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Frostwake|Door")
    TObjectPtr<class UStaticMeshComponent> DoorPanelMesh;

    UPROPERTY(ReplicatedUsing = OnRep_DoorState, EditAnywhere, BlueprintReadOnly, Category = "Frostwake|Door")
    FName DoorId;

    UPROPERTY(ReplicatedUsing = OnRep_DoorState, BlueprintReadOnly, Category = "Frostwake|Door")
    bool bIsOpen;

    UPROPERTY(ReplicatedUsing = OnRep_DoorState, BlueprintReadOnly, Category = "Frostwake|Door")
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
