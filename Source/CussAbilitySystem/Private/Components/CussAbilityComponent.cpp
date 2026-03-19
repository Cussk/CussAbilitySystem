//Copyright Kyle Cuss and Cuss Programming 2026.

#include "Components/CussAbilityComponent.h"

#include "Actors/CussAbilityProjectile.h"
#include "Components/CussStatComponent.h"
#include "CoreTypes/CussAbilityGameplayTags.h"
#include "Data/CussAbilitySetData.h"
#include "Data/CussAbilityData.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

namespace
{
FText GetGameplayTagDisplayText(FGameplayTag Tag)
{
	if (!Tag.IsValid())
	{
		return FText::GetEmpty();
	}

	FString TagString = Tag.ToString();
	FString Left;
	FString Right;

	if (TagString.Split(TEXT("."), &Left, &Right, ESearchCase::CaseSensitive, ESearchDir::FromEnd))
	{
		return FText::FromString(Right);
	}

	return FText::FromString(TagString);
}

FText GetAbilityDisplayText(const UCussAbilityData* AbilityData, FGameplayTag AbilityTag)
{
	if (AbilityData && !AbilityData->DisplayName.IsEmpty())
	{
		return AbilityData->DisplayName;
	}

	return GetGameplayTagDisplayText(AbilityTag);
}

FText GetActorDisplayText(const AActor* Actor)
{
	return Actor ? FText::FromString(Actor->GetName()) : FText::FromString(TEXT("World"));
}

FText GetEffectDisplayText(const FCussAbilityEffectDef& EffectDef, const UCussAbilityData* SourceAbilityData, FGameplayTag AbilityTag)
{
	if (EffectDef.GrantedTag.IsValid())
	{
		return GetGameplayTagDisplayText(EffectDef.GrantedTag);
	}

	switch (EffectDef.EffectType)
	{
	case ECussAbilityEffectType::Damage:
		return FText::FromString(TEXT("Damage"));
	case ECussAbilityEffectType::Heal:
		return FText::FromString(TEXT("Heal"));
	case ECussAbilityEffectType::ModifyStat:
		return EffectDef.AffectedStatTag.IsValid()
			? GetGameplayTagDisplayText(EffectDef.AffectedStatTag)
			: FText::FromString(TEXT("Stat Effect"));
	case ECussAbilityEffectType::ApplyTag:
		return EffectDef.GrantedTag.IsValid()
			? GetGameplayTagDisplayText(EffectDef.GrantedTag)
			: FText::FromString(TEXT("Tag Applied"));
	case ECussAbilityEffectType::RemoveTag:
		return EffectDef.GrantedTag.IsValid()
			? GetGameplayTagDisplayText(EffectDef.GrantedTag)
			: FText::FromString(TEXT("Tag Removed"));
	case ECussAbilityEffectType::None:
	default:
		return GetAbilityDisplayText(SourceAbilityData, AbilityTag);
	}
}

int32 GetSlotIndexForInputTag(FGameplayTag InputTag)
{
	if (InputTag == CussAbilityTags::TAG_Input_Ability_Primary)
	{
		return 0;
	}

	if (InputTag == CussAbilityTags::TAG_Input_Ability_Secondary)
	{
		return 1;
	}

	if (InputTag == CussAbilityTags::TAG_Input_Ability_Utility)
	{
		return 2;
	}

	if (InputTag == CussAbilityTags::TAG_Input_Ability_Ultimate)
	{
		return 3;
	}

	return INDEX_NONE;
}

FText BuildActivationMessage(const UCussAbilityData* AbilityData, FGameplayTag AbilityTag, bool bSuccess, ECussAbilityActivationFailReason FailReason, FGameplayTag RelatedTag)
{
	const FText AbilityName = GetAbilityDisplayText(AbilityData, AbilityTag);

	if (bSuccess)
	{
		return FText::Format(NSLOCTEXT("CussAbility", "ActivationSucceeded", "{0} activated"), AbilityName);
	}

	switch (FailReason)
	{
	case ECussAbilityActivationFailReason::NotGranted:
		return FText::Format(NSLOCTEXT("CussAbility", "ActivationNotGranted", "{0} failed: Not Granted"), AbilityName);
	case ECussAbilityActivationFailReason::OnCooldown:
		return FText::Format(NSLOCTEXT("CussAbility", "ActivationOnCooldown", "{0} failed: On Cooldown"), AbilityName);
	case ECussAbilityActivationFailReason::InsufficientResources:
		return RelatedTag.IsValid()
			? FText::Format(NSLOCTEXT("CussAbility", "ActivationMissingResource", "{0} failed: Missing {1}"), AbilityName, GetGameplayTagDisplayText(RelatedTag))
			: FText::Format(NSLOCTEXT("CussAbility", "ActivationMissingResources", "{0} failed: Missing Resources"), AbilityName);
	case ECussAbilityActivationFailReason::BlockedByTags:
		return RelatedTag.IsValid()
			? FText::Format(NSLOCTEXT("CussAbility", "ActivationBlockedTag", "{0} failed: Blocked by tag {1}"), AbilityName, GetGameplayTagDisplayText(RelatedTag))
			: FText::Format(NSLOCTEXT("CussAbility", "ActivationBlockedTags", "{0} failed: Blocked by Tags"), AbilityName);
	case ECussAbilityActivationFailReason::MissingRequiredTags:
		return RelatedTag.IsValid()
			? FText::Format(NSLOCTEXT("CussAbility", "ActivationMissingTag", "{0} failed: Missing tag {1}"), AbilityName, GetGameplayTagDisplayText(RelatedTag))
			: FText::Format(NSLOCTEXT("CussAbility", "ActivationMissingTags", "{0} failed: Missing Required Tags"), AbilityName);
	case ECussAbilityActivationFailReason::InvalidTarget:
		return FText::Format(NSLOCTEXT("CussAbility", "ActivationInvalidTarget", "{0} failed: Invalid Target"), AbilityName);
	case ECussAbilityActivationFailReason::OutOfRange:
		return FText::Format(NSLOCTEXT("CussAbility", "ActivationOutOfRange", "{0} failed: Out Of Range"), AbilityName);
	case ECussAbilityActivationFailReason::InvalidContext:
		return FText::Format(NSLOCTEXT("CussAbility", "ActivationInvalidContext", "{0} failed: Invalid Context"), AbilityName);
	case ECussAbilityActivationFailReason::Unknown:
	case ECussAbilityActivationFailReason::None:
	default:
		return FText::Format(NSLOCTEXT("CussAbility", "ActivationUnknownFailure", "{0} failed"), AbilityName);
	}
}

FText BuildCooldownLogMessage(const UCussAbilityData* AbilityData)
{
	return FText::Format(NSLOCTEXT("CussAbility", "CooldownStarted", "{0} cooldown started"), GetAbilityDisplayText(AbilityData, AbilityData ? AbilityData->AbilityTag : FGameplayTag()));
}

FText BuildProjectileSpawnLogMessage(const UCussAbilityData* AbilityData)
{
	return FText::Format(NSLOCTEXT("CussAbility", "ProjectileSpawned", "{0} projectile spawned"), GetAbilityDisplayText(AbilityData, AbilityData ? AbilityData->AbilityTag : FGameplayTag()));
}

FText BuildProjectileImpactLogMessage(const UCussAbilityData* AbilityData, AActor* ImpactActor)
{
	return FText::Format(NSLOCTEXT("CussAbility", "ProjectileHit", "{0} hit {1}"), GetAbilityDisplayText(AbilityData, AbilityData ? AbilityData->AbilityTag : FGameplayTag()), GetActorDisplayText(ImpactActor));
}

FText BuildEffectLogMessage(const UCussAbilityData* AbilityData, const FCussAbilityEffectDef& EffectDef, AActor* TargetActor)
{
	const FText EffectName = GetEffectDisplayText(EffectDef, AbilityData, AbilityData ? AbilityData->AbilityTag : FGameplayTag());

	if (EffectDef.Duration > 0.f)
	{
		return FText::Format(NSLOCTEXT("CussAbility", "EffectAppliedDuration", "{0} applied x1 for {1}s"), EffectName, FText::AsNumber(EffectDef.Duration));
	}

	switch (EffectDef.EffectType)
	{
	case ECussAbilityEffectType::Damage:
		return FText::Format(NSLOCTEXT("CussAbility", "DamageApplied", "{0} damage applied to {1}"), GetAbilityDisplayText(AbilityData, AbilityData ? AbilityData->AbilityTag : FGameplayTag()), GetActorDisplayText(TargetActor));
	case ECussAbilityEffectType::Heal:
		return FText::Format(NSLOCTEXT("CussAbility", "HealApplied", "{0} heal applied to {1}"), GetAbilityDisplayText(AbilityData, AbilityData ? AbilityData->AbilityTag : FGameplayTag()), GetActorDisplayText(TargetActor));
	case ECussAbilityEffectType::ModifyStat:
		return FText::Format(NSLOCTEXT("CussAbility", "StatApplied", "{0} applied to {1}"), EffectName, GetActorDisplayText(TargetActor));
	case ECussAbilityEffectType::ApplyTag:
		return FText::Format(NSLOCTEXT("CussAbility", "TagApplied", "{0} applied to {1}"), EffectName, GetActorDisplayText(TargetActor));
	case ECussAbilityEffectType::RemoveTag:
		return FText::Format(NSLOCTEXT("CussAbility", "TagRemoved", "{0} removed from {1}"), EffectName, GetActorDisplayText(TargetActor));
	case ECussAbilityEffectType::None:
	default:
		return FText::Format(NSLOCTEXT("CussAbility", "EffectApplied", "{0} applied"), EffectName);
	}
}

FText BuildActiveEffectRemovedLogMessage(const FCussActiveEffect& ActiveEffect, bool bExpired)
{
	FGameplayTag DisplayTag;
	TArray<FGameplayTag> GrantedTags;
	ActiveEffect.GrantedTags.GetGameplayTagArray(GrantedTags);

	if (!GrantedTags.IsEmpty())
	{
		DisplayTag = GrantedTags[0];
	}
	else
	{
		DisplayTag = ActiveEffect.AbilityTag;
	}

	const FText EffectName = GetGameplayTagDisplayText(DisplayTag).IsEmpty()
		? GetAbilityDisplayText(ActiveEffect.SourceAbilityData, ActiveEffect.AbilityTag)
		: GetGameplayTagDisplayText(DisplayTag);

	return bExpired
		? FText::Format(NSLOCTEXT("CussAbility", "EffectExpired", "{0} expired"), EffectName)
		: FText::Format(NSLOCTEXT("CussAbility", "EffectRemoved", "{0} removed"), EffectName);
}
}

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
	NotifyAbilitySlotsChanged();
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
		const FCussAbilityActivationResultView ActivationResult = BuildActivationResultView(
			nullptr,
			AbilityTag,
			false,
			ECussAbilityActivationFailReason::NotGranted,
			FGameplayTag(),
			0.f,
			BuildActivationMessage(nullptr, AbilityTag, false, ECussAbilityActivationFailReason::NotGranted, FGameplayTag()));
		EmitActivationResult(ActivationResult);
		AddCombatLogEntry(ECussCombatLogEntryType::Validation, ActivationResult.Message);
		BroadcastEvent(GetOwner(), OptionalTargetActor, AbilityTag, ECussAbilityEventResult::Failed, 0.f, TargetLocation, FGameplayTag(), FGameplayTagContainer());
		return false;
	}

	if (!CanActivateAbility(*Spec, OptionalTargetActor, TargetLocation))
	{
		FGameplayTag RelatedTag;
		float CooldownRemaining = 0.f;
		const ECussAbilityActivationFailReason FailReason = DetermineActivationFailure(*Spec, OptionalTargetActor, TargetLocation, RelatedTag, CooldownRemaining);
		const FCussAbilityActivationResultView ActivationResult = BuildActivationResultView(
			Spec->AbilityData,
			AbilityTag,
			false,
			FailReason,
			RelatedTag,
			CooldownRemaining,
			BuildActivationMessage(Spec->AbilityData, AbilityTag, false, FailReason, RelatedTag));
		EmitActivationResult(ActivationResult);
		AddCombatLogEntry(ECussCombatLogEntryType::Validation, ActivationResult.Message);
		BroadcastEvent(GetOwner(), OptionalTargetActor, AbilityTag, ECussAbilityEventResult::Failed, 0.f, TargetLocation, FGameplayTag(), FGameplayTagContainer());
		return false;
	}

	ActivateAbilityInternal(*Spec, OptionalTargetActor, TargetLocation);
	const FCussAbilityActivationResultView ActivationResult = BuildActivationResultView(
		Spec->AbilityData,
		AbilityTag,
		true,
		ECussAbilityActivationFailReason::None,
		FGameplayTag(),
		0.f,
		BuildActivationMessage(Spec->AbilityData, AbilityTag, true, ECussAbilityActivationFailReason::None, FGameplayTag()));
	EmitActivationResult(ActivationResult);
	AddCombatLogEntry(ECussCombatLogEntryType::Activation, ActivationResult.Message);
	return true;
}

bool UCussAbilityComponent::TryActivateAbilityByInputTag(FGameplayTag InputTag, AActor* OptionalTargetActor, FVector TargetLocation)
{
	const FCussGrantedAbilitySpec* Spec = FindGrantedAbilityByInputTag(InputTag);
	if (!Spec || !Spec->AbilityData)
	{
		const FCussAbilityActivationResultView ActivationResult = BuildActivationResultView(
			nullptr,
			FGameplayTag(),
			false,
			ECussAbilityActivationFailReason::NotGranted,
			InputTag,
			0.f,
			FText::FromString(TEXT("Ability failed: Not Granted")));
		EmitActivationResult(ActivationResult);
		AddCombatLogEntry(ECussCombatLogEntryType::Validation, ActivationResult.Message);
		return false;
	}

	return TryActivateAbilityByTag(Spec->AbilityData->AbilityTag, OptionalTargetActor, TargetLocation);
}

void UCussAbilityComponent::ServerTryActivateAbility_Implementation(FGameplayTag AbilityTag, AActor* OptionalTargetActor, FVector_NetQuantize TargetLocation)
{
	TryActivateAbilityByTag(AbilityTag, OptionalTargetActor, TargetLocation);
}

void UCussAbilityComponent::OnRep_GrantedAbilities()
{
	NotifyAbilitySlotsChanged();
}

void UCussAbilityComponent::OnRep_OwnedTags()
{
	NotifyOwnedTagsChanged();
	NotifyAbilitySlotsChanged();
}

void UCussAbilityComponent::OnRep_ActiveEffects()
{
	NotifyActiveEffectsChanged();
}

void UCussAbilityComponent::OnRep_CooldownGroupStates()
{
	NotifyAbilitySlotsChanged();
}

void UCussAbilityComponent::ClientReceiveActivationResult_Implementation(const FCussAbilityActivationResultView& ActivationResult)
{
	OnAbilityActivationResult.Broadcast(ActivationResult);
}

void UCussAbilityComponent::ClientReceiveCombatLogEntry_Implementation(const FCussCombatLogEntry& CombatLogEntry)
{
	OnCombatLogEntryAdded.Broadcast(CombatLogEntry);
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

void UCussAbilityComponent::GetAbilitySlotViews(TArray<FCussAbilitySlotView>& OutViews) const
{
	OutViews.Reset();

	for (int32 SlotIndex = 0; SlotIndex < 4; ++SlotIndex)
	{
		FCussAbilitySlotView SlotView;
		SlotView.SlotIndex = SlotIndex;
		OutViews.Add(SlotView);
	}

	for (const FCussGrantedAbilitySpec& Spec : GrantedAbilities)
	{
		FCussAbilitySlotView SlotView;
		SlotView.SlotIndex = GetSlotIndexForInputTag(Spec.InputTag);
		SlotView.AbilityTag = Spec.AbilityData ? Spec.AbilityData->AbilityTag : FGameplayTag();
		SlotView.AbilityName = GetAbilityDisplayText(Spec.AbilityData, SlotView.AbilityTag);
		SlotView.bIsGranted = Spec.IsValid();
		SlotView.CooldownDuration = Spec.AbilityData ? Spec.AbilityData->CooldownSeconds : 0.f;
		SlotView.CooldownRemaining = SlotView.AbilityTag.IsValid() ? GetRemainingCooldown(SlotView.AbilityTag) : 0.f;
		SlotView.bIsOnCooldown = SlotView.CooldownRemaining > 0.f;
		SlotView.bIsBlocked = Spec.AbilityData && (!CheckOwnerTags(Spec.AbilityData) || !CheckDelivery(Spec.AbilityData));

		if (SlotView.SlotIndex != INDEX_NONE && OutViews.IsValidIndex(SlotView.SlotIndex))
		{
			OutViews[SlotView.SlotIndex] = SlotView;
			continue;
		}

		SlotView.SlotIndex = OutViews.Num();
		OutViews.Add(SlotView);
	}
}

void UCussAbilityComponent::GetActiveEffectViews(TArray<FCussActiveEffectView>& OutViews) const
{
	OutViews.Reset();

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	for (const FCussActiveEffect& ActiveEffect : ActiveEffects)
	{
		FCussActiveEffectView EffectView;
		TArray<FGameplayTag> GrantedTags;
		ActiveEffect.GrantedTags.GetGameplayTagArray(GrantedTags);
		EffectView.EffectTag = GrantedTags.IsEmpty() ? ActiveEffect.AbilityTag : GrantedTags[0];
		const FText EffectTagText = GetGameplayTagDisplayText(EffectView.EffectTag);
		EffectView.EffectName = EffectTagText.IsEmpty()
			? GetAbilityDisplayText(ActiveEffect.SourceAbilityData, ActiveEffect.AbilityTag)
			: EffectTagText;
		EffectView.StackCount = ActiveEffect.Stacks;
		EffectView.bInfiniteDuration = ActiveEffect.Duration <= 0.f;
		EffectView.TimeRemaining = EffectView.bInfiniteDuration ? 0.f : FMath::Max(0.f, ActiveEffect.EndTime - CurrentTime);
		EffectView.SourceAbilityName = GetAbilityDisplayText(ActiveEffect.SourceAbilityData, ActiveEffect.AbilityTag);
		OutViews.Add(EffectView);
	}

	OutViews.Sort(
		[](const FCussActiveEffectView& Left, const FCussActiveEffectView& Right)
		{
			return Left.TimeRemaining < Right.TimeRemaining;
		});
}

void UCussAbilityComponent::GetOwnedTags(FGameplayTagContainer& OutTags) const
{
	OutTags = OwnedTags;
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
		&& CheckDelivery(Spec.AbilityData)
		&& CheckCosts(Spec.AbilityData)
		&& CheckOwnerTags(Spec.AbilityData)
		&& ValidateTargeting(Spec.AbilityData, OptionalTargetActor, TargetLocation);
}

bool UCussAbilityComponent::CheckDelivery(UCussAbilityData* AbilityData) const
{
	if (!AbilityData)
	{
		return false;
	}

	if (AbilityData->DeliveryType != ECussAbilityDeliveryType::Projectile)
	{
		return true;
	}

	return GetWorld() && AbilityData->ProjectileDelivery.ProjectileClass != nullptr;
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

	if (Spec.AbilityData->DeliveryType == ECussAbilityDeliveryType::Projectile)
	{
		SpawnProjectileForAbility(Spec, OptionalTargetActor, TargetLocation);
		return;
	}

	ResolveAndApplyEffects(Spec.AbilityData, Spec.AbilityData->Effects, Spec.AbilityLevel, OptionalTargetActor, TargetLocation);
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

	NotifyAbilitySlotsChanged();
	AddCombatLogEntry(ECussCombatLogEntryType::Cooldown, BuildCooldownLogMessage(Spec.AbilityData));
}

void UCussAbilityComponent::HandleProjectileImpact(const FCussAbilityProjectileSpawnContext& ProjectileContext, AActor* ImpactActor, const FVector& ImpactLocation)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (!ProjectileContext.SourceAbilityData || ProjectileContext.SourceActor != GetOwner())
	{
		return;
	}

	AddCombatLogEntry(ECussCombatLogEntryType::Delivery, BuildProjectileImpactLogMessage(ProjectileContext.SourceAbilityData, ImpactActor));
	ResolveAndApplyEffects(ProjectileContext.SourceAbilityData, ProjectileContext.Effects, ProjectileContext.AbilityLevel, ImpactActor, ImpactLocation);
}

void UCussAbilityComponent::SpawnProjectileForAbility(const FCussGrantedAbilitySpec& Spec, AActor* OptionalTargetActor, const FVector& TargetLocation)
{
	if (!GetOwner() || !GetWorld() || !Spec.AbilityData)
	{
		return;
	}

	const FCussProjectileDeliveryDef& ProjectileDef = Spec.AbilityData->ProjectileDelivery;
	if (!ProjectileDef.ProjectileClass)
	{
		return;
	}

	const FVector SpawnLocation = GetOwner()->GetActorLocation();
	FVector LaunchDirection = (ResolveProjectileAimLocation(Spec.AbilityData, OptionalTargetActor, TargetLocation) - SpawnLocation).GetSafeNormal();
	if (LaunchDirection.IsNearlyZero())
	{
		LaunchDirection = GetOwner()->GetActorForwardVector().GetSafeNormal();
	}

	const FCussAbilityProjectileSpawnContext SpawnContext = BuildProjectileSpawnContext(Spec, SpawnLocation, LaunchDirection);
	const FTransform SpawnTransform(LaunchDirection.Rotation(), SpawnLocation);
	ACussAbilityProjectile* Projectile = GetWorld()->SpawnActorDeferred<ACussAbilityProjectile>(
		ProjectileDef.ProjectileClass,
		SpawnTransform,
		GetOwner(),
		GetOwner()->GetInstigator(),
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

	if (!Projectile)
	{
		return;
	}

	Projectile->InitializeProjectile(SpawnContext, ProjectileDef);
	Projectile->FinishSpawning(SpawnTransform);
	AddCombatLogEntry(ECussCombatLogEntryType::Delivery, BuildProjectileSpawnLogMessage(Spec.AbilityData));
}

FCussAbilityProjectileSpawnContext UCussAbilityComponent::BuildProjectileSpawnContext(const FCussGrantedAbilitySpec& Spec, const FVector& SpawnLocation, const FVector& LaunchDirection) const
{
	FCussAbilityProjectileSpawnContext SpawnContext;
	SpawnContext.SourceActor = GetOwner();
	SpawnContext.SourceAbilityData = Spec.AbilityData;
	SpawnContext.AbilityTag = Spec.AbilityData ? Spec.AbilityData->AbilityTag : FGameplayTag();
	SpawnContext.Effects = Spec.AbilityData ? Spec.AbilityData->Effects : TArray<FCussAbilityEffectDef>();
	SpawnContext.SpawnLocation = SpawnLocation;
	SpawnContext.LaunchDirection = LaunchDirection;
	SpawnContext.AbilityLevel = Spec.AbilityLevel;
	return SpawnContext;
}

FVector UCussAbilityComponent::ResolveProjectileAimLocation(const UCussAbilityData* AbilityData, AActor* OptionalTargetActor, const FVector& TargetLocation) const
{
	if (!GetOwner())
	{
		return TargetLocation;
	}

	if (AActor* PrimaryTarget = ResolvePrimaryTarget(AbilityData, OptionalTargetActor))
	{
		return PrimaryTarget->GetActorLocation();
	}

	if (!TargetLocation.IsNearlyZero())
	{
		return TargetLocation;
	}

	return GetOwner()->GetActorLocation() + (GetOwner()->GetActorForwardVector() * 1000.f);
}

ECussAbilityActivationFailReason UCussAbilityComponent::DetermineActivationFailure(const FCussGrantedAbilitySpec& Spec, AActor* OptionalTargetActor, const FVector& TargetLocation, FGameplayTag& OutRelatedTag, float& OutCooldownRemaining) const
{
	OutRelatedTag = FGameplayTag();
	OutCooldownRemaining = 0.f;

	if (!Spec.IsValid() || !Spec.AbilityData)
	{
		return ECussAbilityActivationFailReason::InvalidContext;
	}

	if (!CheckDelivery(Spec.AbilityData))
	{
		return ECussAbilityActivationFailReason::InvalidContext;
	}

	if (!CheckCooldown(Spec))
	{
		OutCooldownRemaining = GetRemainingCooldown(Spec.AbilityData->AbilityTag);
		OutRelatedTag = Spec.AbilityData->CooldownGroupTag;
		return ECussAbilityActivationFailReason::OnCooldown;
	}

	if (!StatComponent && !Spec.AbilityData->Costs.IsEmpty())
	{
		return ECussAbilityActivationFailReason::InvalidContext;
	}

	for (const FCussAbilityStatCost& Cost : Spec.AbilityData->Costs)
	{
		if (!StatComponent || !StatComponent->HasEnoughStat(Cost.StatTag, Cost.Amount))
		{
			OutRelatedTag = Cost.StatTag;
			return ECussAbilityActivationFailReason::InsufficientResources;
		}
	}

	for (const FGameplayTag& RequiredTag : Spec.AbilityData->RequiredOwnerTags)
	{
		if (!OwnedTags.HasTag(RequiredTag))
		{
			OutRelatedTag = RequiredTag;
			return ECussAbilityActivationFailReason::MissingRequiredTags;
		}
	}

	for (const FGameplayTag& BlockedTag : Spec.AbilityData->BlockedOwnerTags)
	{
		if (OwnedTags.HasTag(BlockedTag))
		{
			OutRelatedTag = BlockedTag;
			return ECussAbilityActivationFailReason::BlockedByTags;
		}
	}

	if (!ValidateTargeting(Spec.AbilityData, OptionalTargetActor, TargetLocation))
	{
		return ECussAbilityActivationFailReason::InvalidTarget;
	}

	return ECussAbilityActivationFailReason::Unknown;
}

FCussAbilityActivationResultView UCussAbilityComponent::BuildActivationResultView(const UCussAbilityData* AbilityData, FGameplayTag AbilityTag, bool bSuccess, ECussAbilityActivationFailReason FailReason, FGameplayTag RelatedTag, float CooldownRemaining, const FText& Message) const
{
	FCussAbilityActivationResultView ActivationResult;
	ActivationResult.AbilityTag = AbilityTag;
	ActivationResult.AbilityName = GetAbilityDisplayText(AbilityData, AbilityTag);
	ActivationResult.bSuccess = bSuccess;
	ActivationResult.FailReason = FailReason;
	ActivationResult.RelatedTag = RelatedTag;
	ActivationResult.CooldownRemaining = CooldownRemaining;
	ActivationResult.Message = Message;
	return ActivationResult;
}

void UCussAbilityComponent::ResolveAndApplyEffects(UCussAbilityData* AbilityData, const TArray<FCussAbilityEffectDef>& Effects, int32 AbilityLevel, AActor* OptionalTargetActor, const FVector& TargetLocation)
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

		for (const FCussAbilityEffectDef& EffectDef : Effects)
		{
			ApplyEffectToResolvedTarget(AbilityData, EffectDef, AbilityLevel, ResolvedTarget, TargetLocation);
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

void UCussAbilityComponent::ApplyEffectToResolvedTarget(UCussAbilityData* AbilityData, const FCussAbilityEffectDef& EffectDef, int32 AbilityLevel, AActor* ResolvedTarget, const FVector& TargetLocation)
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
	EffectContext.AbilityLevel = AbilityLevel;

	if (EffectDef.Duration > 0.f)
	{
		if (UCussAbilityComponent* TargetAbilityComp = GetAbilityComponent(ResolvedTarget))
		{
			TargetAbilityComp->AddActiveEffectFromContext(EffectContext, EffectDef);
			AddCombatLogEntry(ECussCombatLogEntryType::Effect, BuildEffectLogMessage(AbilityData, EffectDef, ResolvedTarget));
		}
		return;
	}

	ApplyInstantEffectFromContext(EffectContext, EffectDef);
	AddCombatLogEntry(ECussCombatLogEntryType::Effect, BuildEffectLogMessage(AbilityData, EffectDef, ResolvedTarget));
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

	if (EffectDef.EffectType == ECussAbilityEffectType::ModifyStat)
	{
		if (UCussStatComponent* Stats = GetStatComponent(GetOwner()))
		{
			Stats->ModifyStat(EffectDef.AffectedStatTag, NewEffect.Magnitude, true);
		}
	}

	ActiveEffects.Add(NewEffect);
	NotifyActiveEffectsChanged();

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

void UCussAbilityComponent::RemoveActiveEffectById(FGuid EffectId, bool bExpired)
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
	NotifyActiveEffectsChanged();
	AddCombatLogEntry(ECussCombatLogEntryType::Effect, BuildActiveEffectRemovedLogMessage(RemovedEffect, bExpired));

	BroadcastEvent(RemovedEffect.SourceActor.Get(), GetOwner(), RemovedEffect.AbilityTag, ECussAbilityEventResult::EffectRemoved, 0.f, GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector, FGameplayTag(), RemovedEffect.GrantedTags);
}

void UCussAbilityComponent::AddOwnedTag(FGameplayTag Tag)
{
	if (Tag.IsValid())
	{
		OwnedTags.AddTag(Tag);
		NotifyOwnedTagsChanged();
		NotifyAbilitySlotsChanged();
	}
}

void UCussAbilityComponent::RemoveOwnedTag(FGameplayTag Tag)
{
	if (Tag.IsValid())
	{
		OwnedTags.RemoveTag(Tag);
		NotifyOwnedTagsChanged();
		NotifyAbilitySlotsChanged();
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
	RemoveActiveEffectById(EffectId, true);
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

void UCussAbilityComponent::EmitActivationResult(const FCussAbilityActivationResultView& ActivationResult)
{
	OnAbilityActivationResult.Broadcast(ActivationResult);

	if (GetOwner() && GetOwner()->HasAuthority() && !GetOwner()->HasLocalNetOwner())
	{
		ClientReceiveActivationResult(ActivationResult);
	}
}

void UCussAbilityComponent::AddCombatLogEntry(ECussCombatLogEntryType EntryType, const FText& Message)
{
	if (Message.IsEmpty())
	{
		return;
	}

	FCussCombatLogEntry CombatLogEntry;
	CombatLogEntry.Type = EntryType;
	CombatLogEntry.Message = Message;
	CombatLogEntry.WorldTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	OnCombatLogEntryAdded.Broadcast(CombatLogEntry);

	if (GetOwner() && GetOwner()->HasAuthority() && !GetOwner()->HasLocalNetOwner())
	{
		ClientReceiveCombatLogEntry(CombatLogEntry);
	}
}

void UCussAbilityComponent::NotifyAbilitySlotsChanged()
{
	OnAbilitySlotsChanged.Broadcast();
}

void UCussAbilityComponent::NotifyActiveEffectsChanged()
{
	OnActiveEffectsChanged.Broadcast();
}

void UCussAbilityComponent::NotifyOwnedTagsChanged()
{
	OnOwnedTagsChanged.Broadcast();
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
	NotifyActiveEffectsChanged();
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

