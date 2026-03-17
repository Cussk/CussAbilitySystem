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

	PlayerInputComponent->BindKey(EKeys::One, IE_Pressed, this, &ACussAbilityTestCharacter::HandleAbility1Pressed);
	PlayerInputComponent->BindKey(EKeys::Two, IE_Pressed, this, &ACussAbilityTestCharacter::HandleAbility2Pressed);
	PlayerInputComponent->BindKey(EKeys::Three, IE_Pressed, this, &ACussAbilityTestCharacter::HandleAbility3Pressed);
	PlayerInputComponent->BindKey(EKeys::Four, IE_Pressed, this, &ACussAbilityTestCharacter::HandleAbility4Pressed);
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
	
	UE_LOG(LogTemp, Log, TEXT("Trying ability: %s"), *InputTag.ToString());

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