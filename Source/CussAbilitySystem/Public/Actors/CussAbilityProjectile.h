//Copyright Kyle Cuss and Cuss Programming 2026.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoreTypes/CussAbilityTypes.h"
#include "CussAbilityProjectile.generated.h"

class UPrimitiveComponent;
class UProjectileMovementComponent;
class USphereComponent;

/** Reusable delivery actor that carries explicit runtime ability context until it impacts. */
UCLASS()
class CUSSABILITYSYSTEM_API ACussAbilityProjectile : public AActor
{
	GENERATED_BODY()

public:
	/** Enables replication, creates collision and movement components, and disables ticking. */
	ACussAbilityProjectile();

	/** Copies the explicit runtime payload and authored delivery settings onto this projectile instance. */
	void InitializeProjectile(const FCussAbilityProjectileSpawnContext& InSpawnContext, const FCussProjectileDeliveryDef& InDeliveryDef);

	/** Returns the inspectable runtime payload driving this projectile instance. */
	const FCussAbilityProjectileSpawnContext& GetSpawnContext() const { return SpawnContext; }

	/** Returns the inspectable authored delivery settings applied to this projectile instance. */
	const FCussProjectileDeliveryDef& GetDeliveryDefinition() const { return DeliveryDefinition; }

protected:
	/** Re-applies source collision ignores after spawning on the authority instance. */
	virtual void BeginPlay() override;
	/** Replicates the runtime payload and delivery settings for remote inspection. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Projectile")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category="Projectile")
	FCussAbilityProjectileSpawnContext SpawnContext;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category="Projectile")
	FCussProjectileDeliveryDef DeliveryDefinition;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Projectile")
	bool bHasResolvedImpact = false;

	UFUNCTION()
	/** Handles authority-side overlap impacts against valid actor targets. */
	void HandleCollisionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	/** Handles blocking hits against either actor targets or world geometry. */
	void HandleCollisionHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	/** Marks the projectile resolved and routes the impact through the source ability component's existing effect path. */
	void ResolveImpact(AActor* ImpactActor, const FVector& ImpactLocation, bool bWorldHit);
	/** Adds the source actor to the projectile's ignore list and movement ignore set. */
	void IgnoreSourceActor();
	/** Returns whether the projectile should ignore the supplied actor for collision resolution. */
	bool ShouldIgnoreActor(const AActor* OtherActor) const;
	/** Applies single-impact cleanup by destroying or stopping the projectile after resolution. */
	void StopOrDestroyAfterImpact(bool bWorldHit);

	TArray<TWeakObjectPtr<AActor>> IgnoredActors;
};
