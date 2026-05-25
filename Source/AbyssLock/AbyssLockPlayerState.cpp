#include "AbyssLockPlayerState.h"
#include "Net/UnrealNetwork.h"

AAbyssLockPlayerState::AAbyssLockPlayerState()
    : LifeState(EAbyssLifeState::Alive)
    , RevealedTeam(EAbyssTeam::Unassigned)
    , SecretTeam(EAbyssTeam::Unassigned)
    , bReadyForMatch(false)
{
}

void AAbyssLockPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AAbyssLockPlayerState, LifeState);
    DOREPLIFETIME(AAbyssLockPlayerState, RevealedTeam);
    DOREPLIFETIME_CONDITION(AAbyssLockPlayerState, SecretTeam, COND_OwnerOnly);
    DOREPLIFETIME(AAbyssLockPlayerState, bReadyForMatch);
}

void AAbyssLockPlayerState::SetSecretTeam(EAbyssTeam NewTeam)
{
    if (HasAuthority())
    {
        SecretTeam = NewTeam;
    }
}

void AAbyssLockPlayerState::SetRevealedTeam(EAbyssTeam NewTeam)
{
    if (HasAuthority())
    {
        RevealedTeam = NewTeam;
    }
}

void AAbyssLockPlayerState::SetLifeState(EAbyssLifeState NewLifeState)
{
    if (HasAuthority())
    {
        LifeState = NewLifeState;
    }
}

void AAbyssLockPlayerState::SetReadyForMatch(bool bNewReady)
{
    if (HasAuthority())
    {
        bReadyForMatch = bNewReady;
    }
}

void AAbyssLockPlayerState::OnRep_LifeState()
{
}

void AAbyssLockPlayerState::OnRep_ReadyForMatch()
{
}
