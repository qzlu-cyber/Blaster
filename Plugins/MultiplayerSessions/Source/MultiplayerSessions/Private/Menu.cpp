// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu.h"
#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"

#include "Components/Button.h"

void UMenu::MenuSetUp(int32 NumberOfPublicConnections, FString TypeOfMatch, int32 MaxResultsOfSearch, FString LevelPath)
{
	// 设置参数
	NumPublicConnections = NumberOfPublicConnections;
	MatchType = TypeOfMatch;
	MaxSearchResults = MaxResultsOfSearch;
	PathToLevel = FString::Printf(TEXT("%s?listen"), *LevelPath);
	
	AddToViewport(); // 将 Widget 添加到 Viewport 中
	SetVisibility(ESlateVisibility::Visible); // 设置 Widget 的可见性
	SetIsFocusable(true); // 设置 Widget 可以获得焦点

	//
	// 设置 Input Mode
	//
	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeUIOnly InputMode; // UI Only 模式，一旦创建了 Widgets 并且正在调用 MenuSetUp，只专注于 UI 不会对 pawns 产生影响
			InputMode.SetWidgetToFocus(TakeWidget()); // 设置 Widget 获得焦点
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); // 设置鼠标不锁定在 Viewport 中
			PlayerController->SetInputMode(InputMode); // 设置 Input Mode
			PlayerController->bShowMouseCursor = true; // 显示鼠标
		}
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance) MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();

	if (MultiplayerSessionsSubsystem)
	{
		// 绑定回调函数到自定义委托上
		MultiplayerSessionsSubsystem->MultiplayerCreateSessionCompleteDelegate.AddDynamic(this, &UMenu::UMenu::OnCreateSessionComplete);
		MultiplayerSessionsSubsystem->MultiplayerFindSessionsCompleteDelegate.AddUObject(this, &UMenu::OnFindSessionsComplete);
		MultiplayerSessionsSubsystem->MultiplayerJoinSessionCompleteDelegate.AddUObject(this, &UMenu::OnJoinSessionComplete);
		MultiplayerSessionsSubsystem->MultiplayerDestroySessionCompleteDelegate.AddDynamic(this, &UMenu::OnDestroySessionComplete);
		MultiplayerSessionsSubsystem->MultiplayerStartSessionCompleteDelegate.AddDynamic(this, &UMenu::OnStartSessionComplete);
	}
}

bool UMenu::Initialize()
{
	if (!Super::Initialize()) return false;

	// 获取 Widget 中的 Button, 并绑定 OnClicked 事件
	if (HostButton) HostButton->OnClicked.AddDynamic(this, &UMenu::HostButtonClicked);
	if (JoinButton) JoinButton->OnClicked.AddDynamic(this, &UMenu::JoinButtonClicked);
	
	return true;
}

void UMenu::NativeDestruct()
{
	MenuTearDown();
	Super::NativeDestruct();
}

void UMenu::OnCreateSessionComplete(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Blue,
				FString::Printf(TEXT("Creating session succeeded!"))
			);
		}

		UWorld* World = GetWorld();
		if (World) World->ServerTravel(PathToLevel);
	}
	else
	{
		// 创建失败时，重新启用 HostButton
		HostButton->SetIsEnabled(true);
		
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Red,
				FString::Printf(TEXT("Creating session failed!"))
			);
		}
	}
}

void UMenu::OnFindSessionsComplete(const TArray<FOnlineSessionSearchResult>& SearchResults, bool bWasSuccessful)
{
	if (!MultiplayerSessionsSubsystem) return;
	
	if (SearchResults.Num() == 0)
	{
		// 搜索失败或者搜索结果为空时，重新启用 JoinButton
		JoinButton->SetIsEnabled(true);
	}
	
	for (const auto Result : SearchResults)
	{
		FString TypeOfMatch;
		Result.Session.SessionSettings.Get(FName("MatchType"), TypeOfMatch);

		if (TypeOfMatch == MatchType)
		{
			MultiplayerSessionsSubsystem->JoinSession(Result);
			return;
		}
	}
}

void UMenu::OnJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result)
{
	if (!MultiplayerSessionsSubsystem) return;

	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
		if (OnlineSubsystem)
		{
			TSharedPtr<class IOnlineSession, ESPMode::ThreadSafe> SessionInterface = OnlineSubsystem->GetSessionInterface();

			FString Address;
			if (SessionInterface->GetResolvedConnectString(NAME_GameSession, Address))
			{
				APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
				if (PlayerController) PlayerController->ClientTravel(Address, TRAVEL_Absolute);
			}
		}
	}
	else
	{
		// 加入失败时，重新启用 JoinButton
		JoinButton->SetIsEnabled(true);
	}
}

void UMenu::OnDestroySessionComplete(bool bWasSuccessful)
{
}

void UMenu::OnStartSessionComplete(bool bWasSuccessful)
{
}

void UMenu::HostButtonClicked()
{
	// 点击 HostButton 后禁用 HostButton
	HostButton->SetIsEnabled(false);
	if (MultiplayerSessionsSubsystem)  MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);
}

void UMenu::JoinButtonClicked()
{
	// 点击 JoinButton 后禁用 JoinButton
	JoinButton->SetIsEnabled(false);
	if (MultiplayerSessionsSubsystem) MultiplayerSessionsSubsystem->FindSessions(MaxSearchResults);
}

void UMenu::MenuTearDown()
{
	RemoveFromParent(); // 从 Viewport 中移除 Widget

	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeGameOnly InputMode;
			PlayerController->SetInputMode(InputMode); // 设置 Input Mode
			PlayerController->bShowMouseCursor = false; // 隐藏鼠标
		}
	}
}
