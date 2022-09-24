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
	/** Called when net connection is created. */
	virtual void ReceivedPlayer() override;		
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
public:
	void UpdatePlayerHealth(float Health, float MaxHealth);
	void UpdatePlayerScore(float Value);
	void UpdatePlayerDefeats(int32 Value);
	void UpdateWeaponAmmo(int32 AmmoAmount);
	void UpdateCarriedAmmo(int32 AmmoAmount);
	void UpdateWeaponType(const FString& WeaponType);
	/** Update the warmup time before matching or update the cooldown time after the match has finished */
	void UpdateAnnouncement(int32 Countdown);
	/** Update the match time after matching */
	void UpdateMatchCountDown(int32 Countdown);
	/** Set the warmup time, match time, ...etc each frame */
	void SetHUDTime();
	void RefreshHUD();

	/** Once the game mode's MatchState is changed, the player controller's MatchState callback is going to be executed. */
	void OnMatchStateSet(FName State);

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

	/** Level starting time, MatchState on GameMode is EnteringMap */
	float LevelStartingTime = 0.f;

	/** Warmup time, MatchState on GameMode is WaitingToStart */
	float WarmupTime = 0.f;
	
	/** Match time, MatchState on GameMode is InProgress */
	float MatchTime = 0.f;

	/** Cooldown time when MatchState is InProgress and the match countdown has finished */
	float CooldownTime = 0.f;

	/** Help to distinguish 2 time seconds in the unit of integer when ticking */
	int32 CountdownInt = 0;

	/** Match State, once the game mode's match state is changed, the player controller will respond */
	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	UFUNCTION()
	void OnRep_MatchState();

	/** Handle the common functionality on replication about starting match */
	void HandleMatchHasStarted();

	/** Request the server to get the warmup time, match time, level starting time and match state once the player joins mid-game */
	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();

	/** Instead of RepNotify, we use a client RPC to transmit the data from the server to the requesting client */
	UFUNCTION(Client, Reliable)
	void ClientJoinMidGame(float LevelStarting, float Warmup, float Match, float Cooldown, FName State);
};
