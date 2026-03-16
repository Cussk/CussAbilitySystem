// Copyright Kyle Cuss and Cuss Programming 2026

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "CussAbilityTypes.generated.h"

class AActor;
class UCussAbilityData;

UENUM(BlueprintType)
enum class ECussAbilityTargetMode : uint8
{
	None,
	Self,
	Actor,
	Point,
	AreaSelf,
	AreaTarget
};

UENUM(BlueprintType)
enum class ECussAbilityTargetFilter : uint8
{
	None,
	Self,
	Ally,
	Enemy,
	AnyLiving,
	Any
};

UENUM(BlueprintType)
enum class ECussAbilityEffectType : uint8
{
	None,
	Damage,
	Heal,
	ModifyStat,
	ApplyTag,
	RemoveTag
};

UENUM(BlueprintType)
enum class ECussAbilityEventResult : uint8
{
	None,
	Activated,
	Failed,
	CostCommitted,
	CooldownStarted,
	EffectApplied,
	EffectAdded,
	EffectRemoved
};

USTRUCT(BlueprintType)
struct FCussAbilityStatCost
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag StatTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float Amount = 0.f;
};

USTRUCT(BlueprintType)
struct FCussAbilityTargetingDef
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	ECussAbilityTargetMode TargetMode = ECussAbilityTargetMode::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	ECussAbilityTargetFilter TargetFilter = ECussAbilityTargetFilter::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float Radius = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bRequireLineOfSight = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bAllowDeadTargets = false;
};

USTRUCT(BlueprintType)
struct FCussAbilityEffectDef
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	ECussAbilityEffectType EffectType = ECussAbilityEffectType::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag AffectedStatTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag GrantedTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float Magnitude = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float Duration = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float Period = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 MaxStacks = 1;
};

USTRUCT(BlueprintType)
struct FCussGrantedAbilitySpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<const UCussAbilityData> AbilityData = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 AbilityLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag InputTag;

	UPROPERTY()
	float CooldownEndTime = 0.f;

	bool IsValid() const
	{
		return AbilityData != nullptr;
	}
};

USTRUCT(BlueprintType)
struct FCussActiveEffect
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid EffectId;

	UPROPERTY()
	FGameplayTag AbilityTag;

	UPROPERTY()
	TObjectPtr<const UCussAbilityData> SourceAbilityData = nullptr;

	UPROPERTY()
	TWeakObjectPtr<AActor> SourceActor;

	UPROPERTY()
	TWeakObjectPtr<AActor> TargetActor;

	UPROPERTY()
	ECussAbilityEffectType EffectType = ECussAbilityEffectType::None;

	UPROPERTY()
	FGameplayTag ModifiedStatTag;

	UPROPERTY()
	FGameplayTagContainer GrantedTags;

	UPROPERTY()
	float Magnitude = 0.f;

	UPROPERTY()
	float Duration = 0.f;

	UPROPERTY()
	float Period = 0.f;

	UPROPERTY()
	float StartTime = 0.f;

	UPROPERTY()
	float EndTime = 0.f;

	UPROPERTY()
	int32 Stacks = 1;

	UPROPERTY()
	bool bPeriodic = false;
};

USTRUCT(BlueprintType)
struct FCussAbilityEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<AActor> InstigatorActor;

	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<AActor> TargetActor;

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag AbilityTag;

	UPROPERTY(BlueprintReadOnly)
	ECussAbilityEventResult Result = ECussAbilityEventResult::None;

	UPROPERTY(BlueprintReadOnly)
	float Magnitude = 0.f;

	UPROPERTY(BlueprintReadOnly)
	FVector EventLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag CueTag;

	UPROPERTY(BlueprintReadOnly)
	FGameplayTagContainer AppliedTags;
};
