//Copyright Kyle Cuss and Cuss Programming 2026.

#include "Actors/CussAbilityProjectile.h"

#include "Components/CussAbilityComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"

ACussAbilityProjectile::ACussAbilityProjectile()
{
	bReplicates = true;
	AActor::SetReplicateMovement(true);
	PrimaryActorTick.bCanEverTick = false;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);
	CollisionComponent->InitSphereRadius(16.f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComponent->SetGenerateOverlapEvents(true);
	CollisionComponent->SetNotifyRigidBodyCollision(true);
	CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &ACussAbilityProjectile::HandleCollisionOverlap);
	CollisionComponent->OnComponentHit.AddDynamic(this, &ACussAbilityProjectile::HandleCollisionHit);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->UpdatedComponent = CollisionComponent;
	ProjectileMovementComponent->InitialSpeed = 0.f;
	ProjectileMovementComponent->MaxSpeed = 0.f;
	ProjectileMovementComponent->ProjectileGravityScale = 0.f;
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->bShouldBounce = false;
}

void ACussAbilityProjectile::InitializeProjectile(const FCussAbilityProjectileSpawnContext& InSpawnContext, const FCussProjectileDeliveryDef& InDeliveryDef)
{
	SpawnContext = InSpawnContext;
	DeliveryDefinition = InDeliveryDef;
	bHasResolvedImpact = false;
	IgnoredActors.Reset();

	const float CollisionRadius = FMath::Max(1.f, DeliveryDefinition.CollisionRadius);
	CollisionComponent->SetSphereRadius(CollisionRadius);

	const FVector LaunchDirection = SpawnContext.LaunchDirection.GetSafeNormal();
	const float InitialSpeed = FMath::Max(0.f, DeliveryDefinition.InitialSpeed);
	ProjectileMovementComponent->InitialSpeed = InitialSpeed;
	ProjectileMovementComponent->MaxSpeed = InitialSpeed;
	ProjectileMovementComponent->Velocity = LaunchDirection * InitialSpeed;

	if (DeliveryDefinition.MaxLifetime > 0.f)
	{
		SetLifeSpan(DeliveryDefinition.MaxLifetime);
	}

	IgnoreSourceActor();
}

void ACussAbilityProjectile::BeginPlay()
{
	Super::BeginPlay();

	IgnoreSourceActor();
}

void ACussAbilityProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACussAbilityProjectile, SpawnContext);
	DOREPLIFETIME(ACussAbilityProjectile, DeliveryDefinition);
}

void ACussAbilityProjectile::HandleCollisionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || bHasResolvedImpact || !OtherActor || ShouldIgnoreActor(OtherActor))
	{
		return;
	}

	const FVector ImpactLocation = !SweepResult.ImpactPoint.IsNearlyZero()
		? FVector(SweepResult.ImpactPoint)
		: OtherActor->GetActorLocation();

	ResolveImpact(OtherActor, ImpactLocation, false);
}

void ACussAbilityProjectile::HandleCollisionHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority() || bHasResolvedImpact)
	{
		return;
	}

	if (OtherActor && !ShouldIgnoreActor(OtherActor))
	{
		const FVector ImpactLocation = !Hit.ImpactPoint.IsNearlyZero()
			? FVector(Hit.ImpactPoint)
			: OtherActor->GetActorLocation();

		ResolveImpact(OtherActor, ImpactLocation, false);
		return;
	}

	const FVector ImpactLocation = !Hit.ImpactPoint.IsNearlyZero()
		? FVector(Hit.ImpactPoint)
		: GetActorLocation();

	ResolveImpact(nullptr, ImpactLocation, true);
}

void ACussAbilityProjectile::ResolveImpact(AActor* ImpactActor, const FVector& ImpactLocation, bool bWorldHit)
{
	if (!HasAuthority() || bHasResolvedImpact || !SpawnContext.SourceActor)
	{
		return;
	}

	bHasResolvedImpact = true;

	if (UCussAbilityComponent* SourceAbilityComponent = SpawnContext.SourceActor->FindComponentByClass<UCussAbilityComponent>())
	{
		SourceAbilityComponent->HandleProjectileImpact(SpawnContext, ImpactActor, ImpactLocation);
	}

	StopOrDestroyAfterImpact(bWorldHit);
}

void ACussAbilityProjectile::IgnoreSourceActor()
{
	if (!CollisionComponent || !SpawnContext.SourceActor)
	{
		return;
	}

	CollisionComponent->IgnoreActorWhenMoving(SpawnContext.SourceActor, true);
	IgnoredActors.Add(SpawnContext.SourceActor);
}

bool ACussAbilityProjectile::ShouldIgnoreActor(const AActor* OtherActor) const
{
	if (!OtherActor || OtherActor == this)
	{
		return true;
	}

	for (const TWeakObjectPtr<AActor>& IgnoredActor : IgnoredActors)
	{
		if (IgnoredActor.Get() == OtherActor)
		{
			return true;
		}
	}

	return false;
}

void ACussAbilityProjectile::StopOrDestroyAfterImpact(bool bWorldHit)
{
	if (DeliveryDefinition.bDestroyOnHit)
	{
		Destroy();
		return;
	}

	if (!bWorldHit || DeliveryDefinition.bStopOnWorldHit)
	{
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ProjectileMovementComponent->StopMovementImmediately();
		ProjectileMovementComponent->Deactivate();
	}
}
