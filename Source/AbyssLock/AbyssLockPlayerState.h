#pragma once

#include "CoreMinimal.h"
#include "AbyssLockTypes.h"
#include "GameFramework/PlayerState.h"
#include "AbyssLockPlayerState.generated.h"

UCLASS()
class ABYSSLOCK_API AAbyssLockPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    AAbyssLockPlayerState();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "Abyss|Player")
    EAbyssLifeState GetLifeState() const { return LifeState; }

    UFUNCTION(BlueprintCallable, Category = "Abyss|Player")
    EAbyssTeam GetRevealedTeam() const { return RevealedTeam; }

    UFUNCTION(BlueprintCallable, Category = "Abyss|Player")
    EAbyssTeam GetSecretTeamForOwner() const { return SecretTeam; }

    UFUNCTION(BlueprintCallable, Category = "Abyss|Lobby")
    bool IsReadyForMatch() const { return bReadyForMatch; }

    void SetSecretTeam(EAbyssTeam NewTeam);
    void SetRevealedTeam(EAbyssTeam NewTeam);
    void SetLifeState(EAbyssLifeState NewLifeState);
    void SetReadyForMatch(bool bNewReady);

protected:
    UPROPERTY(ReplicatedUsing = OnRep_LifeState, BlueprintReadOnly, Category = "Abyss|Player")
    EAbyssLifeState LifeState;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Abyss|Player")
    EAbyssTeam RevealedTeam;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Abyss|Player")
    EAbyssTeam SecretTeam;

    UPROPERTY(ReplicatedUsing = OnRep_ReadyForMatch, BlueprintReadOnly, Category = "Abyss|Lobby")
    bool bReadyForMatch;

    UFUNCTION()
    void OnRep_LifeState();

    UFUNCTION()
    void OnRep_ReadyForMatch();
};
