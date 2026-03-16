// Copyright Kyle Cuss and Cuss Programming 2026

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "CussStatComponent.generated.h"

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

UCLASS(ClassGroup=(Cuss), meta=(BlueprintSpawnableComponent))
class CUSSABILITYSYSTEM_API UCussStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCussStatComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable)
	bool HasStat(FGameplayTag StatTag) const;

	UFUNCTION(BlueprintCallable)
	float GetCurrentStatValue(FGameplayTag StatTag) const;

	UFUNCTION(BlueprintCallable)
	float GetMaxStatValue(FGameplayTag StatTag) const;

	UFUNCTION(BlueprintCallable)
	bool HasEnoughStat(FGameplayTag StatTag, float RequiredAmount) const;

	UFUNCTION(BlueprintCallable)
	bool ModifyStat(FGameplayTag StatTag, float Delta, bool bClampToMax = true);

	UFUNCTION(BlueprintCallable)
	bool SetStatCurrent(FGameplayTag StatTag, float NewValue, bool bClampToMax = true);

	UPROPERTY(BlueprintAssignable)
	FCussOnStatChanged OnStatChanged;

protected:
	UPROPERTY(EditAnywhere, ReplicatedUsing=OnRep_RuntimeStats)
	TArray<FCussRuntimeStat> RuntimeStats;

	UFUNCTION()
	void OnRep_RuntimeStats();

	int32 FindStatIndex(FGameplayTag StatTag) const;
};
