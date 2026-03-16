// Copyright Kyle Cuss and Cuss Programming 2026

#include "CussAbilitySystem/Public/Components/CussAbilityComponent.h"

#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "CussAbilitySystem/Public/Components/CussStatComponent.h"
#include "CussAbilitySystem/Public/Data/CussAbilityData.h"
#include "Net/UnrealNetwork.h"

UCussAbilityComponent::UCussAbilityComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UCussAbilityComponent::BeginPlay()
{
	Super::BeginPlay();
	StatComponent = GetOwner() ? GetOwner()->FindComponentByClass<UCussStatComponent>() : nullptr;
}

void UCussAbilityComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		for (TPair<FGuid, FTimerHandle>& Pair : EffectDurationTimers)
		{
			GetWorld()->GetTimerManager().ClearTimer(Pair.Value);
		}

		for (TPair<FGuid, FTimerHandle>& Pair : EffectPeriodTimers)
		{
			GetWorld()->GetTimerManager().ClearTimer(Pair.Value);
		}
	}

	EffectDurationTimers.Empty();
	EffectPeriodTimers.Empty();

	Super::EndPlay(EndPlayReason);
}

void UCussAbilityComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCussAbilityComponent, GrantedAbilities);
	DOREPLIFETIME(UCussAbilityComponent, OwnedTags);
	DOREPLIFETIME(UCussAbilityComponent, ActiveEffects);
}

void UCussAbilityComponent::GrantAbility(const UCussAbilityData* AbilityData, int32 AbilityLevel, FGameplayTag InputTag)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !AbilityData)
	{
		return;
	}

	FCussGrantedAbilitySpec NewSpec;
	NewSpec.AbilityData = AbilityData;
	NewSpec.AbilityLevel = AbilityLevel;
	NewSpec.InputTag = InputTag;
	NewSpec.CooldownEndTime = 0.f;

	GrantedAbilities.Add(NewSpec);
}

bool UCussAbilityComponent::TryActivateAbilityByTag(FGameplayTag AbilityTag, AActor* OptionalTargetActor, FVector TargetLocation)
{
	if (!AbilityTag.IsValid())
	{
		return false;
	}

	if (!GetOwner())
	{
		return false;
	}

	if (!GetOwner()->HasAuthority())
	{
		ServerTryActivateAbility(AbilityTag, OptionalTargetActor, TargetLocation);
		return true;
	}

	FCussGrantedAbilitySpec* Spec = FindGrantedAbilityByTagMutable(AbilityTag);
	if (!Spec)
	{
		BroadcastEvent(GetOwner(), OptionalTargetActor, AbilityTag, ECussAbilityEventResult::Failed, 0.f, TargetLocation, FGameplayTag(), FGameplayTagContainer());
		return false;
	}

	if (!CanActivateAbility(*Spec, OptionalTargetActor, TargetLocation))
	{
		BroadcastEvent(GetOwner(), OptionalTargetActor, AbilityTag, ECussAbilityEventResult::Failed, 0.f, TargetLocation, FGameplayTag(), FGameplayTagContainer());
		return false;
	}

	ActivateAbilityInternal(*Spec, OptionalTargetActor, TargetLocation);
	return true;
}

bool UCussAbilityComponent::TryActivateAbilityByInputTag(FGameplayTag InputTag, AActor* OptionalTargetActor, FVector TargetLocation)
{
	const FCussGrantedAbilitySpec* Spec = FindGrantedAbilityByInputTag(InputTag);
	return Spec && Spec->AbilityData ? TryActivateAbilityByTag(Spec->AbilityData->AbilityTag, OptionalTargetActor, TargetLocation) : false;
}

void UCussAbilityComponent::ServerTryActivateAbility_Implementation(FGameplayTag AbilityTag, AActor* OptionalTargetActor, FVector_NetQuantize TargetLocation)
{
	TryActivateAbilityByTag(AbilityTag, OptionalTargetActor, TargetLocation);
}

void UCussAbilityComponent::OnRep_OwnedTags()
{
}

bool UCussAbilityComponent::IsAbilityOnCooldown(FGameplayTag AbilityTag) const
{
	const FCussGrantedAbilitySpec* Spec = FindGrantedAbilityByTag(AbilityTag);
	return Spec ? !CheckCooldown(*Spec) : false;
}

float UCussAbilityComponent::GetRemainingCooldown(FGameplayTag AbilityTag) const
{
	const FCussGrantedAbilitySpec* Spec = FindGrantedAbilityByTag(AbilityTag);
	if (!Spec || !GetWorld())
	{
		return 0.f;
	}

	return FMath::Max(0.f, Spec->CooldownEndTime - GetWorld()->GetTimeSeconds());
}

bool UCussAbilityComponent::HasMatchingOwnedTag(FGameplayTag Tag) const
{
	return OwnedTags.HasTag(Tag);
}

bool UCussAbilityComponent::HasAnyMatchingOwnedTags(const FGameplayTagContainer& Tags) const
{
	return OwnedTags.HasAny(Tags);
}

const FCussGrantedAbilitySpec* UCussAbilityComponent::FindGrantedAbilityByTag(FGameplayTag AbilityTag) const
{
	return GrantedAbilities.FindByPredicate(
		[&AbilityTag](const FCussGrantedAbilitySpec& Spec)
		{
			return Spec.AbilityData && Spec.AbilityData->AbilityTag == AbilityTag;
		});
}

FCussGrantedAbilitySpec* UCussAbilityComponent::FindGrantedAbilityByTagMutable(FGameplayTag AbilityTag)
{
	return GrantedAbilities.FindByPredicate(
		[&AbilityTag](const FCussGrantedAbilitySpec& Spec)
		{
			return Spec.AbilityData && Spec.AbilityData->AbilityTag == AbilityTag;
		});
}

const FCussGrantedAbilitySpec* UCussAbilityComponent::FindGrantedAbilityByInputTag(FGameplayTag InputTag) const
{
	return GrantedAbilities.FindByPredicate(
		[&InputTag](const FCussGrantedAbilitySpec& Spec)
		{
			return Spec.InputTag == InputTag;
		});
}

bool UCussAbilityComponent::CanActivateAbility(const FCussGrantedAbilitySpec& Spec, AActor* OptionalTargetActor, const FVector& TargetLocation) const
{
	if (!Spec.IsValid() || !Spec.AbilityData)
	{
		return false;
	}

	return CheckCooldown(Spec)
		&& CheckCosts(Spec.AbilityData)
		&& CheckOwnerTags(Spec.AbilityData)
		&& ValidateTargeting(Spec.AbilityData, OptionalTargetActor, TargetLocation);
}

bool UCussAbilityComponent::CheckCosts(const UCussAbilityData* AbilityData) const
{
	if (!AbilityData || !StatComponent)
	{
		return false;
	}

	for (const FCussAbilityStatCost& Cost : AbilityData->Costs)
	{
		if (!StatComponent->HasEnoughStat(Cost.StatTag, Cost.Amount))
		{
			return false;
		}
	}

	return true;
}

bool UCussAbilityComponent::CheckCooldown(const FCussGrantedAbilitySpec& Spec) const
{
	if (!GetWorld())
	{
		return false;
	}

	return GetWorld()->GetTimeSeconds() >= Spec.CooldownEndTime;
}

bool UCussAbilityComponent::CheckOwnerTags(const UCussAbilityData* AbilityData) const
{
	if (!AbilityData)
	{
		return false;
	}

	if (!AbilityData->RequiredOwnerTags.IsEmpty() && !OwnedTags.HasAll(AbilityData->RequiredOwnerTags))
	{
		return false;
	}

	if (!AbilityData->BlockedOwnerTags.IsEmpty() && OwnedTags.HasAny(AbilityData->BlockedOwnerTags))
	{
		return false;
	}

	return true;
}

bool UCussAbilityComponent::ValidateTargeting(const UCussAbilityData* AbilityData, AActor* OptionalTargetActor, const FVector& TargetLocation) const
{
	if (!AbilityData || !GetOwner())
	{
		return false;
	}

	switch (AbilityData->Targeting.TargetMode)
	{
	case ECussAbilityTargetMode::Self:
		return true;

	case ECussAbilityTargetMode::Actor:
		return OptionalTargetActor != nullptr;

	case ECussAbilityTargetMode::Point:
		return !TargetLocation.IsNearlyZero();

	case ECussAbilityTargetMode::AreaSelf:
		return true;

	case ECussAbilityTargetMode::AreaTarget:
		return OptionalTargetActor != nullptr || !TargetLocation.IsNearlyZero();

	default:
		return false;
	}
}

void UCussAbilityComponent::ActivateAbilityInternal(FCussGrantedAbilitySpec& Spec, AActor* OptionalTargetActor, const FVector& TargetLocation)
{
	if (!Spec.AbilityData)
	{
		return;
	}

	BroadcastEvent(GetOwner(), OptionalTargetActor, Spec.AbilityData->AbilityTag, ECussAbilityEventResult::Activated, 0.f, TargetLocation, Spec.AbilityData->CastCueTag, FGameplayTagContainer());

	CommitCosts(Spec.AbilityData);
	BroadcastEvent(GetOwner(), OptionalTargetActor, Spec.AbilityData->AbilityTag, ECussAbilityEventResult::CostCommitted, 0.f, TargetLocation, Spec.AbilityData->CastCueTag, FGameplayTagContainer());

	StartCooldown(Spec);
	BroadcastEvent(GetOwner(), OptionalTargetActor, Spec.AbilityData->AbilityTag, ECussAbilityEventResult::CooldownStarted, 0.f, TargetLocation, Spec.AbilityData->CastCueTag, FGameplayTagContainer());

	ResolveAndApplyEffects(Spec.AbilityData, OptionalTargetActor, TargetLocation);
}

void UCussAbilityComponent::CommitCosts(const UCussAbilityData* AbilityData)
{
	if (!AbilityData || !StatComponent)
	{
		return;
	}

	for (const FCussAbilityStatCost& Cost : AbilityData->Costs)
	{
		StatComponent->ModifyStat(Cost.StatTag, -Cost.Amount, true);
	}
}

void UCussAbilityComponent::StartCooldown(FCussGrantedAbilitySpec& Spec)
{
	if (!GetWorld() || !Spec.AbilityData)
	{
		return;
	}

	Spec.CooldownEndTime = GetWorld()->GetTimeSeconds() + Spec.AbilityData->CooldownSeconds;
}

void UCussAbilityComponent::ResolveAndApplyEffects(const UCussAbilityData* AbilityData, AActor* OptionalTargetActor, const FVector& TargetLocation)
{
	if (!AbilityData)
	{
		return;
	}

	AActor* ResolvedTarget = ResolvePrimaryTarget(AbilityData, OptionalTargetActor);

	for (const FCussAbilityEffectDef& EffectDef : AbilityData->Effects)
	{
		ApplyEffectToResolvedTarget(AbilityData, EffectDef, ResolvedTarget, TargetLocation);
	}
}

void UCussAbilityComponent::ApplyEffectToResolvedTarget(const UCussAbilityData* AbilityData, const FCussAbilityEffectDef& EffectDef, AActor* ResolvedTarget, const FVector& TargetLocation)
{
	if (!AbilityData || !ResolvedTarget)
	{
		return;
	}

	if (EffectDef.Duration > 0.f)
	{
		if (UCussAbilityComponent* TargetAbilityComp = GetAbilityComponent(ResolvedTarget))
		{
			TargetAbilityComp->AddActiveEffectFromSource(GetOwner(), AbilityData, EffectDef);
		}
		return;
	}

	ApplyInstantEffectFromSource(GetOwner(), AbilityData, EffectDef, ResolvedTarget, TargetLocation);
}

void UCussAbilityComponent::ApplyInstantEffectFromSource(AActor* SourceActor, const UCussAbilityData* AbilityData, const FCussAbilityEffectDef& EffectDef, AActor* TargetActor, const FVector& EventLocation)
{
	if (!AbilityData || !TargetActor)
	{
		return;
	}

	UCussAbilityComponent* TargetAbilityComp = GetAbilityComponent(TargetActor);
	UCussStatComponent* TargetStats = GetStatComponent(TargetActor);

	float AppliedMagnitude = 0.f;
	FGameplayTagContainer AppliedTags;

	switch (EffectDef.EffectType)
	{
	case ECussAbilityEffectType::Damage:
		if (TargetStats)
		{
			AppliedMagnitude = -FMath::Abs(EffectDef.Magnitude);
			TargetStats->ModifyStat(EffectDef.AffectedStatTag, AppliedMagnitude, true);
		}
		break;

	case ECussAbilityEffectType::Heal:
		if (TargetStats)
		{
			AppliedMagnitude = FMath::Abs(EffectDef.Magnitude);
			TargetStats->ModifyStat(EffectDef.AffectedStatTag, AppliedMagnitude, true);
		}
		break;

	case ECussAbilityEffectType::ModifyStat:
		if (TargetStats)
		{
			AppliedMagnitude = EffectDef.Magnitude;
			TargetStats->ModifyStat(EffectDef.AffectedStatTag, AppliedMagnitude, true);
		}
		break;

	case ECussAbilityEffectType::ApplyTag:
		if (TargetAbilityComp && EffectDef.GrantedTag.IsValid())
		{
			TargetAbilityComp->AddOwnedTag(EffectDef.GrantedTag);
			AppliedTags.AddTag(EffectDef.GrantedTag);
		}
		break;

	case ECussAbilityEffectType::RemoveTag:
		if (TargetAbilityComp && EffectDef.GrantedTag.IsValid())
		{
			TargetAbilityComp->RemoveOwnedTag(EffectDef.GrantedTag);
		}
		break;

	default:
		break;
	}

	if (TargetAbilityComp)
	{
		TargetAbilityComp->BroadcastEvent(SourceActor, TargetActor, AbilityData->AbilityTag, ECussAbilityEventResult::EffectApplied, AppliedMagnitude, EventLocation, AbilityData->ImpactCueTag, AppliedTags);
	}
}

void UCussAbilityComponent::AddActiveEffectFromSource(AActor* SourceActor, const UCussAbilityData* AbilityData, const FCussAbilityEffectDef& EffectDef)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !AbilityData || !GetWorld())
	{
		return;
	}

	FCussActiveEffect NewEffect;
	NewEffect.EffectId = FGuid::NewGuid();
	NewEffect.AbilityTag = AbilityData->AbilityTag;
	NewEffect.SourceAbilityData = AbilityData;
	NewEffect.SourceActor = SourceActor;
	NewEffect.TargetActor = GetOwner();
	NewEffect.EffectType = EffectDef.EffectType;
	NewEffect.ModifiedStatTag = EffectDef.AffectedStatTag;
	NewEffect.Magnitude = EffectDef.Magnitude;
	NewEffect.Duration = EffectDef.Duration;
	NewEffect.Period = EffectDef.Period;
	NewEffect.StartTime = GetWorld()->GetTimeSeconds();
	NewEffect.EndTime = NewEffect.StartTime + EffectDef.Duration;
	NewEffect.Stacks = 1;
	NewEffect.bPeriodic = EffectDef.Period > 0.f;

	if (EffectDef.GrantedTag.IsValid())
	{
		NewEffect.GrantedTags.AddTag(EffectDef.GrantedTag);
		AddOwnedTag(EffectDef.GrantedTag);
	}

	ActiveEffects.Add(NewEffect);

	FTimerHandle DurationHandle;
	GetWorld()->GetTimerManager().SetTimer(
		DurationHandle,
		FTimerDelegate::CreateUObject(this, &UCussAbilityComponent::HandleEffectExpired, NewEffect.EffectId),
		EffectDef.Duration,
		false);
	EffectDurationTimers.Add(NewEffect.EffectId, DurationHandle);

	if (NewEffect.bPeriodic)
	{
		FTimerHandle PeriodHandle;
		GetWorld()->GetTimerManager().SetTimer(
			PeriodHandle,
			FTimerDelegate::CreateUObject(this, &UCussAbilityComponent::HandleEffectPeriod, NewEffect.EffectId),
			EffectDef.Period,
			true);
		EffectPeriodTimers.Add(NewEffect.EffectId, PeriodHandle);
	}

	BroadcastEvent(SourceActor, GetOwner(), AbilityData->AbilityTag, ECussAbilityEventResult::EffectAdded, 0.f, GetOwner()->GetActorLocation(), AbilityData->ImpactCueTag, NewEffect.GrantedTags);
}

void UCussAbilityComponent::ApplyPeriodicEffectTick(const FCussActiveEffect& Effect)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	UCussStatComponent* TargetStats = GetStatComponent(GetOwner());
	if (!TargetStats)
	{
		return;
	}

	float AppliedMagnitude = 0.f;

	switch (Effect.EffectType)
	{
	case ECussAbilityEffectType::Damage:
		AppliedMagnitude = -FMath::Abs(Effect.Magnitude);
		TargetStats->ModifyStat(Effect.ModifiedStatTag, AppliedMagnitude, true);
		break;

	case ECussAbilityEffectType::Heal:
		AppliedMagnitude = FMath::Abs(Effect.Magnitude);
		TargetStats->ModifyStat(Effect.ModifiedStatTag, AppliedMagnitude, true);
		break;

	case ECussAbilityEffectType::ModifyStat:
		AppliedMagnitude = Effect.Magnitude;
		TargetStats->ModifyStat(Effect.ModifiedStatTag, AppliedMagnitude, true);
		break;

	default:
		break;
	}

	BroadcastEvent(Effect.SourceActor.Get(), GetOwner(), Effect.AbilityTag, ECussAbilityEventResult::EffectApplied, AppliedMagnitude, GetOwner()->GetActorLocation(), FGameplayTag(), Effect.GrantedTags);
}

void UCussAbilityComponent::RemoveActiveEffectById(FGuid EffectId)
{
	const int32 Index = ActiveEffects.IndexOfByPredicate(
		[&EffectId](const FCussActiveEffect& Effect)
		{
			return Effect.EffectId == EffectId;
		});

	if (!ActiveEffects.IsValidIndex(Index))
	{
		return;
	}

	const FCussActiveEffect RemovedEffect = ActiveEffects[Index];

	if (FTimerHandle* PeriodHandle = EffectPeriodTimers.Find(EffectId))
	{
		GetWorld()->GetTimerManager().ClearTimer(*PeriodHandle);
		EffectPeriodTimers.Remove(EffectId);
	}

	if (FTimerHandle* DurationHandle = EffectDurationTimers.Find(EffectId))
	{
		GetWorld()->GetTimerManager().ClearTimer(*DurationHandle);
		EffectDurationTimers.Remove(EffectId);
	}

	for (const FGameplayTag& Tag : RemovedEffect.GrantedTags)
	{
		RemoveOwnedTag(Tag);
	}

	ActiveEffects.RemoveAt(Index);

	BroadcastEvent(RemovedEffect.SourceActor.Get(), GetOwner(), RemovedEffect.AbilityTag, ECussAbilityEventResult::EffectRemoved, 0.f, GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector, FGameplayTag(), RemovedEffect.GrantedTags);
}

void UCussAbilityComponent::AddOwnedTag(FGameplayTag Tag)
{
	if (Tag.IsValid())
	{
		OwnedTags.AddTag(Tag);
	}
}

void UCussAbilityComponent::RemoveOwnedTag(FGameplayTag Tag)
{
	if (Tag.IsValid())
	{
		OwnedTags.RemoveTag(Tag);
	}
}

void UCussAbilityComponent::HandleEffectPeriod(FGuid EffectId)
{
	const FCussActiveEffect* Effect = ActiveEffects.FindByPredicate(
		[&EffectId](const FCussActiveEffect& Item)
		{
			return Item.EffectId == EffectId;
		});

	if (!Effect)
	{
		return;
	}

	ApplyPeriodicEffectTick(*Effect);
}

void UCussAbilityComponent::HandleEffectExpired(FGuid EffectId)
{
	RemoveActiveEffectById(EffectId);
}

void UCussAbilityComponent::BroadcastEvent(AActor* InstigatorActor, AActor* TargetActor, FGameplayTag AbilityTag, ECussAbilityEventResult Result, float Magnitude, const FVector& EventLocation, FGameplayTag CueTag, const FGameplayTagContainer& AppliedTags)
{
	FCussAbilityEvent EventData;
	EventData.InstigatorActor = InstigatorActor;
	EventData.TargetActor = TargetActor;
	EventData.AbilityTag = AbilityTag;
	EventData.Result = Result;
	EventData.Magnitude = Magnitude;
	EventData.EventLocation = EventLocation;
	EventData.CueTag = CueTag;
	EventData.AppliedTags = AppliedTags;

	OnAbilityEvent.Broadcast(EventData);
	MulticastAbilityEvent(EventData);
}

void UCussAbilityComponent::MulticastAbilityEvent_Implementation(const FCussAbilityEvent& EventData)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		return;
	}

	OnAbilityEvent.Broadcast(EventData);
}

UCussAbilityComponent* UCussAbilityComponent::GetAbilityComponent(AActor* Actor) const
{
	return Actor ? Actor->FindComponentByClass<UCussAbilityComponent>() : nullptr;
}

UCussStatComponent* UCussAbilityComponent::GetStatComponent(AActor* Actor) const
{
	return Actor ? Actor->FindComponentByClass<UCussStatComponent>() : nullptr;
}

AActor* UCussAbilityComponent::ResolvePrimaryTarget(const UCussAbilityData* AbilityData, AActor* OptionalTargetActor) const
{
	if (!AbilityData || !GetOwner())
	{
		return nullptr;
	}

	switch (AbilityData->Targeting.TargetMode)
	{
	case ECussAbilityTargetMode::Self:
	case ECussAbilityTargetMode::AreaSelf:
		return GetOwner();

	case ECussAbilityTargetMode::Actor:
	case ECussAbilityTargetMode::AreaTarget:
		return OptionalTargetActor;

	default:
		return OptionalTargetActor;
	}
}

