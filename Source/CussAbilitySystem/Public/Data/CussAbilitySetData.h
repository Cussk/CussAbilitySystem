//Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "CussAbilitySetData.generated.h"

class UCussAbilityData;

/** Single ability grant entry used by an ability set asset. */
USTRUCT(BlueprintType)
struct FCussAbilitySetEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability")
	TObjectPtr<UCussAbilityData> AbilityData = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability")
	int32 AbilityLevel = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability")
	FGameplayTag InputTag;
};

/** Data asset that groups multiple ability grants for startup or bulk assignment. */
UCLASS(BlueprintType)
class CUSSABILITYSYSTEM_API UCussAbilitySetData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability Set")
	TArray<FCussAbilitySetEntry> Abilities;
};
