#pragma once

#include "CoreMinimal.h"
#include "AbyssInteractableActor.h"
#include "AbyssLockTypes.h"
#include "AbyssShipTaskActor.generated.h"

UCLASS(Blueprintable)
class ABYSSLOCK_API AAbyssShipTaskActor : public AAbyssInteractableActor
{
    GENERATED_BODY()

public:
    AAbyssShipTaskActor();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual bool CanInteract(const APawn* InstigatorPawn) const override;
    virtual bool Interact(APawn* InstigatorPawn) override;

    UFUNCTION(BlueprintCallable, Category = "Abyss|Ship Task")
    bool IsCompleted() const { return bCompleted; }

    UFUNCTION(BlueprintCallable, Category = "Abyss|Ship Task")
    EAbyssShipSystem GetTargetSystem() const { return TargetSystem; }

    UFUNCTION(BlueprintCallable, Category = "Abyss|Ship Task")
    EAbyssShipTaskMode GetTaskMode() const { return TaskMode; }

    UFUNCTION(BlueprintCallable, Category = "Abyss|Ship Task")
    bool ConfigureTaskByName(FName InTargetSystem, bool bInSabotage, float InConditionDelta, bool bInSingleUse, bool bInSaboteurOnly, bool bInSetOfflineOnSabotage);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abyss|Ship Task")
    TObjectPtr<class UStaticMeshComponent> MarkerMesh;

    UPROPERTY(ReplicatedUsing = OnRep_TaskConfig, EditAnywhere, BlueprintReadOnly, Category = "Abyss|Ship Task")
    EAbyssShipSystem TargetSystem;

    UPROPERTY(ReplicatedUsing = OnRep_TaskConfig, EditAnywhere, BlueprintReadOnly, Category = "Abyss|Ship Task")
    EAbyssShipTaskMode TaskMode;

    UPROPERTY(ReplicatedUsing = OnRep_TaskConfig, EditAnywhere, BlueprintReadOnly, Category = "Abyss|Ship Task", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ConditionDelta;

    UPROPERTY(ReplicatedUsing = OnRep_TaskConfig, EditAnywhere, BlueprintReadOnly, Category = "Abyss|Ship Task")
    bool bSingleUse;

    UPROPERTY(ReplicatedUsing = OnRep_TaskConfig, EditAnywhere, BlueprintReadOnly, Category = "Abyss|Ship Task")
    bool bSaboteurOnly;

    UPROPERTY(ReplicatedUsing = OnRep_TaskConfig, EditAnywhere, BlueprintReadOnly, Category = "Abyss|Ship Task")
    bool bSetOfflineOnSabotage;

    UPROPERTY(ReplicatedUsing = OnRep_TaskState, BlueprintReadOnly, Category = "Abyss|Ship Task")
    bool bCompleted;

    UFUNCTION()
    void OnRep_TaskState();

    UFUNCTION()
    void OnRep_TaskConfig();

private:
    bool IsInstigatorAllowed(const APawn* InstigatorPawn) const;
    void ApplyCompletedState();
    void ApplyTaskConfigState();
    static bool ResolveShipSystem(FName SystemName, EAbyssShipSystem& OutSystem);
};
