//Copyright Kyle Cuss and Cuss Programming 2026.

#include "Characters/CussAbilityTestCharacter.h"

#include "Components/CussAbilityComponent.h"
#include "Components/CussStatComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "CoreTypes/CussAbilityGameplayTags.h"
#include "GameFramework/PlayerController.h"

ACussAbilityTestCharacter::ACussAbilityTestCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	AbilityComponent = CreateDefaultSubobject<UCussAbilityComponent>(TEXT("AbilityComponent"));
	StatComponent = CreateDefaultSubobject<UCussStatComponent>(TEXT("StatComponent"));
}

void ACussAbilityTestCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		InitializeDefaultStats();
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				if (DefaultMappingContext)
				{
					InputSubsystem->AddMappingContext(DefaultMappingContext, 0);
				}
			}
		}
	}
}

void ACussAbilityTestCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput)
	{
		return;
	}

	if (Ability1Action)
	{
		EnhancedInput->BindAction(Ability1Action, ETriggerEvent::Started, this, &ACussAbilityTestCharacter::HandleAbility1Pressed);
	}

	if (Ability2Action)
	{
		EnhancedInput->BindAction(Ability2Action, ETriggerEvent::Started, this, &ACussAbilityTestCharacter::HandleAbility2Pressed);
	}

	if (Ability3Action)
	{
		EnhancedInput->BindAction(Ability3Action, ETriggerEvent::Started, this, &ACussAbilityTestCharacter::HandleAbility3Pressed);
	}

	if (Ability4Action)
	{
		EnhancedInput->BindAction(Ability4Action, ETriggerEvent::Started, this, &ACussAbilityTestCharacter::HandleAbility4Pressed);
	}
}

void ACussAbilityTestCharacter::HandleAbility1Pressed()
{
	TryActivateInputTag(CussAbilityTags::TAG_Input_Ability_Primary);
}

void ACussAbilityTestCharacter::HandleAbility2Pressed()
{
	TryActivateInputTag(CussAbilityTags::TAG_Input_Ability_Secondary);
}

void ACussAbilityTestCharacter::HandleAbility3Pressed()
{
	TryActivateInputTag(CussAbilityTags::TAG_Input_Ability_Utility);
}

void ACussAbilityTestCharacter::HandleAbility4Pressed()
{
	TryActivateInputTag(CussAbilityTags::TAG_Input_Ability_Ultimate);
}

void ACussAbilityTestCharacter::TryActivateInputTag(const FGameplayTag& InputTag)
{
	if (!AbilityComponent)
	{
		return;
	}

	AActor* TargetActor = nullptr;
	const FVector TargetLocation = FVector::ZeroVector;

	AbilityComponent->TryActivateAbilityByInputTag(InputTag, TargetActor, TargetLocation);
}

void ACussAbilityTestCharacter::InitializeDefaultStats()
{
	if (!StatComponent)
	{
		return;
	}

	StatComponent->InitializeStat(CussAbilityTags::TAG_Stat_Health, 100.f, 100.f);
	StatComponent->InitializeStat(CussAbilityTags::TAG_Stat_Mana, 100.f, 100.f);
	StatComponent->InitializeStat(CussAbilityTags::TAG_Stat_Energy, 100.f, 100.f);
}