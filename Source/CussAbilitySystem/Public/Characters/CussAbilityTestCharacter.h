//Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "CussAbilityTestCharacter.generated.h"

class UCussAbilityComponent;
class UCussStatComponent;
class UInputAction;
class UInputMappingContext;

/** Simple playable test character that wires input into the ability and stat components. */
UCLASS()
class CUSSABILITYSYSTEM_API ACussAbilityTestCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	/** Creates the default ability and stat components used by the test pawn. */
	ACussAbilityTestCharacter();

protected:
	/** Initializes default stats on the authority and applies the input mapping context for local players. */
	virtual void BeginPlay() override;
	/** Binds the four configured input actions to their matching ability slots. */
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Cuss|Components")
	TObjectPtr<UCussAbilityComponent> AbilityComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Cuss|Components")
	TObjectPtr<UCussStatComponent> StatComponent;

	UPROPERTY(EditDefaultsOnly, Category="Cuss|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category="Cuss|Input")
	TObjectPtr<UInputAction> Ability1Action;

	UPROPERTY(EditDefaultsOnly, Category="Cuss|Input")
	TObjectPtr<UInputAction> Ability2Action;

	UPROPERTY(EditDefaultsOnly, Category="Cuss|Input")
	TObjectPtr<UInputAction> Ability3Action;

	UPROPERTY(EditDefaultsOnly, Category="Cuss|Input")
	TObjectPtr<UInputAction> Ability4Action;

	UFUNCTION()
	/** Tries to activate the primary ability input slot. */
	void HandleAbility1Pressed();

	UFUNCTION()
	/** Tries to activate the secondary ability input slot. */
	void HandleAbility2Pressed();

	UFUNCTION()
	/** Tries to activate the utility ability input slot. */
	void HandleAbility3Pressed();

	UFUNCTION()
	/** Tries to activate the ultimate ability input slot. */
	void HandleAbility4Pressed();

	/** Resolves an input tag into an ability activation request on the ability component. */
	void TryActivateInputTag(const FGameplayTag& InputTag);
	/** Seeds the test character with baseline health, mana, and energy values. */
	void InitializeDefaultStats();
	
	bool TraceAbilityTarget(AActor*& OutTargetActor, FVector& OutTargetLocation) const;
};
