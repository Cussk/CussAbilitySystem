// Copyright Kyle Cuss and Cuss Programming 2026

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "CussAbilitySystem/Public/CoreTypes/CussAbilityTypes.h"
#include "CussAbilityData.generated.h"

/** Data asset describing a single ability's tags, costs, targeting, and effects. */
UCLASS(BlueprintType)
class CUSSABILITYSYSTEM_API UCussAbilityData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability")
	FGameplayTag AbilityTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability")
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability")
	FGameplayTag CooldownGroupTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability")
	float CooldownSeconds = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability")
	TArray<FCussAbilityStatCost> Costs;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability")
	FCussAbilityTargetingDef Targeting;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability")
	TArray<FCussAbilityEffectDef> Effects;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability")
	FGameplayTag CastCueTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability")
	FGameplayTag ImpactCueTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability")
	FGameplayTagContainer RequiredOwnerTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability")
	FGameplayTagContainer BlockedOwnerTags;
};
