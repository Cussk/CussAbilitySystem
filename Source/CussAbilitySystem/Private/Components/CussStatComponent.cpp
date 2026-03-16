// Copyright Kyle Cuss and Cuss Programming 2026

#include "CussAbilitySystem/Public/Components/CussStatComponent.h"

#include "Net/UnrealNetwork.h"

UCussStatComponent::UCussStatComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UCussStatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCussStatComponent, RuntimeStats);
}

bool UCussStatComponent::HasStat(FGameplayTag StatTag) const
{
	return FindStatIndex(StatTag) != INDEX_NONE;
}

float UCussStatComponent::GetCurrentStatValue(FGameplayTag StatTag) const
{
	const int32 Index = FindStatIndex(StatTag);
	return RuntimeStats.IsValidIndex(Index) ? RuntimeStats[Index].CurrentValue : 0.f;
}

float UCussStatComponent::GetMaxStatValue(FGameplayTag StatTag) const
{
	const int32 Index = FindStatIndex(StatTag);
	return RuntimeStats.IsValidIndex(Index) ? RuntimeStats[Index].MaxValue : 0.f;
}

bool UCussStatComponent::HasEnoughStat(FGameplayTag StatTag, float RequiredAmount) const
{
	return GetCurrentStatValue(StatTag) >= RequiredAmount;
}

bool UCussStatComponent::ModifyStat(FGameplayTag StatTag, float Delta, bool bClampToMax)
{
	const int32 Index = FindStatIndex(StatTag);
	if (!RuntimeStats.IsValidIndex(Index))
	{
		return false;
	}

	const float OldValue = RuntimeStats[Index].CurrentValue;
	float NewValue = OldValue + Delta;

	if (bClampToMax)
	{
		NewValue = FMath::Clamp(NewValue, 0.f, RuntimeStats[Index].MaxValue);
	}

	RuntimeStats[Index].CurrentValue = NewValue;
	OnStatChanged.Broadcast(StatTag, OldValue, NewValue);
	return true;
}

bool UCussStatComponent::SetStatCurrent(FGameplayTag StatTag, float NewValue, bool bClampToMax)
{
	const int32 Index = FindStatIndex(StatTag);
	if (!RuntimeStats.IsValidIndex(Index))
	{
		return false;
	}

	const float OldValue = RuntimeStats[Index].CurrentValue;

	if (bClampToMax)
	{
		NewValue = FMath::Clamp(NewValue, 0.f, RuntimeStats[Index].MaxValue);
	}

	RuntimeStats[Index].CurrentValue = NewValue;
	OnStatChanged.Broadcast(StatTag, OldValue, NewValue);
	return true;
}

void UCussStatComponent::OnRep_RuntimeStats()
{
}

int32 UCussStatComponent::FindStatIndex(FGameplayTag StatTag) const
{
	return RuntimeStats.IndexOfByPredicate(
		[&StatTag](const FCussRuntimeStat& Stat)
		{
			return Stat.StatTag == StatTag;
		});
}
