#pragma once

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	EWT_AssaultRifle UMETA(DisplayName = "Assault Rifle"),
	EWT_RocketLauncher UMETA(DisplayName = "Rocket Launcher"),
	EWT_HitScan UMETA(DisplayName = "Hit Scan"),
	EWT_MAX UMETA(DisplayName = "Max")
};
