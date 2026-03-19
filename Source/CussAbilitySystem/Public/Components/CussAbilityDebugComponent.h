//Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/CussAbilityDebugTypes.h"
#include "CussAbilityDebugComponent.generated.h"

class APlayerController;
class UCussAbilityComponent;
class UCussAbilityDebugPanelWidget;
class UCussDebugCombatLogWidget;
class UCussDebugHotbarWidget;
class UCussDebugValidationMessageWidget;

/** Thin local-only presentation bridge that observes the ability runtime and drives the Phase 4 debug UI. */
UCLASS(ClassGroup=(Cuss), meta=(BlueprintSpawnableComponent))
class CUSSABILITYSYSTEM_API UCussAbilityDebugComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Disables ticking and seeds default native widget classes for the debug presentation layer. */
	UCussAbilityDebugComponent();

protected:
	/** Starts the local-only debug initialization flow for the owning pawn. */
	virtual void BeginPlay() override;
	/** Clears timers, removes widgets, and unbinds runtime delegates when the owner is torn down. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Creates the local debug widgets and begins periodic snapshot refreshes once the owning player is ready. */
	void InitializeDebugUI();
	/** Finds the ability component on the owner and binds the debug-facing delegates. */
	bool BindAbilityComponent();
	/** Refreshes the hotbar widget from the latest slot snapshot. */
	void RefreshHotbar();
	/** Refreshes the active-effect widget from the latest effect snapshot. */
	void RefreshEffects();
	/** Refreshes the owned-tag widget from the latest tag snapshot. */
	void RefreshTags();
	/** Refreshes the time-based debug widgets that need periodic countdown updates. */
	void RefreshTimedViews();
	/** Receives a runtime activation result and forwards it into the validation and hotbar presentation. */
	void HandleActivationResult(const FCussAbilityActivationResultView& ActivationResult);
	/** Receives a runtime combat log entry and forwards it into the local combat log widget. */
	void HandleCombatLogEntry(const FCussCombatLogEntry& CombatLogEntry);
	/** Displays the latest activation result message in the transient validation widget. */
	void ShowValidationMessage(const FCussAbilityActivationResultView& ActivationResult);
	/** Clears the transient validation message after its short display window expires. */
	void ClearValidationMessage();
	/** Returns the local player controller that owns this debug presentation, if one exists. */
	APlayerController* ResolveOwningPlayerController() const;
	/** Returns whether this component should create debug UI for its current owner on this machine. */
	bool ShouldCreateLocalDebugUI() const;

	UPROPERTY()
	TObjectPtr<UCussAbilityComponent> AbilityComponent = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Cuss|Debug")
	TSubclassOf<UCussDebugHotbarWidget> HotbarWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="Cuss|Debug")
	TSubclassOf<UCussAbilityDebugPanelWidget> DebugPanelWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="Cuss|Debug")
	TSubclassOf<UCussDebugCombatLogWidget> CombatLogWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="Cuss|Debug")
	TSubclassOf<UCussDebugValidationMessageWidget> ValidationMessageWidgetClass;

	UPROPERTY()
	TObjectPtr<UCussDebugHotbarWidget> HotbarWidget = nullptr;

	UPROPERTY()
	TObjectPtr<UCussAbilityDebugPanelWidget> DebugPanelWidget = nullptr;

	UPROPERTY()
	TObjectPtr<UCussDebugCombatLogWidget> CombatLogWidget = nullptr;

	UPROPERTY()
	TObjectPtr<UCussDebugValidationMessageWidget> ValidationMessageWidget = nullptr;

	TArray<FCussCombatLogEntry> CombatLogEntries;
	FTimerHandle InitializeTimerHandle;
	FTimerHandle RefreshTimerHandle;
	FTimerHandle ValidationMessageTimerHandle;
	bool bDebugUIInitialized = false;
};
