// Copyright Kyle Cuss and Cuss Programming 2026

#include "CussAbilitySystem/Public/Components/CussAbilityComponent.h"

#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "CoreTypes/CussAbilityGameplayTags.h"
#include "CussAbilitySystem/Public/Components/CussStatComponent.h"
#include "CussAbilitySystem/Public/Data/CussAbilityData.h"
#include "Data/CussAbilitySetData.h"
#include "Engine/OverlapResult.h"
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
	
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		InitializeStartupOwnedTags();
		GrantStartupAbilitySets();
	}
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
	DOREPLIFETIME(UCussAbilityComponent, CooldownGroupStates);
}

void UCussAbilityComponent::GrantAbility(UCussAbilityData* AbilityData, int32 AbilityLevel, FGameplayTag InputTag)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !AbilityData)
	{
		return;
	}

	const FCussGrantedAbilitySpec* ExistingSpec = FindGrantedAbilityByTag(AbilityData->AbilityTag);
	if (ExistingSpec)
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

void UCussAbilityComponent::GrantAbilitySet(const UCussAbilitySetData* AbilitySet)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !AbilitySet)
	{
		return;
	}

	for (const FCussAbilitySetEntry& Entry : AbilitySet->Abilities)
	{
		if (!Entry.AbilityData)
		{
			continue;
		}

		GrantAbility(Entry.AbilityData, Entry.AbilityLevel, Entry.InputTag);
	}
}

void UCussAbilityComponent::GrantStartupAbilitySets()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	for (const UCussAbilitySetData* AbilitySet : StartupAbilitySets)
	{
		GrantAbilitySet(AbilitySet);
	}
}

void UCussAbilityComponent::InitializeStartupOwnedTags()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	for (const FGameplayTag& Tag : StartupOwnedTags)
	{
		AddOwnedTag(Tag);
	}
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
	if (!Spec || !Spec->AbilityData || !GetWorld())
	{
		return 0.f;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	const float AbilityRemaining = FMath::Max(0.f, Spec->CooldownEndTime - CurrentTime);
	const float GroupRemaining = Spec->AbilityData->CooldownGroupTag.IsValid()
		? FMath::Max(0.f, GetCooldownGroupEndTime(Spec->AbilityData->CooldownGroupTag) - CurrentTime)
		: 0.f;

	return FMath::Max(AbilityRemaining, GroupRemaining);
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

bool UCussAbilityComponent::CheckCosts(UCussAbilityData* AbilityData) const
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
	if (!GetWorld() || !Spec.AbilityData)
	{
		return false;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();

	if (CurrentTime < Spec.CooldownEndTime)
	{
		return false;
	}

	if (Spec.AbilityData->CooldownGroupTag.IsValid())
	{
		if (CurrentTime < GetCooldownGroupEndTime(Spec.AbilityData->CooldownGroupTag))
		{
			return false;
		}
	}

	return true;
}

bool UCussAbilityComponent::CheckOwnerTags(UCussAbilityData* AbilityData) const
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

bool UCussAbilityComponent::ValidateTargeting(UCussAbilityData* AbilityData, AActor* OptionalTargetActor, const FVector& TargetLocation) const
{
	if (!AbilityData || !GetOwner())
	{
		return false;
	}

	if (AbilityData->Targeting.TargetMode == ECussAbilityTargetMode::Point)
	{
		return !TargetLocation.IsNearlyZero();
	}

	TArray<AActor*> ResolvedTargets;
	ResolveTargetsForAbility(AbilityData, OptionalTargetActor, TargetLocation, ResolvedTargets);
	return ResolvedTargets.Num() > 0;
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

void UCussAbilityComponent::CommitCosts(UCussAbilityData* AbilityData)
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

	const float CooldownEndTime = GetWorld()->GetTimeSeconds() + Spec.AbilityData->CooldownSeconds;
	Spec.CooldownEndTime = CooldownEndTime;

	if (Spec.AbilityData->CooldownGroupTag.IsValid())
	{
		SetCooldownGroupEndTime(Spec.AbilityData->CooldownGroupTag, CooldownEndTime);
	}
}

void UCussAbilityComponent::ResolveAndApplyEffects(UCussAbilityData* AbilityData, AActor* OptionalTargetActor, const FVector& TargetLocation)
{
	if (!AbilityData)
	{
		return;
	}

	TArray<AActor*> ResolvedTargets;
	ResolveTargetsForAbility(AbilityData, OptionalTargetActor, TargetLocation, ResolvedTargets);

	for (AActor* ResolvedTarget : ResolvedTargets)
	{
		if (!ResolvedTarget)
		{
			continue;
		}

		for (const FCussAbilityEffectDef& EffectDef : AbilityData->Effects)
		{
			ApplyEffectToResolvedTarget(AbilityData, EffectDef, ResolvedTarget, TargetLocation);
		}
	}
}

void UCussAbilityComponent::ResolveTargetsForAbility(const UCussAbilityData* AbilityData, AActor* OptionalTargetActor, const FVector& TargetLocation, TArray<AActor*>& OutTargets) const
{
	OutTargets.Reset();

	if (!AbilityData || !GetOwner())
	{
		return;
	}

	switch (AbilityData->Targeting.TargetMode)
	{
	case ECussAbilityTargetMode::Self:
		if (IsTargetValidForAbility(AbilityData, GetOwner()))
		{
			OutTargets.Add(GetOwner());
		}
		break;

	case ECussAbilityTargetMode::Actor:
		if (IsTargetValidForAbility(AbilityData, OptionalTargetActor))
		{
			OutTargets.Add(OptionalTargetActor);
		}
		break;

	case ECussAbilityTargetMode::AreaSelf:
		GatherAreaTargets(AbilityData, GetOwner()->GetActorLocation(), OutTargets);
		break;

	case ECussAbilityTargetMode::AreaTarget:
		if (OptionalTargetActor)
		{
			GatherAreaTargets(AbilityData, OptionalTargetActor->GetActorLocation(), OutTargets);
		}
		else if (!TargetLocation.IsNearlyZero())
		{
			GatherAreaTargets(AbilityData, TargetLocation, OutTargets);
		}
		break;

	default:
		break;
	}
}

void UCussAbilityComponent::GatherAreaTargets(const UCussAbilityData* AbilityData, const FVector& Origin, TArray<AActor*>& OutTargets) const
{
	if (!AbilityData || !GetWorld() || AbilityData->Targeting.Radius <= 0.f)
	{
		return;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CussAbilityAreaTargeting), false);

	GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		Origin,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(AbilityData->Targeting.Radius),
		QueryParams);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* HitActor = Overlap.GetActor();
		if (!HitActor)
		{
			continue;
		}

		if (OutTargets.Contains(HitActor))
		{
			continue;
		}

		if (!IsTargetValidForAbility(AbilityData, HitActor))
		{
			continue;
		}

		OutTargets.Add(HitActor);
	}
}

void UCussAbilityComponent::ApplyEffectToResolvedTarget(UCussAbilityData* AbilityData, const FCussAbilityEffectDef& EffectDef, AActor* ResolvedTarget, const FVector& TargetLocation)
{
	if (!AbilityData || !ResolvedTarget)
	{
		return;
	}

	FCussEffectContext EffectContext;
	EffectContext.SourceActor = GetOwner();
	EffectContext.TargetActor = ResolvedTarget;
	EffectContext.SourceAbilityData = AbilityData;
	EffectContext.AbilityTag = AbilityData->AbilityTag;
	EffectContext.EventLocation = !TargetLocation.IsNearlyZero() ? TargetLocation : ResolvedTarget->GetActorLocation();
	EffectContext.ResolvedMagnitude = EffectDef.Magnitude;
	EffectContext.AbilityLevel = 1;

	if (EffectDef.Duration > 0.f)
	{
		if (UCussAbilityComponent* TargetAbilityComp = GetAbilityComponent(ResolvedTarget))
		{
			TargetAbilityComp->AddActiveEffectFromContext(EffectContext, EffectDef);
		}
		return;
	}

	ApplyInstantEffectFromContext(EffectContext, EffectDef);
}

void UCussAbilityComponent::ApplyInstantEffectFromContext(const FCussEffectContext& EffectContext, const FCussAbilityEffectDef& EffectDef)
{
	AActor* TargetActor = EffectContext.TargetActor.Get();
	if (!TargetActor)
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
			AppliedMagnitude = -FMath::Abs(EffectContext.ResolvedMagnitude);
			TargetStats->ModifyStat(EffectDef.AffectedStatTag, AppliedMagnitude, true);
		}
		break;

	case ECussAbilityEffectType::Heal:
		if (TargetStats)
		{
			AppliedMagnitude = FMath::Abs(EffectContext.ResolvedMagnitude);
			TargetStats->ModifyStat(EffectDef.AffectedStatTag, AppliedMagnitude, true);
		}
		break;

	case ECussAbilityEffectType::ModifyStat:
		if (TargetStats)
		{
			AppliedMagnitude = EffectContext.ResolvedMagnitude;
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
		TargetAbilityComp->BroadcastEvent(
			EffectContext.SourceActor.Get(),
			TargetActor,
			EffectContext.AbilityTag,
			ECussAbilityEventResult::EffectApplied,
			AppliedMagnitude,
			EffectContext.EventLocation,
			EffectContext.SourceAbilityData ? EffectContext.SourceAbilityData->ImpactCueTag : FGameplayTag(),
			AppliedTags);
	}
}

void UCussAbilityComponent::AddActiveEffectFromContext(const FCussEffectContext& EffectContext, const FCussAbilityEffectDef& EffectDef)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !GetWorld())
	{
		return;
	}

	// Phase 2B: stacking / refresh
	if (FCussActiveEffect* Existing = FindMatchingActiveEffect(EffectContext, EffectDef))
	{
		switch (EffectDef.StackingPolicy)
		{
		case ECussEffectStackingPolicy::RefreshDuration:
			RefreshActiveEffectDuration(*Existing);
			return;

		case ECussEffectStackingPolicy::RejectNew:
			return;

		case ECussEffectStackingPolicy::AddNew:
		default:
			break;
		}
	}

	FCussActiveEffect NewEffect;
	NewEffect.EffectId = FGuid::NewGuid();
	NewEffect.AbilityTag = EffectContext.AbilityTag;
	NewEffect.SourceAbilityData = EffectContext.SourceAbilityData;
	NewEffect.SourceActor = EffectContext.SourceActor;
	NewEffect.TargetActor = GetOwner();
	NewEffect.EffectType = EffectDef.EffectType;
	NewEffect.ModifiedStatTag = EffectDef.AffectedStatTag;
	NewEffect.Magnitude = EffectContext.ResolvedMagnitude;
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

	// 🔹 APPLY INITIAL STAT MODIFIER (for buffs)
	if (EffectDef.EffectType == ECussAbilityEffectType::ModifyStat)
	{
		if (UCussStatComponent* Stats = GetStatComponent(GetOwner()))
		{
			Stats->ModifyStat(EffectDef.AffectedStatTag, NewEffect.Magnitude, true);
		}
	}

	ActiveEffects.Add(NewEffect);

	ResetDurationTimer(NewEffect);

	if (NewEffect.bPeriodic)
	{
		ResetPeriodTimer(NewEffect);
	}

	BroadcastEvent(
		EffectContext.SourceActor.Get(),
		GetOwner(),
		EffectContext.AbilityTag,
		ECussAbilityEventResult::EffectAdded,
		0.f,
		GetOwner()->GetActorLocation(),
		EffectContext.SourceAbilityData ? EffectContext.SourceAbilityData->ImpactCueTag : FGameplayTag(),
		NewEffect.GrantedTags);
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
	
	if (RemovedEffect.EffectType == ECussAbilityEffectType::ModifyStat)
	{
		if (UCussStatComponent* Stats = GetStatComponent(GetOwner()))
		{
			Stats->ModifyStat(RemovedEffect.ModifiedStatTag, -RemovedEffect.Magnitude, true);
		}
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
UCussAbilityComponent* UCussAbilityComponent::GetAbilityComponent(const AActor* Actor) const
{
	return Actor ? Actor->FindComponentByClass<UCussAbilityComponent>() : nullptr;
}

UCussStatComponent* UCussAbilityComponent::GetStatComponent(const AActor* Actor) const
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

bool UCussAbilityComponent::IsTargetValidForAbility(const UCussAbilityData* AbilityData, const AActor* CandidateTarget) const
{
	if (!AbilityData || !GetOwner() || !CandidateTarget)
	{
		return false;
	}
	
	const UCussAbilityComponent* TargetAbilityComp = GetAbilityComponent(CandidateTarget);
	const bool bIsDead = TargetAbilityComp && TargetAbilityComp->HasMatchingOwnedTag(CussAbilityTags::TAG_Status_Dead);

	if (!AbilityData->Targeting.bAllowDeadTargets && bIsDead)
	{
		return false;
	}

	switch (AbilityData->Targeting.TargetFilter)
	{
	case ECussAbilityTargetFilter::Self:
		if (CandidateTarget != GetOwner())
		{
			return false;
		}
		break;

	case ECussAbilityTargetFilter::Ally:
		if (!AreActorsAllies(GetOwner(), CandidateTarget))
		{
			return false;
		}
		break;

	case ECussAbilityTargetFilter::Enemy:
		if (!AreActorsEnemies(GetOwner(), CandidateTarget))
		{
			return false;
		}
		break;

	case ECussAbilityTargetFilter::AnyLiving:
		if (bIsDead)
		{
			return false;
		}
		break;

	case ECussAbilityTargetFilter::Any:
	case ECussAbilityTargetFilter::None:
	default:
		break;
	}

	if (AbilityData->Targeting.bRequireLineOfSight && CandidateTarget != GetOwner())
	{
		if (!HasLineOfSightToActor(CandidateTarget))
		{
			return false;
		}
	}

	return true;
}

bool UCussAbilityComponent::HasLineOfSightToActor(const AActor* OtherActor) const
{
	if (!GetOwner() || !OtherActor || !GetWorld())
	{
		return false;
	}

	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CussAbilityLOS), false);
	QueryParams.AddIgnoredActor(GetOwner());
	QueryParams.AddIgnoredActor(OtherActor);

	const FVector Start = GetOwner()->GetActorLocation();
	const FVector End = OtherActor->GetActorLocation();

	const bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams);
	return !bHit;
}

FGameplayTag UCussAbilityComponent::GetTeamTagForActor(const AActor* Actor) const
{
	if (!Actor)
	{
		return FGameplayTag();
	}

	const UCussAbilityComponent* OtherComp = GetAbilityComponent(Actor);
	if (!OtherComp)
	{
		return FGameplayTag();
	};

	if (OtherComp->OwnedTags.HasTag(CussAbilityTags::TAG_Team_Player))
	{
		return CussAbilityTags::TAG_Team_Player;
	}

	if (OtherComp->OwnedTags.HasTag(CussAbilityTags::TAG_Team_Enemy))
	{
		return CussAbilityTags::TAG_Team_Enemy;
	}

	return FGameplayTag();
}

bool UCussAbilityComponent::AreActorsAllies(const AActor* ActorA, const AActor* ActorB) const
{
	if (!ActorA || !ActorB)
	{
		return false;
	}

	if (ActorA == ActorB)
	{
		return true;
	}

	const FGameplayTag TeamA = GetTeamTagForActor(ActorA);
	const FGameplayTag TeamB = GetTeamTagForActor(ActorB);

	return TeamA.IsValid() && TeamB.IsValid() && TeamA == TeamB;
}

bool UCussAbilityComponent::AreActorsEnemies(const AActor* ActorA, const AActor* ActorB) const
{
	if (!ActorA || !ActorB || ActorA == ActorB)
	{
		return false;
	}

	const FGameplayTag TeamA = GetTeamTagForActor(ActorA);
	const FGameplayTag TeamB = GetTeamTagForActor(ActorB);

	return TeamA.IsValid() && TeamB.IsValid() && TeamA != TeamB;
}

float UCussAbilityComponent::GetCooldownGroupEndTime(FGameplayTag CooldownGroupTag) const
{
	if (!CooldownGroupTag.IsValid())
	{
		return 0.f;
	}

	const FCussCooldownGroupState* FoundState = CooldownGroupStates.FindByPredicate(
		[&CooldownGroupTag](const FCussCooldownGroupState& State)
		{
			return State.CooldownGroupTag == CooldownGroupTag;
		});

	return FoundState ? FoundState->EndTime : 0.f;
}

void UCussAbilityComponent::SetCooldownGroupEndTime(FGameplayTag CooldownGroupTag, float EndTime)
{
	if (!CooldownGroupTag.IsValid())
	{
		return;
	}

	FCussCooldownGroupState* ExistingState = CooldownGroupStates.FindByPredicate(
		[&CooldownGroupTag](const FCussCooldownGroupState& State)
		{
			return State.CooldownGroupTag == CooldownGroupTag;
		});

	if (ExistingState)
	{
		ExistingState->EndTime = EndTime;
		return;
	}

	FCussCooldownGroupState NewState;
	NewState.CooldownGroupTag = CooldownGroupTag;
	NewState.EndTime = EndTime;
	CooldownGroupStates.Add(NewState);
}

float UCussAbilityComponent::GetRemainingCooldownForGroup(FGameplayTag CooldownGroupTag) const
{
	if (!GetWorld() || !CooldownGroupTag.IsValid())
	{
		return 0.f;
	}

	return FMath::Max(0.f, GetCooldownGroupEndTime(CooldownGroupTag) - GetWorld()->GetTimeSeconds());
}

FCussActiveEffect* UCussAbilityComponent::FindMatchingActiveEffect(const FCussEffectContext& EffectContext, const FCussAbilityEffectDef& EffectDef)
{
	return ActiveEffects.FindByPredicate(
		[&](const FCussActiveEffect& Effect)
		{
			return Effect.AbilityTag == EffectContext.AbilityTag
				&& Effect.EffectType == EffectDef.EffectType
				&& Effect.ModifiedStatTag == EffectDef.AffectedStatTag;
		});
}

void UCussAbilityComponent::RefreshActiveEffectDuration(FCussActiveEffect& ActiveEffect)
{
	if (!GetWorld())
	{
		return;
	}

	ActiveEffect.StartTime = GetWorld()->GetTimeSeconds();
	ActiveEffect.EndTime = ActiveEffect.StartTime + ActiveEffect.Duration;

	ResetDurationTimer(ActiveEffect);
}

void UCussAbilityComponent::ResetDurationTimer(const FCussActiveEffect& ActiveEffect)
{
	if (!GetWorld())
	{
		return;
	}

	if (FTimerHandle* Existing = EffectDurationTimers.Find(ActiveEffect.EffectId))
	{
		GetWorld()->GetTimerManager().ClearTimer(*Existing);
	}

	FTimerHandle NewHandle;
	GetWorld()->GetTimerManager().SetTimer(
		NewHandle,
		FTimerDelegate::CreateUObject(this, &UCussAbilityComponent::HandleEffectExpired, ActiveEffect.EffectId),
		ActiveEffect.Duration,
		false);

	EffectDurationTimers.Add(ActiveEffect.EffectId, NewHandle);
}

void UCussAbilityComponent::ResetPeriodTimer(const FCussActiveEffect& ActiveEffect)
{
	if (!GetWorld() || !ActiveEffect.bPeriodic)
	{
		return;
	}

	if (FTimerHandle* Existing = EffectPeriodTimers.Find(ActiveEffect.EffectId))
	{
		GetWorld()->GetTimerManager().ClearTimer(*Existing);
	}

	FTimerHandle NewHandle;
	GetWorld()->GetTimerManager().SetTimer(
		NewHandle,
		FTimerDelegate::CreateUObject(this, &UCussAbilityComponent::HandleEffectPeriod, ActiveEffect.EffectId),
		ActiveEffect.Period,
		true);

	EffectPeriodTimers.Add(ActiveEffect.EffectId, NewHandle);
}

