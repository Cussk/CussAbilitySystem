//Copyright Kyle Cuss and Cuss Programming 2026.

#include "Characters/CussAbilityTestCharacter.h"

#include "Components/CussAbilityComponent.h"
#include "Components/CussAbilityDebugComponent.h"
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
	AbilityDebugComponent = CreateDefaultSubobject<UCussAbilityDebugComponent>(TEXT("AbilityDebugComponent"));
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
	FVector TargetLocation = FVector::ZeroVector;

	TraceAbilityTarget(TargetActor, TargetLocation);

	AbilityComponent->TryActivateAbilityByInputTag(InputTag, TargetActor, TargetLocation);
}

bool ACussAbilityTestCharacter::TraceAbilityTarget(AActor*& OutTargetActor, FVector& OutTargetLocation) const
{
	OutTargetActor = nullptr;
	OutTargetLocation = FVector::ZeroVector;

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !GetWorld())
	{
		return false;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	PC->GetPlayerViewPoint(ViewLocation, ViewRotation);

	const FVector TraceEnd = ViewLocation + (ViewRotation.Vector() * 5000.f);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CussAbilityTargetTrace), false);
	QueryParams.AddIgnoredActor(this);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, ViewLocation, TraceEnd, ECC_Visibility, QueryParams);
	
	DrawDebugLine(GetWorld(), ViewLocation, TraceEnd, bHit ? FColor::Green : FColor::Red, false, 1.0f, 0, 1.5f);

	if (bHit)
	{
		DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 12.f, 12, FColor::Green, false, 1.0f);
	}
	
	if (!bHit)
	{
		OutTargetLocation = TraceEnd;
		return false;
	}

	OutTargetActor = HitResult.GetActor();
	OutTargetLocation = HitResult.ImpactPoint;
	return true;
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
