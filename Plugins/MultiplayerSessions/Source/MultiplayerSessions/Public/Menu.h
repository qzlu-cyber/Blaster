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
	void MenuSetUp(
		int32 NumberOfPublicConnections = 4,
		FString TypeOfMatch = FString(TEXT("FreeForAll")),
		int32 MaxResultsOfSearch = 10000,
		FString LobbyPath = FString(TEXT("/Game/Maps/Lobby"))
	);

protected:
	//
	// Widget 的生命周期
	// 
	virtual bool Initialize() override; // Widget 被创建时调用
	virtual void NativeDestruct() override; // Widget 被销毁时调用

	//
	// 绑定在自定义委托上的回调函数
	// 绑定到 Dynamic Multicast Delegate 上的回调函数必须是 UFUNCTION
	//
	UFUNCTION()
	void OnCreateSessionComplete(bool bWasSuccessful);
	void OnFindSessionsComplete(const TArray<FOnlineSessionSearchResult>& SearchResults, bool bWasSuccessful);
	void OnJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result);
	UFUNCTION()
	void OnDestroySessionComplete(bool bWasSuccessful);
	UFUNCTION()
	void OnStartSessionComplete(bool bWasSuccessful);

private:
	UFUNCTION()
	void HostButtonClicked();

	UFUNCTION()
	void JoinButtonClicked();

	void MenuTearDown();

private:
	UPROPERTY(meta=(BindWidget))
	class UButton* HostButton;
	UPROPERTY(meta=(BindWidget))
	UButton* JoinButton;

	UPROPERTY(BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	int32 NumPublicConnections{4};
	int32 MaxSearchResults{10000};
	UPROPERTY(BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	FString MatchType{TEXT("FreeForAll")};
	FString PathToLobby{TEXT("")};

	class UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;
};
