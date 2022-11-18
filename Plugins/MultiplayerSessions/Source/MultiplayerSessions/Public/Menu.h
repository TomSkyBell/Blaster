// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Menu.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void SetupMenu(int32 NumberOfPublicConnections = 4, FString TypeOfMatch = TEXT("FreeForAll"),
		int32 MaxNumberOfSearchResults = 10000, FString PathOfLobby = FString(TEXT("/Game/ThirdPerson/Maps/Lobby")));

	virtual bool Initialize() override;

private:
	// Widget
	UPROPERTY(meta = (BindWidget))
	class UButton* HostButton;
	
	UPROPERTY(meta = (BindWidget))
	class UButton* JoinButton;

	// Member Variables
	int32 NumPublicConnections{4};
	FString MatchType{TEXT("FreeForAll")};
	int32 MaxSearchResults{10000};
	FString LobbyPath{"/Game/ThirdPerson/Maps/Lobby"};

	// Callback function for the dynamic delegate, needs a specifier
	UFUNCTION()
	void HostButtonClicked();

	UFUNCTION()
	void JoinButtonClicked();

	// Shut off menu when callback 'OnLevelRemovedFromWorld' responds
	void ShutOffMenu();

	// Callback of the custom dynamic-multicast delegate 'MultiplayerOnCreateSessionComplete'
	UFUNCTION()
	void OnCreateSession(bool bWasSuccessful);

	void OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);

	void OnJoinSession(EOnJoinSessionCompleteResult::Type Result);

	UFUNCTION()
	void OnStartSession(bool bWasSuccessful);

	UFUNCTION()
	void OnDestroySession(bool bWasSuccessful);

	// Callback when travel to a new level
	void OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld);

	// The subsystem designed to handle all online session functionality
	UPROPERTY()
	class UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;
};
