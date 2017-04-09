/*
This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.
Attribution-NonCommercial-ShareAlike 4.0 International
An EscapeVelocity Production (Nate Gillard).

This code based on work under
Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
*/

#include "FPDev.h"
#include "FPDevProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"

AFPDevProjectile::AFPDevProjectile() : ImpactVelocityTransferScale(100.0f)
{
	// Use a sphere as a simple collision representation
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	//CollisionComp->InitSphereRadius(5.0f);
	CollisionComp->BodyInstance.SetCollisionProfileName("Projectile");
	CollisionComp->OnComponentHit.AddDynamic(this, &AFPDevProjectile::OnHit);		// set up a notification for when this component hits something blocking

	// Players can't walk on it
	CollisionComp->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
	CollisionComp->CanCharacterStepUpOn = ECB_No;

	// Set as root component
	RootComponent = CollisionComp;

	ProjectileMovement = InitProjectileMovementComponent();

	// Die after 3 seconds by default
	InitialLifeSpan = 3.0f;

	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

void AFPDevProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Only add impulse and destroy projectile if we hit a physics
	if ((OtherActor != NULL) && (OtherActor != this) && (OtherComp != NULL) && OtherComp->IsSimulatingPhysics())
	{
		OtherComp->AddImpulseAtLocation(GetVelocity() * ImpactVelocityTransferScale, GetActorLocation());

		Destroy();
	}
}

UProjectileMovementComponent*  AFPDevProjectile::InitProjectileMovementComponent_Implementation() {
	// Use a ProjectileMovementComponent to govern this projectile's movement
	UProjectileMovementComponent* PMC = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	PMC->UpdatedComponent = CollisionComp;

	return PMC;
}

float AFPDevProjectile::GetDamage_Implementation() const{
	return 10.0;
}

void AFPDevProjectile::AddVelocity(FVector Delta)
{
	GetProjectileMovement()->Velocity = GetVelocity() + Delta;
}
