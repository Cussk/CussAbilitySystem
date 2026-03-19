//Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "CussAbilityDebugTypes.generated.h"

UENUM(BlueprintType)
enum class ECussAbilityActivationFailReason : uint8
{
	None,
	NotGranted,
	OnCooldown,
	InsufficientResources,
	BlockedByTags,
	MissingRequiredTags,
	InvalidTarget,
	OutOfRange,
	InvalidContext,
	Unknown
};

UENUM(BlueprintType)
enum class ECussCombatLogEntryType : uint8
{
	Info,
	Activation,
	Validation,
	Delivery,
	Effect,
	Cooldown
};

/** Lightweight display snapshot describing one activation attempt for demo-facing validation feedback. */
USTRUCT(BlueprintType)
struct FCussAbilityActivationResultView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag AbilityTag;

	UPROPERTY(BlueprintReadOnly)
	FText AbilityName;

	UPROPERTY(BlueprintReadOnly)
	bool bSuccess = false;

	UPROPERTY(BlueprintReadOnly)
	ECussAbilityActivationFailReason FailReason = ECussAbilityActivationFailReason::Unknown;

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag RelatedTag;

	UPROPERTY(BlueprintReadOnly)
	float CooldownRemaining = 0.f;

	UPROPERTY(BlueprintReadOnly)
	FText Message;
};

/** Lightweight display snapshot describing one granted hotbar slot for demo-facing cooldown visibility. */
USTRUCT(BlueprintType)
struct FCussAbilitySlotView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag AbilityTag;

	UPROPERTY(BlueprintReadOnly)
	FText AbilityName;

	UPROPERTY(BlueprintReadOnly)
	bool bIsGranted = false;

	UPROPERTY(BlueprintReadOnly)
	bool bIsOnCooldown = false;

	UPROPERTY(BlueprintReadOnly)
	float CooldownRemaining = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float CooldownDuration = 0.f;

	UPROPERTY(BlueprintReadOnly)
	bool bIsBlocked = false;
};

/** Lightweight display snapshot describing one currently active effect on the ability component owner. */
USTRUCT(BlueprintType)
struct FCussActiveEffectView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag EffectTag;

	UPROPERTY(BlueprintReadOnly)
	FText EffectName;

	UPROPERTY(BlueprintReadOnly)
	int32 StackCount = 0;

	UPROPERTY(BlueprintReadOnly)
	bool bInfiniteDuration = false;

	UPROPERTY(BlueprintReadOnly)
	float TimeRemaining = 0.f;

	UPROPERTY(BlueprintReadOnly)
	FText SourceAbilityName;
};

/** Lightweight display entry pushed into the local debug combat log for key runtime moments. */
USTRUCT(BlueprintType)
struct FCussCombatLogEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	ECussCombatLogEntryType Type = ECussCombatLogEntryType::Info;

	UPROPERTY(BlueprintReadOnly)
	FText Message;

	UPROPERTY(BlueprintReadOnly)
	float WorldTimeSeconds = 0.f;
};
