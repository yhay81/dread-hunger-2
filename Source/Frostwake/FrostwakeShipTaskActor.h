#pragma once

#include "CoreMinimal.h"
#include "FrostwakeInteractableActor.h"
#include "FrostwakeTypes.h"
#include "FrostwakeShipTaskActor.generated.h"

UCLASS(Blueprintable)
class FROSTWAKE_API AFrostwakeShipTaskActor : public AFrostwakeInteractableActor
{
    GENERATED_BODY()

public:
    AFrostwakeShipTaskActor();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual bool CanInteract(const APawn* InstigatorPawn) const override;
    virtual bool Interact(APawn* InstigatorPawn) override;

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Ship Task")
    bool IsCompleted() const { return bCompleted; }

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Ship Task")
    EFrostwakeShipSystem GetTargetSystem() const { return TargetSystem; }

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Ship Task")
    EFrostwakeShipTaskMode GetTaskMode() const { return TaskMode; }

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Ship Task")
    bool ConfigureTaskByName(FName InTargetSystem, bool bInSabotage, float InConditionDelta, bool bInSingleUse, bool bInSaboteurOnly, bool bInSetOfflineOnSabotage);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Frostwake|Ship Task")
    TObjectPtr<class UStaticMeshComponent> MarkerMesh;

    UPROPERTY(ReplicatedUsing = OnRep_TaskConfig, EditAnywhere, BlueprintReadOnly, Category = "Frostwake|Ship Task")
    EFrostwakeShipSystem TargetSystem;

    UPROPERTY(ReplicatedUsing = OnRep_TaskConfig, EditAnywhere, BlueprintReadOnly, Category = "Frostwake|Ship Task")
    EFrostwakeShipTaskMode TaskMode;

    UPROPERTY(ReplicatedUsing = OnRep_TaskConfig, EditAnywhere, BlueprintReadOnly, Category = "Frostwake|Ship Task", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ConditionDelta;

    UPROPERTY(ReplicatedUsing = OnRep_TaskConfig, EditAnywhere, BlueprintReadOnly, Category = "Frostwake|Ship Task")
    bool bSingleUse;

    UPROPERTY(ReplicatedUsing = OnRep_TaskConfig, EditAnywhere, BlueprintReadOnly, Category = "Frostwake|Ship Task")
    bool bSaboteurOnly;

    UPROPERTY(ReplicatedUsing = OnRep_TaskConfig, EditAnywhere, BlueprintReadOnly, Category = "Frostwake|Ship Task")
    bool bSetOfflineOnSabotage;

    UPROPERTY(ReplicatedUsing = OnRep_TaskState, BlueprintReadOnly, Category = "Frostwake|Ship Task")
    bool bCompleted;

    UFUNCTION()
    void OnRep_TaskState();

    UFUNCTION()
    void OnRep_TaskConfig();

private:
    bool IsInstigatorAllowed(const APawn* InstigatorPawn) const;
    void ApplyCompletedState();
    void ApplyTaskConfigState();
    static bool ResolveShipSystem(FName SystemName, EFrostwakeShipSystem& OutSystem);
    // Difficulty multiplier applied to sabotage severity (1.0 if no match config / not authoritative).
    float GetMatchSabotageIntensity() const;
};
