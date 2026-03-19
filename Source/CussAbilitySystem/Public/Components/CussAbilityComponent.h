//Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "CoreTypes/CussAbilityTypes.h"
#include "Data/CussAbilityDebugTypes.h"
#include "CussAbilityComponent.generated.h"

class UCussAbilityData;
class UCussAbilitySetData;
class UCussStatComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCussAbilityEventDelegate, const FCussAbilityEvent&, EventData);
DECLARE_MULTICAST_DELEGATE_OneParam(FCussAbilityActivationResultViewDelegate, const FCussAbilityActivationResultView&);
DECLARE_MULTICAST_DELEGATE(FCussAbilityDebugRefreshDelegate);
DECLARE_MULTICAST_DELEGATE_OneParam(FCussCombatLogEntryDelegate, const FCussCombatLogEntry&);

/** Current vertical-slice ability component that grants abilities, validates activations, and applies effects. */
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
	void GrantAbility(UCussAbilityData* AbilityData, int32 AbilityLevel, FGameplayTag InputTag);

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

	/** Broadcasts explicit activation success and failure results for local debug presentation. */
	FCussAbilityActivationResultViewDelegate OnAbilityActivationResult;
	/** Broadcasts that granted slot, cooldown, or blocked hotbar state should be refreshed. */
	FCussAbilityDebugRefreshDelegate OnAbilitySlotsChanged;
	/** Broadcasts that the active-effect presentation snapshot should be refreshed. */
	FCussAbilityDebugRefreshDelegate OnActiveEffectsChanged;
	/** Broadcasts that the owned-tag presentation snapshot should be refreshed. */
	FCussAbilityDebugRefreshDelegate OnOwnedTagsChanged;
	/** Broadcasts a new lightweight combat log entry for local debug presentation. */
	FCussCombatLogEntryDelegate OnCombatLogEntryAdded;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cuss|Ability")
	FGameplayTagContainer StartupOwnedTags;

	UFUNCTION(BlueprintCallable, Category="Cuss|Ability")
	const FGameplayTagContainer& GetOwnedTags() const { return OwnedTags; }
	/** Copies a display-facing snapshot of the current granted hotbar slots and cooldown state. */
	void GetAbilitySlotViews(TArray<FCussAbilitySlotView>& OutViews) const;
	/** Copies a display-facing snapshot of the current active effects on this component owner. */
	void GetActiveEffectViews(TArray<FCussActiveEffectView>& OutViews) const;
	/** Copies the current owned gameplay tags for debug presentation code. */
	void GetOwnedTags(FGameplayTagContainer& OutTags) const;

	/** Routes an authoritative projectile impact back through the component's standard effect resolution path. */
	void HandleProjectileImpact(const FCussAbilityProjectileSpawnContext& ProjectileContext, AActor* ImpactActor, const FVector& ImpactLocation);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cuss|Ability")
	TArray<TObjectPtr<UCussAbilitySetData>> StartupAbilitySets;

	UPROPERTY(ReplicatedUsing=OnRep_GrantedAbilities)
	TArray<FCussGrantedAbilitySpec> GrantedAbilities;

	UPROPERTY(ReplicatedUsing=OnRep_OwnedTags)
	FGameplayTagContainer OwnedTags;

	UPROPERTY(ReplicatedUsing=OnRep_ActiveEffects)
	TArray<FCussActiveEffect> ActiveEffects;
	
	UPROPERTY(ReplicatedUsing=OnRep_CooldownGroupStates)
	TArray<FCussCooldownGroupState> CooldownGroupStates;

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
	/** Receives replicated granted ability changes for local hotbar refresh. */
	void OnRep_GrantedAbilities();

	UFUNCTION()
	/** Receives replicated owned tag changes. */
	void OnRep_OwnedTags();

	UFUNCTION()
	/** Receives replicated active effect changes for local debug refresh. */
	void OnRep_ActiveEffects();

	UFUNCTION()
	/** Receives replicated cooldown group changes for local hotbar refresh. */
	void OnRep_CooldownGroupStates();

	UFUNCTION(Client, Reliable)
	/** Forwards an explicit activation result to the owning client for local debug presentation. */
	void ClientReceiveActivationResult(const FCussAbilityActivationResultView& ActivationResult);

	UFUNCTION(Client, Reliable)
	/** Forwards a lightweight combat log entry to the owning client for local debug presentation. */
	void ClientReceiveCombatLogEntry(const FCussCombatLogEntry& CombatLogEntry);

	/** Looks up a granted ability spec by its ability tag. */
	const FCussGrantedAbilitySpec* FindGrantedAbilityByTag(FGameplayTag AbilityTag) const;
	/** Looks up a mutable granted ability spec by its ability tag. */
	FCussGrantedAbilitySpec* FindGrantedAbilityByTagMutable(FGameplayTag AbilityTag);
	/** Looks up a granted ability spec by the input tag bound to it. */
	const FCussGrantedAbilitySpec* FindGrantedAbilityByInputTag(FGameplayTag InputTag) const;

	/** Runs the full cooldown, cost, tag, and targeting checks for an activation attempt. */
	bool CanActivateAbility(const FCussGrantedAbilitySpec& Spec, AActor* OptionalTargetActor, const FVector& TargetLocation) const;
	/** Verifies the authored delivery configuration contains the minimum data needed to execute the ability. */
	bool CheckDelivery(UCussAbilityData* AbilityData) const;
	/** Verifies the owner can afford all stat costs required by the ability. */
	bool CheckCosts(UCussAbilityData* AbilityData) const;
	/** Verifies the ability's cooldown has expired. */
	bool CheckCooldown(const FCussGrantedAbilitySpec& Spec) const;
	/** Verifies the owner's tags satisfy the ability's required and blocked tag rules. */
	bool CheckOwnerTags(UCussAbilityData* AbilityData) const;
	/** Validates the current basic target mode requirements for the provided actor or location. */
	bool ValidateTargeting(UCussAbilityData* AbilityData, AActor* OptionalTargetActor, const FVector& TargetLocation) const;

	/** Commits a successful activation by broadcasting events, spending costs, and routing through the authored delivery path. */
	void ActivateAbilityInternal(FCussGrantedAbilitySpec& Spec, AActor* OptionalTargetActor, const FVector& TargetLocation);
	/** Spends the configured stat costs for an ability activation. */
	void CommitCosts(UCussAbilityData* AbilityData);
	/** Starts the cooldown timer for a granted ability spec. */
	void StartCooldown(FCussGrantedAbilitySpec& Spec);
	/** Spawns the configured projectile delivery actor on the server and seeds it with explicit runtime state. */
	void SpawnProjectileForAbility(const FCussGrantedAbilitySpec& Spec, AActor* OptionalTargetActor, const FVector& TargetLocation);
	/** Builds the explicit runtime payload copied onto a new projectile instance for inspection and impact resolution. */
	FCussAbilityProjectileSpawnContext BuildProjectileSpawnContext(const FCussGrantedAbilitySpec& Spec, const FVector& SpawnLocation, const FVector& LaunchDirection) const;
	/** Resolves the world-space aim point used to derive the current projectile's launch direction. */
	FVector ResolveProjectileAimLocation(const UCussAbilityData* AbilityData, AActor* OptionalTargetActor, const FVector& TargetLocation) const;
	/** Evaluates the current activation attempt and returns the most useful demo-facing failure reason. */
	ECussAbilityActivationFailReason DetermineActivationFailure(const FCussGrantedAbilitySpec& Spec, AActor* OptionalTargetActor, const FVector& TargetLocation, FGameplayTag& OutRelatedTag, float& OutCooldownRemaining) const;
	/** Builds a lightweight activation result view for validation messages and combat log output. */
	FCussAbilityActivationResultView BuildActivationResultView(const UCussAbilityData* AbilityData, FGameplayTag AbilityTag, bool bSuccess, ECussAbilityActivationFailReason FailReason, FGameplayTag RelatedTag, float CooldownRemaining, const FText& Message) const;

	/** Resolves targets and applies the supplied explicit effect snapshot using the provided ability level. */
	void ResolveAndApplyEffects(UCussAbilityData* AbilityData, const TArray<FCussAbilityEffectDef>& Effects, int32 AbilityLevel, AActor* OptionalTargetActor, const FVector& TargetLocation);
	/** Builds a resolved execution context for one target and applies one effect using the provided ability level. */
	void ApplyEffectToResolvedTarget(UCussAbilityData* AbilityData, const FCussAbilityEffectDef& EffectDef, int32 AbilityLevel, AActor* ResolvedTarget, const FVector& TargetLocation);
	
	/** Creates or updates an active effect on this component using the provided execution context and stacking rules. */
	void AddActiveEffectFromContext(const FCussEffectContext& EffectContext, const FCussAbilityEffectDef& EffectDef);

	/** Applies a single instant effect using a fully resolved execution context. */
	void ApplyInstantEffectFromContext(const FCussEffectContext& EffectContext, const FCussAbilityEffectDef& EffectDef);
	/** Executes a single periodic tick for an active effect already applied to this owner. */
	void ApplyPeriodicEffectTick(const FCussActiveEffect& Effect);
	/** Removes an active effect entry, clears timers, strips granted tags, and notes whether it expired. */
	void RemoveActiveEffectById(FGuid EffectId, bool bExpired = false);

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
	/** Broadcasts an explicit activation result locally and to the owning client when needed. */
	void EmitActivationResult(const FCussAbilityActivationResultView& ActivationResult);
	/** Broadcasts a lightweight combat log entry locally and to the owning client when needed. */
	void AddCombatLogEntry(ECussCombatLogEntryType EntryType, const FText& Message);
	/** Broadcasts that the hotbar-facing slot snapshot should be refreshed. */
	void NotifyAbilitySlotsChanged();
	/** Broadcasts that the active-effect snapshot should be refreshed. */
	void NotifyActiveEffectsChanged();
	/** Broadcasts that the owned-tag snapshot should be refreshed. */
	void NotifyOwnedTagsChanged();

	/** Finds the ability component on an arbitrary actor. */
	UCussAbilityComponent* GetAbilityComponent(const AActor* Actor) const;
	/** Finds the stat component on an arbitrary actor. */
	UCussStatComponent* GetStatComponent(const AActor* Actor) const;
	/** Resolves the primary actor target to use for the current targeting mode. */
	AActor* ResolvePrimaryTarget(const UCussAbilityData* AbilityData, AActor* OptionalTargetActor) const;
	
	bool IsTargetValidForAbility(const UCussAbilityData* AbilityData, const AActor* CandidateTarget) const;
	void ResolveTargetsForAbility(const UCussAbilityData* AbilityData, AActor* OptionalTargetActor, const FVector& TargetLocation, TArray<AActor*>& OutTargets) const;
	void GatherAreaTargets(const UCussAbilityData* AbilityData, const FVector& Origin, TArray<AActor*>& OutTargets) const;
	bool HasLineOfSightToActor(const AActor* OtherActor) const;

	FGameplayTag GetTeamTagForActor(const AActor* Actor) const;
	bool AreActorsAllies(const AActor* ActorA, const AActor* ActorB) const;
	bool AreActorsEnemies(const AActor* ActorA, const AActor* ActorB) const;

	void InitializeStartupOwnedTags();
	
	float GetCooldownGroupEndTime(FGameplayTag CooldownGroupTag) const;
	void SetCooldownGroupEndTime(FGameplayTag CooldownGroupTag, float EndTime);
	float GetRemainingCooldownForGroup(FGameplayTag CooldownGroupTag) const;

	/** Finds an existing active effect that matches the provided context and definition for stacking/refresh behavior. */
	FCussActiveEffect* FindMatchingActiveEffect(const FCussEffectContext& EffectContext, const FCussAbilityEffectDef& EffectDef);

	/** Refreshes an existing active effect's duration and resets its expiration timer. */
	void RefreshActiveEffectDuration(FCussActiveEffect& ActiveEffect);

	/** Resets the duration timer for an active effect. */
	void ResetDurationTimer(const FCussActiveEffect& ActiveEffect);

	/** Resets the periodic timer for an active effect. */
	void ResetPeriodTimer(const FCussActiveEffect& ActiveEffect);
};
