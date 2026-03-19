//Copyright Kyle Cuss and Cuss Programming 2026.

#include "Components/CussAbilityDebugComponent.h"

#include "Characters/CussAbilityTestCharacter.h"
#include "Components/CussAbilityComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "UI/CussAbilityDebugWidgets.h"

UCussAbilityDebugComponent::UCussAbilityDebugComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	HotbarWidgetClass = UCussDebugHotbarWidget::StaticClass();
	DebugPanelWidgetClass = UCussAbilityDebugPanelWidget::StaticClass();
	CombatLogWidgetClass = UCussDebugCombatLogWidget::StaticClass();
	ValidationMessageWidgetClass = UCussDebugValidationMessageWidget::StaticClass();
}

void UCussAbilityDebugComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetWorld())
	{
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(InitializeTimerHandle, this, &UCussAbilityDebugComponent::InitializeDebugUI, 0.2f, true, 0.0f);
}

void UCussAbilityDebugComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(InitializeTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(RefreshTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(ValidationMessageTimerHandle);
	}

	if (AbilityComponent)
	{
		AbilityComponent->OnAbilityActivationResult.RemoveAll(this);
		AbilityComponent->OnAbilitySlotsChanged.RemoveAll(this);
		AbilityComponent->OnActiveEffectsChanged.RemoveAll(this);
		AbilityComponent->OnOwnedTagsChanged.RemoveAll(this);
		AbilityComponent->OnCombatLogEntryAdded.RemoveAll(this);
	}

	if (HotbarWidget)
	{
		HotbarWidget->RemoveFromParent();
	}

	if (DebugPanelWidget)
	{
		DebugPanelWidget->RemoveFromParent();
	}

	if (CombatLogWidget)
	{
		CombatLogWidget->RemoveFromParent();
	}

	if (ValidationMessageWidget)
	{
		ValidationMessageWidget->RemoveFromParent();
	}

	Super::EndPlay(EndPlayReason);
}

void UCussAbilityDebugComponent::InitializeDebugUI()
{
	if (bDebugUIInitialized || !ShouldCreateLocalDebugUI())
	{
		return;
	}

	APlayerController* PlayerController = ResolveOwningPlayerController();
	if (!PlayerController || !BindAbilityComponent())
	{
		return;
	}

	if (!HotbarWidget && HotbarWidgetClass)
	{
		HotbarWidget = CreateWidget<UCussDebugHotbarWidget>(PlayerController, HotbarWidgetClass);
		if (HotbarWidget)
		{
			HotbarWidget->AddToViewport(5);
		}
	}

	if (!DebugPanelWidget && DebugPanelWidgetClass)
	{
		DebugPanelWidget = CreateWidget<UCussAbilityDebugPanelWidget>(PlayerController, DebugPanelWidgetClass);
		if (DebugPanelWidget)
		{
			DebugPanelWidget->AddToViewport(4);
		}
	}

	if (!CombatLogWidget && CombatLogWidgetClass)
	{
		CombatLogWidget = CreateWidget<UCussDebugCombatLogWidget>(PlayerController, CombatLogWidgetClass);
		if (CombatLogWidget)
		{
			CombatLogWidget->AddToViewport(4);
		}
	}

	if (!ValidationMessageWidget && ValidationMessageWidgetClass)
	{
		ValidationMessageWidget = CreateWidget<UCussDebugValidationMessageWidget>(PlayerController, ValidationMessageWidgetClass);
		if (ValidationMessageWidget)
		{
			ValidationMessageWidget->AddToViewport(6);
		}
	}

	RefreshHotbar();
	RefreshEffects();
	RefreshTags();

	if (CombatLogWidget)
	{
		CombatLogWidget->SetLogEntries(CombatLogEntries);
	}

	GetWorld()->GetTimerManager().ClearTimer(InitializeTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(RefreshTimerHandle, this, &UCussAbilityDebugComponent::RefreshTimedViews, 0.15f, true);
	bDebugUIInitialized = true;
}

bool UCussAbilityDebugComponent::BindAbilityComponent()
{
	if (!GetOwner())
	{
		return false;
	}

	UCussAbilityComponent* NewAbilityComponent = GetOwner()->FindComponentByClass<UCussAbilityComponent>();
	if (!NewAbilityComponent)
	{
		return false;
	}

	if (AbilityComponent == NewAbilityComponent)
	{
		return true;
	}

	if (AbilityComponent)
	{
		AbilityComponent->OnAbilityActivationResult.RemoveAll(this);
		AbilityComponent->OnAbilitySlotsChanged.RemoveAll(this);
		AbilityComponent->OnActiveEffectsChanged.RemoveAll(this);
		AbilityComponent->OnOwnedTagsChanged.RemoveAll(this);
		AbilityComponent->OnCombatLogEntryAdded.RemoveAll(this);
	}

	AbilityComponent = NewAbilityComponent;
	AbilityComponent->OnAbilityActivationResult.AddUObject(this, &UCussAbilityDebugComponent::HandleActivationResult);
	AbilityComponent->OnAbilitySlotsChanged.AddUObject(this, &UCussAbilityDebugComponent::RefreshHotbar);
	AbilityComponent->OnActiveEffectsChanged.AddUObject(this, &UCussAbilityDebugComponent::RefreshEffects);
	AbilityComponent->OnOwnedTagsChanged.AddUObject(this, &UCussAbilityDebugComponent::RefreshTags);
	AbilityComponent->OnOwnedTagsChanged.AddUObject(this, &UCussAbilityDebugComponent::RefreshHotbar);
	AbilityComponent->OnCombatLogEntryAdded.AddUObject(this, &UCussAbilityDebugComponent::HandleCombatLogEntry);
	return true;
}

void UCussAbilityDebugComponent::RefreshHotbar()
{
	if (!AbilityComponent || !HotbarWidget)
	{
		return;
	}

	TArray<FCussAbilitySlotView> SlotViews;
	AbilityComponent->GetAbilitySlotViews(SlotViews);
	HotbarWidget->SetSlotViews(SlotViews);
}

void UCussAbilityDebugComponent::RefreshEffects()
{
	if (!AbilityComponent || !DebugPanelWidget)
	{
		return;
	}

	TArray<FCussActiveEffectView> EffectViews;
	AbilityComponent->GetActiveEffectViews(EffectViews);
	DebugPanelWidget->SetActiveEffectViews(EffectViews);
}

void UCussAbilityDebugComponent::RefreshTags()
{
	if (!AbilityComponent || !DebugPanelWidget)
	{
		return;
	}

	FGameplayTagContainer OwnedTags;
	AbilityComponent->GetOwnedTags(OwnedTags);
	DebugPanelWidget->SetOwnedTags(OwnedTags);
}

void UCussAbilityDebugComponent::RefreshTimedViews()
{
	RefreshHotbar();
	RefreshEffects();
}

void UCussAbilityDebugComponent::HandleActivationResult(const FCussAbilityActivationResultView& ActivationResult)
{
	ShowValidationMessage(ActivationResult);
	RefreshHotbar();
}

void UCussAbilityDebugComponent::HandleCombatLogEntry(const FCussCombatLogEntry& CombatLogEntry)
{
	CombatLogEntries.Add(CombatLogEntry);

	while (CombatLogEntries.Num() > 10)
	{
		CombatLogEntries.RemoveAt(0);
	}

	if (CombatLogWidget)
	{
		CombatLogWidget->SetLogEntries(CombatLogEntries);
	}
}

void UCussAbilityDebugComponent::ShowValidationMessage(const FCussAbilityActivationResultView& ActivationResult)
{
	if (!ValidationMessageWidget || ActivationResult.Message.IsEmpty())
	{
		return;
	}

	ValidationMessageWidget->ShowActivationResult(ActivationResult);

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ValidationMessageTimerHandle);
		GetWorld()->GetTimerManager().SetTimer(ValidationMessageTimerHandle, this, &UCussAbilityDebugComponent::ClearValidationMessage, 1.8f, false);
	}
}

void UCussAbilityDebugComponent::ClearValidationMessage()
{
	if (ValidationMessageWidget)
	{
		ValidationMessageWidget->ClearMessage();
	}
}

APlayerController* UCussAbilityDebugComponent::ResolveOwningPlayerController() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
}

bool UCussAbilityDebugComponent::ShouldCreateLocalDebugUI() const
{
	if (Cast<ACussAbilityTestCharacter>(GetOwner()) == nullptr)
	{
		return false;
	}

	const APlayerController* PlayerController = ResolveOwningPlayerController();
	return PlayerController && PlayerController->IsLocalController();
}
