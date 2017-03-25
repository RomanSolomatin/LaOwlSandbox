// Fill out your copyright notice in the Description page of Project Settings.

#include "FPDev.h"
#include "Runtime/Engine/Classes/Kismet/KismetMathLibrary.h"
#include "Animation/AnimInstance.h"
#include "WeaponMechanic.h"
#include "FPDevProjectile.h"
#include "ShipPawn.h"


AShipPawn::AShipPawn()  {

	SetupPawnView();

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;
	BaseImpulseRate = 100.0f;
	MaxEngineImpulse = 2000.f;
	MinEngineImpulse = 100.f;

	Health = 100.0f;

	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
}

void AShipPawn::SetupPawnView()
{
	MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("Muzzle"));
	MuzzleLocation->SetupAttachment(GetPawnUsedView());
	MuzzleLocation->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
}

UStaticMeshComponent* AShipPawn::GetPawnUsedView()
{
	return Super::GetPawnUsedView();
}

void AShipPawn::BeginPlay() {
	Super::BeginPlay();

	WeaponFunction = NewObject<UWeaponMechanic>();

	TimeSinceFire = 0.0f;

	FireQueue.Init(true, 0);
	TimeSinceBulletSpawn = 0.0f;

	EngineImpulse = MinEngineImpulse;

	TriggerHeld = false;
}

void AShipPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const FVector LocalMove = FVector(EngineImpulse * DeltaTime, 0.f, 0.f);

	// Move plan forwards (with sweep so we stop when we collide with things)
	AddActorLocalOffset(LocalMove, true);

	FireWeapon(DeltaTime);

	if (TriggerHeld && WeaponFunction->FullAutomatic) {
		OnFire();
	}

}

void AShipPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AShipPawn::OnFire()
{
	ActivateWeapon();
}

void AShipPawn::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AShipPawn::PitchAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AShipPawn::ModifyEngineImpluse(float Rate)
{

	EngineImpulse += Rate * BaseImpulseRate * GetWorld()->GetDeltaSeconds();
	FMath::Clamp(EngineImpulse, MinEngineImpulse, MaxEngineImpulse);

}

bool AShipPawn::ActivateWeapon()
{
	if (TimeSinceFire > WeaponFunction->FireDelay) {
		for (int32 i = 0; i < WeaponFunction->ShotMultiplier; ++i) {
			FireQueue.Add(true);
		}
		TimeSinceFire = 0.0;
		return true;
	}
	return false;
}

void AShipPawn::ChangeWeaponMechanicClass(UClass * NewMechanicType)
{
	WeaponFunction = NewObject<UWeaponMechanic>(this, NewMechanicType);
}

void AShipPawn::SetWeaponFunction(UWeaponMechanic * NewWeaponFunction)
{
	WeaponFunction = NewWeaponFunction;
}

void AShipPawn::FireWeapon(float DeltaTime)
{
	if (FireQueue.Num() > 0 && TimeSinceBulletSpawn > WeaponFunction->MultiplierDelay) {
		const FRotator SpawnRotation = GetControlRotation();

		// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
		const FVector SpawnLocation = ((MuzzleLocation != nullptr) ? MuzzleLocation->GetComponentLocation() : GetActorLocation()) + SpawnRotation.RotateVector(FVector(200.0f, 0.0f, 0.0f));
		//const FVector SpawnLocation = GetActorLocation() + SpawnRotation.RotateVector(WeaponComponent->GunOffset);

		if (ProjectileClass != NULL) {

			UWorld* const World = GetWorld();

			if (World != NULL)
			{

				//todo get sound and animation
				/*if (WeaponComponent != NULL) {
				// try and play the sound if specified
				if (WeaponComponent->HasActivationSound())
				{
				UGameplayStatics::PlaySoundAtLocation(this, WeaponComponent->GetActivationSound(), GetActorLocation());
				}

				// try and play a firing animation if specified
				if (WeaponComponent->HasActivationAnimation())
				{
				// Get the animation object for the arms mesh
				UAnimInstance* AnimInstance = GetPawnUsedMesh()->GetAnimInstance();
				if (AnimInstance != NULL)
				{
				AnimInstance->Montage_Play(WeaponComponent->GetActivationAnimation(), 1.f);
				}
				}
				}*/

				//Set Spawn Collision Handling Override
				FActorSpawnParameters ActorSpawnParams;
				ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

				
				float cellDim = WeaponFunction->GetSpreadCellDim();
				float offSetW = (float(WeaponFunction->SpreadWidth) / 2.0f);
				float offSetH = (float(WeaponFunction->GetSpreadHeight()) / 2.0f);
				FVector Forward = UKismetMathLibrary::GetForwardVector(SpawnRotation);
				FVector Up = UKismetMathLibrary::GetUpVector(SpawnRotation);
				FVector Right = UKismetMathLibrary::GetRightVector(SpawnRotation);
				FVector Dist = WeaponFunction->SpreadDepth * Forward;
				FVector inheritedVelocity = Forward * EngineImpulse;
				int32 i = 0;
				//for each element or cell in the SpreadPattern array.
				for (auto& cell : WeaponFunction->SpreadPattern) {
					if (cell) {
						//transform Pattern index to x and y coordinates
						float x = (i%WeaponFunction->SpreadWidth) - offSetW;
						float y = (i / WeaponFunction->SpreadWidth) - offSetH;

						//use x, y, and SpreadDepth to transform the projectile's spawn point to the projectile's target point.
						FVector p = FVector(SpawnLocation.X, SpawnLocation.Y, SpawnLocation.Z);
						p += Dist;
						p += ((y * cellDim) + (0.5f * cellDim)) * Up;
						p += ((x * cellDim) + (0.5f * cellDim)) * Right;

						//get the direction vector normalized.
						p = p - SpawnLocation;
						p.Normalize();

						//spawn a projectile at the weapon barrel which is rotated to point at it's SpreadPattern target point.
						AFPDevProjectile* projectile = World->SpawnActor<AFPDevProjectile>(ProjectileClass,
							SpawnLocation,
							p.ToOrientationRotator(), ActorSpawnParams);
						if (projectile) {
							projectile->AddVelocity(inheritedVelocity);
						}
					}

					//next cell.
					++i;
				}


				FireQueue.RemoveAt(0);
				TimeSinceBulletSpawn = 0.0f;
			}
		}
	}
	else {
		TimeSinceBulletSpawn += DeltaTime;
		TimeSinceFire += DeltaTime;
	}
}
