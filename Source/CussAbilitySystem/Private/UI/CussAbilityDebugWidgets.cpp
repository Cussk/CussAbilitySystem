//Copyright Kyle Cuss and Cuss Programming 2026.

#include "UI/CussAbilityDebugWidgets.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
FText GetSlotText(const FCussAbilitySlotView& SlotView)
{
	const FText SlotLabel = FText::FromString(FString::Printf(TEXT("%d"), SlotView.SlotIndex + 1));
	const FText AbilityName = SlotView.bIsGranted
		? SlotView.AbilityName
		: FText::FromString(TEXT("Empty"));

	if (!SlotView.bIsGranted)
	{
		return FText::Format(NSLOCTEXT("CussAbilityDebug", "EmptySlot", "[{0}] {1}"), SlotLabel, AbilityName);
	}

	if (SlotView.bIsBlocked)
	{
		return FText::Format(NSLOCTEXT("CussAbilityDebug", "BlockedSlot", "[{0}] {1}\nBlocked"), SlotLabel, AbilityName);
	}

	if (SlotView.bIsOnCooldown)
	{
		return FText::Format(NSLOCTEXT("CussAbilityDebug", "CooldownSlot", "[{0}] {1}\nCD {2}s"), SlotLabel, AbilityName, FText::AsNumber(SlotView.CooldownRemaining));
	}

	return FText::Format(NSLOCTEXT("CussAbilityDebug", "ReadySlot", "[{0}] {1}\nReady"), SlotLabel, AbilityName);
}

FLinearColor GetSlotColor(const FCussAbilitySlotView& SlotView)
{
	if (!SlotView.bIsGranted)
	{
		return FLinearColor(0.08f, 0.08f, 0.08f, 0.9f);
	}

	if (SlotView.bIsBlocked)
	{
		return FLinearColor(0.35f, 0.12f, 0.12f, 0.95f);
	}

	if (SlotView.bIsOnCooldown)
	{
		return FLinearColor(0.12f, 0.12f, 0.3f, 0.95f);
	}

	return FLinearColor(0.12f, 0.26f, 0.16f, 0.95f);
}

FText GetCombatLogTypeLabel(ECussCombatLogEntryType EntryType)
{
	switch (EntryType)
	{
	case ECussCombatLogEntryType::Activation:
		return FText::FromString(TEXT("ACT"));
	case ECussCombatLogEntryType::Validation:
		return FText::FromString(TEXT("VAL"));
	case ECussCombatLogEntryType::Delivery:
		return FText::FromString(TEXT("DEL"));
	case ECussCombatLogEntryType::Effect:
		return FText::FromString(TEXT("FX"));
	case ECussCombatLogEntryType::Cooldown:
		return FText::FromString(TEXT("CD"));
	case ECussCombatLogEntryType::Info:
	default:
		return FText::FromString(TEXT("INFO"));
	}
}

void ConfigurePanelBorder(UBorder* Border, const FLinearColor& Color)
{
	if (!Border)
	{
		return;
	}

	Border->SetPadding(FMargin(10.f));
	Border->SetBrushColor(Color);
}
}

void UCussDebugHotbarWidget::SetSlotViews(const TArray<FCussAbilitySlotView>& InSlotViews)
{
	SlotViews = InSlotViews;
	RefreshDisplay();
}

void UCussDebugHotbarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		WidgetTree->RootWidget = RootCanvas;

		UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HotbarBorder"));
		ConfigurePanelBorder(PanelBorder, FLinearColor(0.03f, 0.03f, 0.03f, 0.8f));

		UCanvasPanelSlot* BorderSlot = RootCanvas->AddChildToCanvas(PanelBorder);
		BorderSlot->SetAnchors(FAnchors(0.5f, 1.f));
		BorderSlot->SetAlignment(FVector2D(0.5f, 1.f));
		BorderSlot->SetAutoSize(true);
		BorderSlot->SetPosition(FVector2D(0.f, -28.f));

		SlotBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SlotBox"));
		PanelBorder->SetContent(SlotBox);
	}

	RefreshDisplay();
}

void UCussDebugHotbarWidget::RefreshDisplay()
{
	if (!SlotBox)
	{
		return;
	}

	SlotBox->ClearChildren();

	for (const FCussAbilitySlotView& SlotView : SlotViews)
	{
		UBorder* SlotBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		ConfigurePanelBorder(SlotBorder, GetSlotColor(SlotView));

		UTextBlock* SlotText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		SlotText->SetJustification(ETextJustify::Center);
		SlotText->SetText(GetSlotText(SlotView));
		SlotBorder->SetContent(SlotText);

		if (UHorizontalBoxSlot* HotbarSlot = SlotBox->AddChildToHorizontalBox(SlotBorder))
		{
			HotbarSlot->SetPadding(FMargin(4.f, 0.f));
		}
	}
}

void UCussDebugCombatLogWidget::SetLogEntries(const TArray<FCussCombatLogEntry>& InLogEntries)
{
	LogEntries = InLogEntries;
	RefreshDisplay();
}

void UCussDebugCombatLogWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		WidgetTree->RootWidget = RootCanvas;

		UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CombatLogBorder"));
		ConfigurePanelBorder(PanelBorder, FLinearColor(0.03f, 0.03f, 0.03f, 0.8f));

		UCanvasPanelSlot* BorderSlot = RootCanvas->AddChildToCanvas(PanelBorder);
		BorderSlot->SetAnchors(FAnchors(0.f, 0.f));
		BorderSlot->SetAlignment(FVector2D(0.f, 0.f));
		BorderSlot->SetAutoSize(true);
		BorderSlot->SetPosition(FVector2D(20.f, 140.f));

		UVerticalBox* ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CombatLogContent"));
		PanelBorder->SetContent(ContentBox);

		UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CombatLogTitle"));
		TitleText->SetText(FText::FromString(TEXT("Combat Log")));
		ContentBox->AddChildToVerticalBox(TitleText);

		LogTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CombatLogText"));
		LogTextBlock->SetText(FText::FromString(TEXT("No entries yet.")));
		if (UVerticalBoxSlot* LogSlot = ContentBox->AddChildToVerticalBox(LogTextBlock))
		{
			LogSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
		}
	}

	RefreshDisplay();
}

void UCussDebugCombatLogWidget::RefreshDisplay()
{
	if (!LogTextBlock)
	{
		return;
	}

	if (LogEntries.IsEmpty())
	{
		LogTextBlock->SetText(FText::FromString(TEXT("No entries yet.")));
		return;
	}

	FString CombinedText;
	const int32 StartIndex = FMath::Max(0, LogEntries.Num() - 10);

	for (int32 Index = StartIndex; Index < LogEntries.Num(); ++Index)
	{
		const FCussCombatLogEntry& Entry = LogEntries[Index];
		CombinedText += FString::Printf(TEXT("[%.1f] %s %s"), Entry.WorldTimeSeconds, *GetCombatLogTypeLabel(Entry.Type).ToString(), *Entry.Message.ToString());

		if (Index < LogEntries.Num() - 1)
		{
			CombinedText += TEXT("\n");
		}
	}

	LogTextBlock->SetText(FText::FromString(CombinedText));
}

void UCussAbilityDebugPanelWidget::SetActiveEffectViews(const TArray<FCussActiveEffectView>& InActiveEffects)
{
	ActiveEffectViews = InActiveEffects;
	RefreshEffects();
}

void UCussAbilityDebugPanelWidget::SetOwnedTags(const FGameplayTagContainer& InOwnedTags)
{
	OwnedTags = InOwnedTags;
	RefreshTags();
}

void UCussAbilityDebugPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		WidgetTree->RootWidget = RootCanvas;

		UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DebugPanelBorder"));
		ConfigurePanelBorder(PanelBorder, FLinearColor(0.03f, 0.03f, 0.03f, 0.8f));

		UCanvasPanelSlot* BorderSlot = RootCanvas->AddChildToCanvas(PanelBorder);
		BorderSlot->SetAnchors(FAnchors(1.f, 0.f));
		BorderSlot->SetAlignment(FVector2D(1.f, 0.f));
		BorderSlot->SetAutoSize(true);
		BorderSlot->SetPosition(FVector2D(-20.f, 140.f));

		UVerticalBox* ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DebugPanelContent"));
		PanelBorder->SetContent(ContentBox);

		UTextBlock* EffectsTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EffectsTitle"));
		EffectsTitle->SetText(FText::FromString(TEXT("Active Effects")));
		ContentBox->AddChildToVerticalBox(EffectsTitle);

		ActiveEffectsTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EffectsText"));
		ActiveEffectsTextBlock->SetText(FText::FromString(TEXT("None")));
		if (UVerticalBoxSlot* EffectsSlot = ContentBox->AddChildToVerticalBox(ActiveEffectsTextBlock))
		{
			EffectsSlot->SetPadding(FMargin(0.f, 6.f, 0.f, 12.f));
		}

		UTextBlock* TagsTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TagsTitle"));
		TagsTitle->SetText(FText::FromString(TEXT("Owned Tags")));
		ContentBox->AddChildToVerticalBox(TagsTitle);

		OwnedTagsTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TagsText"));
		OwnedTagsTextBlock->SetText(FText::FromString(TEXT("None")));
		if (UVerticalBoxSlot* TagsSlot = ContentBox->AddChildToVerticalBox(OwnedTagsTextBlock))
		{
			TagsSlot->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
		}
	}

	RefreshEffects();
	RefreshTags();
}

void UCussAbilityDebugPanelWidget::RefreshEffects()
{
	if (!ActiveEffectsTextBlock)
	{
		return;
	}

	if (ActiveEffectViews.IsEmpty())
	{
		ActiveEffectsTextBlock->SetText(FText::FromString(TEXT("None")));
		return;
	}

	FString CombinedText;

	for (int32 Index = 0; Index < ActiveEffectViews.Num(); ++Index)
	{
		const FCussActiveEffectView& EffectView = ActiveEffectViews[Index];
		const FString DurationText = EffectView.bInfiniteDuration
			? TEXT("Inf")
			: FString::Printf(TEXT("%.1fs"), EffectView.TimeRemaining);

		CombinedText += FString::Printf(TEXT("%s x%d (%s)"), *EffectView.EffectName.ToString(), EffectView.StackCount, *DurationText);

		if (!EffectView.SourceAbilityName.IsEmpty())
		{
			CombinedText += FString::Printf(TEXT(" <- %s"), *EffectView.SourceAbilityName.ToString());
		}

		if (Index < ActiveEffectViews.Num() - 1)
		{
			CombinedText += TEXT("\n");
		}
	}

	ActiveEffectsTextBlock->SetText(FText::FromString(CombinedText));
}

void UCussAbilityDebugPanelWidget::RefreshTags()
{
	if (!OwnedTagsTextBlock)
	{
		return;
	}

	TArray<FGameplayTag> SortedTags;
	OwnedTags.GetGameplayTagArray(SortedTags);
	SortedTags.Sort(
		[](const FGameplayTag& Left, const FGameplayTag& Right)
		{
			return Left.ToString() < Right.ToString();
		});

	if (SortedTags.IsEmpty())
	{
		OwnedTagsTextBlock->SetText(FText::FromString(TEXT("None")));
		return;
	}

	FString CombinedText;

	for (int32 Index = 0; Index < SortedTags.Num(); ++Index)
	{
		CombinedText += SortedTags[Index].ToString();

		if (Index < SortedTags.Num() - 1)
		{
			CombinedText += TEXT("\n");
		}
	}

	OwnedTagsTextBlock->SetText(FText::FromString(CombinedText));
}

void UCussDebugValidationMessageWidget::ShowActivationResult(const FCussAbilityActivationResultView& ActivationResult)
{
	if (!MessageBorder || !MessageTextBlock)
	{
		return;
	}

	MessageTextBlock->SetText(ActivationResult.Message);
	MessageBorder->SetBrushColor(
		ActivationResult.bSuccess
			? FLinearColor(0.1f, 0.28f, 0.14f, 0.95f)
			: FLinearColor(0.4f, 0.08f, 0.08f, 0.95f));
	SetVisibility(ESlateVisibility::Visible);
}

void UCussDebugValidationMessageWidget::ClearMessage()
{
	if (MessageTextBlock)
	{
		MessageTextBlock->SetText(FText::GetEmpty());
	}

	SetVisibility(ESlateVisibility::Collapsed);
}

void UCussDebugValidationMessageWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		WidgetTree->RootWidget = RootCanvas;

		MessageBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MessageBorder"));
		ConfigurePanelBorder(MessageBorder, FLinearColor(0.1f, 0.1f, 0.1f, 0.9f));

		UCanvasPanelSlot* BorderSlot = RootCanvas->AddChildToCanvas(MessageBorder);
		BorderSlot->SetAnchors(FAnchors(0.5f, 1.f));
		BorderSlot->SetAlignment(FVector2D(0.5f, 1.f));
		BorderSlot->SetAutoSize(true);
		BorderSlot->SetPosition(FVector2D(0.f, -130.f));

		MessageTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MessageText"));
		MessageTextBlock->SetJustification(ETextJustify::Center);
		MessageBorder->SetContent(MessageTextBlock);
	}

	ClearMessage();
}
