// Copyright Kyle Cuss and Cuss Programming 2026

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "CussAbilitySystem/Public/CoreTypes/CussAbilityTypes.h"
#include "CussAbilityComponent.generated.h"

enum class ECussAbilityEventResult : uint8;
class UCussAbilityData;
class UCussStatComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCussAbilityEventDelegate, const FCussAbilityEvent&, EventData);

UCLASS(ClassGroup=(Cuss), meta=(BlueprintSpawnableComponent))
class CUSSABILITYSYSTEM_API UCussAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCussAbilityComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable)
	void GrantAbility(const UCussAbilityData* AbilityData, int32 AbilityLevel, FGameplayTag InputTag);

	UFUNCTION(BlueprintCallable)
	bool TryActivateAbilityByTag(FGameplayTag AbilityTag, AActor* OptionalTargetActor, FVector TargetLocation);

	UFUNCTION(BlueprintCallable)
	bool TryActivateAbilityByInputTag(FGameplayTag InputTag, AActor* OptionalTargetActor, FVector TargetLocation);

	UFUNCTION(BlueprintCallable)
	bool IsAbilityOnCooldown(FGameplayTag AbilityTag) const;

	UFUNCTION(BlueprintCallable)
	float GetRemainingCooldown(FGameplayTag AbilityTag) const;

	UFUNCTION(BlueprintCallable)
	bool HasMatchingOwnedTag(FGameplayTag Tag) const;

	UFUNCTION(BlueprintCallable)
	bool HasAnyMatchingOwnedTags(const FGameplayTagContainer& Tags) const;

	UPROPERTY(BlueprintAssignable)
	FCussAbilityEventDelegate OnAbilityEvent;

protected:
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
	void ServerTryActivateAbility(FGameplayTag AbilityTag, AActor* OptionalTargetActor, FVector_NetQuantize TargetLocation);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastAbilityEvent(const FCussAbilityEvent& EventData);

	UFUNCTION()
	void OnRep_OwnedTags();

	const FCussGrantedAbilitySpec* FindGrantedAbilityByTag(FGameplayTag AbilityTag) const;
	FCussGrantedAbilitySpec* FindGrantedAbilityByTagMutable(FGameplayTag AbilityTag);
	const FCussGrantedAbilitySpec* FindGrantedAbilityByInputTag(FGameplayTag InputTag) const;

	bool CanActivateAbility(const FCussGrantedAbilitySpec& Spec, AActor* OptionalTargetActor, const FVector& TargetLocation) const;
	bool CheckCosts(const UCussAbilityData* AbilityData) const;
	bool CheckCooldown(const FCussGrantedAbilitySpec& Spec) const;
	bool CheckOwnerTags(const UCussAbilityData* AbilityData) const;
	bool ValidateTargeting(const UCussAbilityData* AbilityData, AActor* OptionalTargetActor, const FVector& TargetLocation) const;

	void ActivateAbilityInternal(FCussGrantedAbilitySpec& Spec, AActor* OptionalTargetActor, const FVector& TargetLocation);
	void CommitCosts(const UCussAbilityData* AbilityData);
	void StartCooldown(FCussGrantedAbilitySpec& Spec);

	void ResolveAndApplyEffects(const UCussAbilityData* AbilityData, AActor* OptionalTargetActor, const FVector& TargetLocation);
	void ApplyEffectToResolvedTarget(const UCussAbilityData* AbilityData, const FCussAbilityEffectDef& EffectDef, AActor* ResolvedTarget, const FVector& TargetLocation);

	void ApplyInstantEffectFromSource(AActor* SourceActor, const UCussAbilityData* AbilityData, const FCussAbilityEffectDef& EffectDef, AActor* TargetActor, const FVector& EventLocation);
	void AddActiveEffectFromSource(AActor* SourceActor, const UCussAbilityData* AbilityData, const FCussAbilityEffectDef& EffectDef);

	void ApplyPeriodicEffectTick(const FCussActiveEffect& Effect);
	void RemoveActiveEffectById(FGuid EffectId);

	void AddOwnedTag(FGameplayTag Tag);
	void RemoveOwnedTag(FGameplayTag Tag);

	void HandleEffectPeriod(FGuid EffectId);
	void HandleEffectExpired(FGuid EffectId);

	void BroadcastEvent(AActor* InstigatorActor, AActor* TargetActor, FGameplayTag AbilityTag, ECussAbilityEventResult Result, float Magnitude, const FVector& EventLocation, FGameplayTag CueTag, const FGameplayTagContainer& AppliedTags);

	UCussAbilityComponent* GetAbilityComponent(AActor* Actor) const;
	UCussStatComponent* GetStatComponent(AActor* Actor) const;
	AActor* ResolvePrimaryTarget(const UCussAbilityData* AbilityData, AActor* OptionalTargetActor) const;
};