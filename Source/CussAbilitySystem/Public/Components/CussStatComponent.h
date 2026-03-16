// Copyright Kyle Cuss and Cuss Programming 2026

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "CussStatComponent.generated.h"

/** Replicated runtime stat entry storing the current and maximum value for a gameplay-tagged stat. */
USTRUCT(BlueprintType)
struct FCussRuntimeStat
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag StatTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float CurrentValue = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MaxValue = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCussOnStatChanged, FGameplayTag, StatTag, float, OldValue, float, NewValue);

/** Replicated stat container that supports initialization, queries, and runtime value changes. */
UCLASS(ClassGroup=(Cuss), meta=(BlueprintSpawnableComponent))
class CUSSABILITYSYSTEM_API UCussStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Enables replication and disables ticking for this stat storage component. */
	UCussStatComponent();

	/** Registers the replicated runtime stat array. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(BlueprintCallable, Category="Cuss|Stats")
	/** Creates or overwrites a stat entry with the supplied current and max values. */
	bool InitializeStat(FGameplayTag StatTag, float CurrentValue, float MaxValue);

	UFUNCTION(BlueprintCallable)
	/** Returns whether a stat entry exists for the supplied gameplay tag. */
	bool HasStat(FGameplayTag StatTag) const;

	UFUNCTION(BlueprintCallable)
	float GetCurrentStatValue(FGameplayTag StatTag) const;

	UFUNCTION(BlueprintCallable)
	float GetMaxStatValue(FGameplayTag StatTag) const;

	UFUNCTION(BlueprintCallable)
	/** Returns whether the current stat value meets or exceeds the requested amount. */
	bool HasEnoughStat(FGameplayTag StatTag, float RequiredAmount) const;

	UFUNCTION(BlueprintCallable)
	/** Adds the supplied delta to the current stat value and broadcasts the change. */
	bool ModifyStat(FGameplayTag StatTag, float Delta, bool bClampToMax = true);

	UFUNCTION(BlueprintCallable)
	bool SetStatCurrent(FGameplayTag StatTag, float NewValue, bool bClampToMax = true);

	UPROPERTY(BlueprintAssignable)
	FCussOnStatChanged OnStatChanged;

protected:
	UPROPERTY(EditAnywhere, ReplicatedUsing=OnRep_RuntimeStats)
	TArray<FCussRuntimeStat> RuntimeStats;

	UFUNCTION()
	/** Receives replicated stat array updates. */
	void OnRep_RuntimeStats();

	/** Finds the array index for the supplied stat tag, or `INDEX_NONE` if missing. */
	int32 FindStatIndex(FGameplayTag StatTag) const;
};
