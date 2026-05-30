#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "FrostwakeAction.generated.h"

class UFrostwakeActionComponent;

/**
 * Base activatable ability (plan §3.2 Tier 2: interact / sabotage / use-item / role / thrall spells).
 * Server-authoritative; owned by a UFrostwakeActionComponent. Subclasses implement the behaviour;
 * the base provides id/tags and start/stop lifecycle so the component can grant and run them uniformly.
 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew, Abstract)
class FROSTWAKE_API UFrostwakeAction : public UObject
{
	GENERATED_BODY()

public:
	/** Stable id of this action. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Action")
	FName ActionId = NAME_None;

	/** Tags describing/gating the action (Ability.*). Assigned in data. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Action")
	FGameplayTagContainer ActionTags;

	/** Whether the action may start now (default: not already running). */
	virtual bool CanStart(UFrostwakeActionComponent* Component) const;

	/** Begin the action (server). Default marks it running. */
	virtual void Start(UFrostwakeActionComponent* Component);

	/** End the action (server). Default clears running. */
	virtual void Stop(UFrostwakeActionComponent* Component);

	UFUNCTION(BlueprintCallable, Category="Frostwake|Action")
	bool IsRunning() const { return bRunning; }

	//~ Route UObject world lookup through the outer component.
	virtual UWorld* GetWorld() const override;

protected:
	bool bRunning = false;
};
