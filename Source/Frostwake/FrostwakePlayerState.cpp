#include "FrostwakePlayerState.h"
#include "Net/UnrealNetwork.h"

AFrostwakePlayerState::AFrostwakePlayerState()
    : LifeState(EFrostwakeLifeState::Alive)
    , RevealedTeam(EFrostwakeTeam::Unassigned)
    , SecretTeam(EFrostwakeTeam::Unassigned)
    , bReadyForMatch(false)
{
}

void AFrostwakePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AFrostwakePlayerState, LifeState);
    DOREPLIFETIME(AFrostwakePlayerState, RevealedTeam);
    DOREPLIFETIME_CONDITION(AFrostwakePlayerState, SecretTeam, COND_OwnerOnly);
    DOREPLIFETIME(AFrostwakePlayerState, bReadyForMatch);
}

void AFrostwakePlayerState::SetSecretTeam(EFrostwakeTeam NewTeam)
{
    if (HasAuthority())
    {
        SecretTeam = NewTeam;
    }
}

void AFrostwakePlayerState::SetRevealedTeam(EFrostwakeTeam NewTeam)
{
    if (HasAuthority())
    {
        RevealedTeam = NewTeam;
    }
}

void AFrostwakePlayerState::SetLifeState(EFrostwakeLifeState NewLifeState)
{
    if (HasAuthority())
    {
        LifeState = NewLifeState;
    }
}

void AFrostwakePlayerState::SetReadyForMatch(bool bNewReady)
{
    if (HasAuthority())
    {
        bReadyForMatch = bNewReady;
    }
}

void AFrostwakePlayerState::OnRep_LifeState()
{
}

void AFrostwakePlayerState::OnRep_ReadyForMatch()
{
}
