//Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/CussAbilityDebugTypes.h"
#include "CussAbilityDebugWidgets.generated.h"

class UBorder;
class UHorizontalBox;
class UTextBlock;

/** Minimal native hotbar widget that displays granted slots, cooldown state, and blocked state. */
UCLASS()
class CUSSABILITYSYSTEM_API UCussDebugHotbarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Copies the latest ability slot snapshot into the widget and rebuilds the hotbar display. */
	void SetSlotViews(const TArray<FCussAbilitySlotView>& InSlotViews);

protected:
	/** Builds the native widget tree the first time the widget is constructed. */
	virtual void NativeConstruct() override;

	/** Rebuilds the visible slot list from the cached slot snapshot. */
	void RefreshDisplay();

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> SlotBox = nullptr;

	TArray<FCussAbilitySlotView> SlotViews;
};

/** Minimal native combat log widget that displays the latest debug log lines for the local player. */
UCLASS()
class CUSSABILITYSYSTEM_API UCussDebugCombatLogWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Copies the latest combat log snapshot into the widget and refreshes the visible text. */
	void SetLogEntries(const TArray<FCussCombatLogEntry>& InLogEntries);

protected:
	/** Builds the native widget tree the first time the widget is constructed. */
	virtual void NativeConstruct() override;

	/** Rebuilds the visible combat log text from the cached entries. */
	void RefreshDisplay();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LogTextBlock = nullptr;

	TArray<FCussCombatLogEntry> LogEntries;
};

/** Minimal native debug panel widget that displays active effects and owned tags for the local player. */
UCLASS()
class CUSSABILITYSYSTEM_API UCussAbilityDebugPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Copies the latest active-effect snapshot into the widget and refreshes the effect list. */
	void SetActiveEffectViews(const TArray<FCussActiveEffectView>& InActiveEffects);
	/** Copies the latest owned-tag snapshot into the widget and refreshes the tag list. */
	void SetOwnedTags(const FGameplayTagContainer& InOwnedTags);

protected:
	/** Builds the native widget tree the first time the widget is constructed. */
	virtual void NativeConstruct() override;

	/** Rebuilds the visible active-effect text from the cached snapshot. */
	void RefreshEffects();
	/** Rebuilds the visible owned-tag text from the cached snapshot. */
	void RefreshTags();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ActiveEffectsTextBlock = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> OwnedTagsTextBlock = nullptr;

	TArray<FCussActiveEffectView> ActiveEffectViews;
	FGameplayTagContainer OwnedTags;
};

/** Minimal native validation widget that displays the latest activation result for the local player. */
UCLASS()
class CUSSABILITYSYSTEM_API UCussDebugValidationMessageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Displays the supplied activation result using a success or failure tint. */
	void ShowActivationResult(const FCussAbilityActivationResultView& ActivationResult);
	/** Clears the current validation message and collapses the widget. */
	void ClearMessage();

protected:
	/** Builds the native widget tree the first time the widget is constructed. */
	virtual void NativeConstruct() override;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> MessageBorder = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MessageTextBlock = nullptr;
};
