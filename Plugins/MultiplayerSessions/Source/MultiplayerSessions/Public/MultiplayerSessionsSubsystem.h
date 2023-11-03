// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/OnlineSessionDelegates.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MultiplayerSessionsSubsystem.generated.h"

//
// 声明自定义的 delegate, 用于在 Menu 中绑定回调函数，以便在创建 Session 时调用获取相关信息
// Dynamic Multicast Delegate 的所有参数类型都必须与蓝图兼容，否则只能使用 Multicast Delegate
// 且绑定到 Dynamic Multicast Delegate 上的回调函数必须是 UFUNCTION
//
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnCreateSessionCompleteDelegate, bool, bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMultiplayerOnFindSessionsCompleteDelegate, const TArray<FOnlineSessionSearchResult>& SearchResults, bool bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_OneParam(FMultiplayerOnJoinSessionCompleteDelegate, EOnJoinSessionCompleteResult::Type Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerDestroySessionCompleteDelegate, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerStartSessionCompleteDelegate, bool, bWasSuccessful);

/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMultiplayerSessionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UMultiplayerSessionsSubsystem();

	//
	// 外部调用接口
	//
	void CreateSession(int32 NumPublicConnections, FString MatchType);
	void FindSessions(int32 MaxSearchResults);
	void JoinSession(const FOnlineSessionSearchResult& SearchResult);
	void DestroySession();
	void StartSession();

	//
	// 自定义委托，在 Menu 中绑定回调函数
	//
	FMultiplayerOnCreateSessionCompleteDelegate MultiplayerCreateSessionCompleteDelegate;
	FMultiplayerOnFindSessionsCompleteDelegate MultiplayerFindSessionsCompleteDelegate;
	FMultiplayerOnJoinSessionCompleteDelegate MultiplayerJoinSessionCompleteDelegate;
	FMultiplayerDestroySessionCompleteDelegate MultiplayerDestroySessionCompleteDelegate;
	FMultiplayerStartSessionCompleteDelegate MultiplayerStartSessionCompleteDelegate;

protected:
	//
	// 回调函数
	//
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);

private:
	bool IsValidSessionInterface();
	
private:
	TSharedPtr<class IOnlineSession, ESPMode::ThreadSafe> SessionInterface;

	TSharedPtr<class FOnlineSessionSettings> LastSessionSettings;
	TSharedPtr<class FOnlineSessionSearch> LastSessionSearch;

	//
	// 绑定回调到委托上，将其加入到 Online Sessions Interface 的 delegate list 中
	//
	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FDelegateHandle CreateSessionCompleteDelegateHandle;
	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	FDelegateHandle FindSessionsCompleteDelegateHandle;
	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
	FDelegateHandle JoinSessionCompleteDelegateHandle;
	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
	FDelegateHandle DestroySessionCompleteDelegateHandle;
	FOnStartSessionCompleteDelegate StartSessionCompleteDelegate;
	FDelegateHandle StartSessionCompleteDelegateHandle;

	bool bCreateSessionOnDestroy{false};
	int32 LastNumPublicConnections;
	FString LastMatchType;
};
