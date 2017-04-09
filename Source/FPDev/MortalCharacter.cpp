/*
This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.
Attribution-NonCommercial-ShareAlike 4.0 International
An EscapeVelocity Production (Nate Gillard).
*/

#include "FPDev.h"
#include "Runtime/Engine/Classes/Kismet/KismetMathLibrary.h"
#include "Animation/AnimInstance.h"
#include "WeaponComponent.h"
#include "WeaponMechanic.h"
#include "FPDevProjectile.h"
#include "MortalCharacter.h"



// Sets default values
AMortalCharacter::AMortalCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

void AMortalCharacter::SetupCharacterView()
{
}

USkeletalMeshComponent * AMortalCharacter::GetCharaterUsedMesh()
{
	return GetMesh();
}

// Called when the game starts or when spawned
void AMortalCharacter::BeginPlay()
{
	Super::BeginPlay();

	WeaponFunction = NewObject<UWeaponMechanic>();

	TimeSinceFire = 0.0f;

	FireQueue.Init(true, 0);
	TimeSinceBulletSpawn = 0.0f;

	TriggerHeld = false;
	
}

// Called every frame
void AMortalCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsHealthDepleated()) {
		HealthDepleated();
	}

	FireWeapon(DeltaTime);

	if (TriggerHeld && WeaponFunction->FullAutomatic) {
		OnFire();
	}

}

bool AMortalCharacter::AttachWeapon(UClass* ComponentClass)
{
	return false;
}

void AMortalCharacter::FireWeapon(float DeltaTime) {
	if (FireQueue.Num() > 0 && TimeSinceBulletSpawn > WeaponFunction->MultiplierDelay) {
		const FRotator SpawnRotation = GetControlRotation();

		// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
		const FVector SpawnLocation = ((FP_MuzzleLocation != nullptr) ? FP_MuzzleLocation->GetComponentLocation() : GetActorLocation()) + SpawnRotation.RotateVector(FVector(100.0f, 0.0f, 0.0f));
		//const FVector SpawnLocation = GetActorLocation() + SpawnRotation.RotateVector(WeaponComponent->GunOffset);

		if (ProjectileClass != NULL) {

			UWorld* const World = GetWorld();

			if (World != NULL)
			{

				if (WeaponComponent != NULL) {
					// try and play the sound if specified
					if (WeaponComponent->HasActivationSound())
					{
						UGameplayStatics::PlaySoundAtLocation(this, WeaponComponent->GetActivationSound(), GetActorLocation());
					}

					// try and play a firing animation if specified
					if (WeaponComponent->HasActivationAnimation())
					{
						// Get the animation object for the arms mesh
						UAnimInstance* AnimInstance = GetCharaterUsedMesh()->GetAnimInstance();
						if (AnimInstance != NULL)
						{
							AnimInstance->Montage_Play(WeaponComponent->GetActivationAnimation(), 1.f);
						}
					}
				}
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
						World->SpawnActor<AFPDevProjectile>(ProjectileClass,
							SpawnLocation,
							p.ToOrientationRotator(), ActorSpawnParams);
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

bool AMortalCharacter::ActivateWeapon() {
	// try and fire a projectile
	if (WeaponComponent != NULL) {
		if (TimeSinceFire > WeaponFunction->FireDelay) {
			for (int32 i = 0; i < WeaponFunction->ShotMultiplier; ++i) {
				FireQueue.Add(true);
			}
			TimeSinceFire = 0.0;
			return true;
		}
	}
	return false;
}

// respond to fire input
void AMortalCharacter::OnFire()
{
	ActivateWeapon();
}

void AMortalCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AMortalCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AMortalCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AMortalCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AMortalCharacter::SetWeaponFunction(UWeaponMechanic* NewWeaponFunction) {
	WeaponFunction = NewWeaponFunction;
}

void AMortalCharacter::ChangeWeaponMechanicClass(UClass* NewMechanicType) {
	WeaponFunction = NewObject<UWeaponMechanic>(this, NewMechanicType);
}

//check is health 0.0 or less.
bool AMortalCharacter::IsHealthDepleated_Implementation() const {
	return !(Health > 0.0);
}

void AMortalCharacter::HealthDepleated_Implementation() {
	Destroy();
}

float AMortalCharacter::TakeDamage_Implementation(float DamageAmount, struct FDamageEvent const & DamageEvent, class AController * EventInstigator, AActor * DamageCauser) {
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	Health -= DamageAmount;

	return DamageAmount;
}