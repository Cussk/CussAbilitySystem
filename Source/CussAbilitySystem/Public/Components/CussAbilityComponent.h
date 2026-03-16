//Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "CoreTypes/CussAbilityTypes.h"
#include "CussAbilityComponent.generated.h"

class UCussAbilityData;
class UCussAbilitySetData;
class UCussStatComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCussAbilityEventDelegate, const FCussAbilityEvent&, EventData);

/** Replicated ability component that grants abilities, validates activations, and applies effects. */
UCLASS(ClassGroup=(Cuss), meta=(BlueprintSpawnableComponent))
class CUSSABILITYSYSTEM_API UCussAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Enables replication and disables ticking for this data-driven ability component. */
	UCussAbilityComponent();

	/** Caches the stat component and grants configured startup ability sets on the server. */
	virtual void BeginPlay() override;
	/** Clears any effect timers that were created for active duration and periodic effects. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	/** Registers the replicated ability, tag, and active effect state for this component. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category="Cuss|Ability")
	/** Grants a single ability spec if the owner has authority and does not already have it. */
	void GrantAbility(const UCussAbilityData* AbilityData, int32 AbilityLevel, FGameplayTag InputTag);

	UFUNCTION(BlueprintCallable, Category="Cuss|Ability")
	/** Grants every valid entry from an ability set asset to the owning actor. */
	void GrantAbilitySet(const UCussAbilitySetData* AbilitySet);

	UFUNCTION(BlueprintCallable, Category="Cuss|Ability")
	/** Grants all startup ability sets configured on this component. */
	void GrantStartupAbilitySets();

	UFUNCTION(BlueprintCallable, Category="Cuss|Ability")
	/** Attempts to activate a granted ability by its ability tag. */
	bool TryActivateAbilityByTag(FGameplayTag AbilityTag, AActor* OptionalTargetActor, FVector TargetLocation);

	UFUNCTION(BlueprintCallable, Category="Cuss|Ability")
	/** Attempts to activate the ability currently mapped to the supplied input tag. */
	bool TryActivateAbilityByInputTag(FGameplayTag InputTag, AActor* OptionalTargetActor, FVector TargetLocation);

	UFUNCTION(BlueprintCallable, Category="Cuss|Ability")
	/** Returns whether the supplied ability is still waiting on its cooldown to expire. */
	bool IsAbilityOnCooldown(FGameplayTag AbilityTag) const;

	UFUNCTION(BlueprintCallable, Category="Cuss|Ability")
	float GetRemainingCooldown(FGameplayTag AbilityTag) const;

	UFUNCTION(BlueprintCallable, Category="Cuss|Ability")
	/** Returns whether the owner currently has the supplied gameplay tag. */
	bool HasMatchingOwnedTag(FGameplayTag Tag) const;

	UFUNCTION(BlueprintCallable, Category="Cuss|Ability")
	/** Returns whether the owner currently has any tag from the supplied container. */
	bool HasAnyMatchingOwnedTags(const FGameplayTagContainer& Tags) const;

	UPROPERTY(BlueprintAssignable, Category="Cuss|Ability")
	FCussAbilityEventDelegate OnAbilityEvent;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cuss|Ability")
	TArray<TObjectPtr<const UCussAbilitySetData>> StartupAbilitySets;

	UPROPERTY(Replicated)
	TArray<FCussGrantedAbilitySpec> GrantedAbilities;

	UPROPERTY(ReplicatedUsing=OnRep_OwnedTags)
	FGameplayTagContainer OwnedTags;

	UPROPERTY(Replicated)
	TArray<FCussActiveEffect> ActiveEffects;

	UPROPERTY()
	TObjectPtr<UCussStatComponent> StatComponent;

	TMap<FGuid, FTimerHandle> EffectDurationTimers;
	TMap<FGuid, FTimerHandle> EffectPeriodTimers;

	UFUNCTION(Server, Reliable)
	/** Forwards client activation requests to the server for authority-side execution. */
	void ServerTryActivateAbility(FGameplayTag AbilityTag, AActor* OptionalTargetActor, FVector_NetQuantize TargetLocation);

	UFUNCTION(NetMulticast, Unreliable)
	/** Relays ability events to remote clients listening for cosmetic or UI updates. */
	void MulticastAbilityEvent(const FCussAbilityEvent& EventData);

	UFUNCTION()
	/** Receives replicated owned tag changes. */
	void OnRep_OwnedTags();

	/** Looks up a granted ability spec by its ability tag. */
	const FCussGrantedAbilitySpec* FindGrantedAbilityByTag(FGameplayTag AbilityTag) const;
	/** Looks up a mutable granted ability spec by its ability tag. */
	FCussGrantedAbilitySpec* FindGrantedAbilityByTagMutable(FGameplayTag AbilityTag);
	/** Looks up a granted ability spec by the input tag bound to it. */
	const FCussGrantedAbilitySpec* FindGrantedAbilityByInputTag(FGameplayTag InputTag) const;

	/** Runs the full cooldown, cost, tag, and targeting checks for an activation attempt. */
	bool CanActivateAbility(const FCussGrantedAbilitySpec& Spec, AActor* OptionalTargetActor, const FVector& TargetLocation) const;
	/** Verifies the owner can afford all stat costs required by the ability. */
	bool CheckCosts(const UCussAbilityData* AbilityData) const;
	/** Verifies the ability's cooldown has expired. */
	bool CheckCooldown(const FCussGrantedAbilitySpec& Spec) const;
	/** Verifies the owner's tags satisfy the ability's required and blocked tag rules. */
	bool CheckOwnerTags(const UCussAbilityData* AbilityData) const;
	/** Validates that the provided actor or location matches the ability's targeting rules. */
	bool ValidateTargeting(const UCussAbilityData* AbilityData, AActor* OptionalTargetActor, const FVector& TargetLocation) const;

	/** Commits a successful activation by broadcasting events, spending costs, and applying effects. */
	void ActivateAbilityInternal(FCussGrantedAbilitySpec& Spec, AActor* OptionalTargetActor, const FVector& TargetLocation);
	/** Spends the configured stat costs for an ability activation. */
	void CommitCosts(const UCussAbilityData* AbilityData);
	/** Starts the cooldown timer for a granted ability spec. */
	void StartCooldown(FCussGrantedAbilitySpec& Spec);

	/** Resolves the primary target and applies every effect defined by the ability asset. */
	void ResolveAndApplyEffects(const UCussAbilityData* AbilityData, AActor* OptionalTargetActor, const FVector& TargetLocation);
	/** Applies one effect definition to the resolved target as instant or duration-based gameplay. */
	void ApplyEffectToResolvedTarget(const UCussAbilityData* AbilityData, const FCussAbilityEffectDef& EffectDef, AActor* ResolvedTarget, const FVector& TargetLocation);

	/** Applies an immediate effect instance from a source actor to a resolved target actor. */
	void ApplyInstantEffectFromSource(AActor* SourceActor, const UCussAbilityData* AbilityData, const FCussAbilityEffectDef& EffectDef, AActor* TargetActor, const FVector& EventLocation);
	/** Creates a replicated active effect entry and starts its timers on the target component. */
	void AddActiveEffectFromSource(AActor* SourceActor, const UCussAbilityData* AbilityData, const FCussAbilityEffectDef& EffectDef);

	/** Executes a single periodic tick for an active effect already applied to this owner. */
	void ApplyPeriodicEffectTick(const FCussActiveEffect& Effect);
	/** Removes an active effect entry, clears timers, and strips granted tags. */
	void RemoveActiveEffectById(FGuid EffectId);

	/** Adds a gameplay tag to the component's owned tag container. */
	void AddOwnedTag(FGameplayTag Tag);
	/** Removes a gameplay tag from the component's owned tag container. */
	void RemoveOwnedTag(FGameplayTag Tag);

	/** Handles a periodic timer firing for the specified active effect. */
	void HandleEffectPeriod(FGuid EffectId);
	/** Handles a duration timer expiring for the specified active effect. */
	void HandleEffectExpired(FGuid EffectId);

	/** Broadcasts a normalized ability event locally and across the network. */
	void BroadcastEvent(AActor* InstigatorActor, AActor* TargetActor, FGameplayTag AbilityTag, ECussAbilityEventResult Result, float Magnitude, const FVector& EventLocation, FGameplayTag CueTag, const FGameplayTagContainer& AppliedTags);

	/** Finds the ability component on an arbitrary actor. */
	UCussAbilityComponent* GetAbilityComponent(AActor* Actor) const;
	/** Finds the stat component on an arbitrary actor. */
	UCussStatComponent* GetStatComponent(AActor* Actor) const;
	/** Resolves the primary actor target to use for the current targeting mode. */
	AActor* ResolvePrimaryTarget(const UCussAbilityData* AbilityData, AActor* OptionalTargetActor) const;
};
