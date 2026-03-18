//Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "CoreTypes/CussAbilityTypes.h"
#include "CussAbilityData.generated.h"

/** Data asset describing a single ability's tags, delivery, costs, targeting, and effects for the current slice. */
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Delivery")
	ECussAbilityDeliveryType DeliveryType = ECussAbilityDeliveryType::Instant;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Delivery", meta=(EditCondition="DeliveryType == ECussAbilityDeliveryType::Projectile", EditConditionHides))
	FCussProjectileDeliveryDef ProjectileDelivery;

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
