#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FrostwakeAttributeComponent.generated.h"

/**
 * The survival attributes (plan §3.15 / Action System Tier 2). Stable boundary enum used as the
 * attribute key (tags become the axis later; an enum keeps the foundation independent of the
 * not-yet-authored tag taxonomy). Order is append-only to keep replicated/save meaning stable.
 */
UENUM(BlueprintType)
enum class EFrostwakeAttribute : uint8
{
	Health         UMETA(DisplayName = "Health"),
	ReservedHealth UMETA(DisplayName = "Reserved Health"),
	Hunger         UMETA(DisplayName = "Hunger"),
	Warmth         UMETA(DisplayName = "Warmth"),
	Stamina        UMETA(DisplayName = "Stamina"),
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FFrostwakeAttributeChangedSignature,
	UFrostwakeAttributeComponent*, Component, EFrostwakeAttribute, Attribute, float, NewValue, float, OldValue);

/**
 * Server-authoritative, push-model-replicated survival attributes (plan §3.2 Tier 2 Action System,
 * §3.4 push model). Current values replicate; maxes are data-config (same on both sides). All
 * mutation is server-only via ModifyAttribute / SetAttribute; clients observe via OnRep and the
 * OnAttributeChanged delegate. This replaces the hand-written replicated floats on the character
 * (§3.1) — the migration of AFrostwakeCharacter is a separate Phase 2 step (shared file, not touched here).
 */
UCLASS(ClassGroup=(Frostwake), meta=(BlueprintSpawnableComponent))
class FROSTWAKE_API UFrostwakeAttributeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFrostwakeAttributeComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Fires on server and clients whenever an attribute's value changes. */
	UPROPERTY(BlueprintAssignable, Category="Frostwake|Attributes")
	FFrostwakeAttributeChangedSignature OnAttributeChanged;

	/** Server-authoritative: add Delta (clamped to [0, max]). Returns the resulting value. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Frostwake|Attributes")
	float ModifyAttribute(EFrostwakeAttribute Attribute, float Delta);

	/** Server-authoritative: set an absolute value (clamped to [0, max]). */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Frostwake|Attributes")
	void SetAttribute(EFrostwakeAttribute Attribute, float NewValue);

	UFUNCTION(BlueprintCallable, Category="Frostwake|Attributes")
	float GetAttribute(EFrostwakeAttribute Attribute) const;

	UFUNCTION(BlueprintCallable, Category="Frostwake|Attributes")
	float GetMaxAttribute(EFrostwakeAttribute Attribute) const;

protected:
	UPROPERTY(ReplicatedUsing=OnRep_Health, EditAnywhere, BlueprintReadOnly, Category="Frostwake|Attributes")
	float Health;

	UPROPERTY(ReplicatedUsing=OnRep_ReservedHealth, EditAnywhere, BlueprintReadOnly, Category="Frostwake|Attributes")
	float ReservedHealth;

	UPROPERTY(ReplicatedUsing=OnRep_Hunger, EditAnywhere, BlueprintReadOnly, Category="Frostwake|Attributes")
	float Hunger;

	UPROPERTY(ReplicatedUsing=OnRep_Warmth, EditAnywhere, BlueprintReadOnly, Category="Frostwake|Attributes")
	float Warmth;

	UPROPERTY(ReplicatedUsing=OnRep_Stamina, EditAnywhere, BlueprintReadOnly, Category="Frostwake|Attributes")
	float Stamina;

	// Maxes are data-config (set from a role/definition; not replicated — identical on server & client).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Attributes")
	float MaxHealth = 100.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Attributes")
	float MaxReservedHealth = 100.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Attributes")
	float MaxHunger = 100.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Attributes")
	float MaxWarmth = 100.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Frostwake|Attributes")
	float MaxStamina = 100.f;

	UFUNCTION() void OnRep_Health(float OldValue);
	UFUNCTION() void OnRep_ReservedHealth(float OldValue);
	UFUNCTION() void OnRep_Hunger(float OldValue);
	UFUNCTION() void OnRep_Warmth(float OldValue);
	UFUNCTION() void OnRep_Stamina(float OldValue);

private:
	/** Address of the backing field for an attribute (single source of the switch). */
	float* AttributePtr(EFrostwakeAttribute Attribute);
	const float* AttributePtr(EFrostwakeAttribute Attribute) const;
	float MaxFor(EFrostwakeAttribute Attribute) const;
	/** Marks the matching replicated property dirty for push-model networking. */
	void MarkAttributeDirty(EFrostwakeAttribute Attribute);
};
