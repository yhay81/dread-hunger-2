#pragma once

#include "CoreMinimal.h"
#include "FrostwakeTypes.h"
#include "GameFramework/PlayerState.h"
#include "FrostwakePlayerState.generated.h"

UCLASS()
class FROSTWAKE_API AFrostwakePlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    AFrostwakePlayerState();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Player")
    EFrostwakeLifeState GetLifeState() const { return LifeState; }

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Player")
    EFrostwakeTeam GetRevealedTeam() const { return RevealedTeam; }

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Player")
    EFrostwakeTeam GetSecretTeamForOwner() const { return SecretTeam; }

    UFUNCTION(BlueprintCallable, Category = "Frostwake|Lobby")
    bool IsReadyForMatch() const { return bReadyForMatch; }

    void SetSecretTeam(EFrostwakeTeam NewTeam);
    void SetRevealedTeam(EFrostwakeTeam NewTeam);
    void SetLifeState(EFrostwakeLifeState NewLifeState);
    void SetReadyForMatch(bool bNewReady);

protected:
    UPROPERTY(ReplicatedUsing = OnRep_LifeState, BlueprintReadOnly, Category = "Frostwake|Player")
    EFrostwakeLifeState LifeState;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Frostwake|Player")
    EFrostwakeTeam RevealedTeam;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Frostwake|Player")
    EFrostwakeTeam SecretTeam;

    UPROPERTY(ReplicatedUsing = OnRep_ReadyForMatch, BlueprintReadOnly, Category = "Frostwake|Lobby")
    bool bReadyForMatch;

    UFUNCTION()
    void OnRep_LifeState();

    UFUNCTION()
    void OnRep_ReadyForMatch();
};
