// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void ReceivedPlayer() override;		// Called when net connection is created.
	
public:
	void UpdatePlayerHealth(float Health, float MaxHealth);
	void UpdatePlayerScore(float Value);
	void UpdatePlayerDefeats(int32 Value);
	void UpdateWeaponAmmo(int32 AmmoAmount);
	void UpdateCarriedAmmo(int32 AmmoAmount);
	void UpdateWeaponType(const FString& WeaponType);
	void UpdateMatchCountDown(float Countdown);
	void SetHUDTime();
	void RefreshHUD();

private:
	UPROPERTY()
	class ABlasterHUD* BlasterHUD;

	/**
	 *	Sync
	 */
	UFUNCTION(Server, Reliable)
	void RequestServerTimeFromClient(float ClientRequestTime);

	UFUNCTION(Client, Reliable)
	void ReportServerTimeToClient(float ClientRequestTime, float ServerReportTime);

	void CheckTimeSync(float DeltaTime);

	FORCEINLINE float GetServerTime() const { return HasAuthority() ? GetWorld()->GetTimeSeconds() : GetWorld()->GetTimeSeconds() + SyncDiffTime; }

	UPROPERTY(EditAnywhere, Category = Sync)
	float SyncFreq = 5.f;
	
	float SyncDiffTime = 0.f;
	float SyncRunningTime = 0.f;
	
	/**
	 * Match
	 */ 
	float MatchTime = 130.f;
	int32 CountdownInt = 0;
	
};
