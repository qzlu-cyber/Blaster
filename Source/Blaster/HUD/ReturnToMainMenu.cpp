#include "ReturnToMainMenu.h"

#include "Components/Button.h"
#include "GameFramework/GameModeBase.h"
#include "MultiplayerSessionsSubsystem.h"

void UReturnToMainMenu::MenuSetup()
{
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);

	UWorld* World = GetWorld();
	if (World)
	{
		PlayerController = PlayerController == nullptr ? World->GetFirstPlayerController() : PlayerController;
		if (PlayerController)
		{
			FInputModeGameAndUI InputModeData;
			InputModeData.SetWidgetToFocus(TakeWidget());
			InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(true);
		}
	}
	
	if (ReturnButton && !ReturnButton->OnClicked.IsBound()) ReturnButton->OnClicked.AddDynamic(this, &UReturnToMainMenu::ReturnButtonClicked);
	if (ExitButton && !ExitButton->OnClicked.IsBound()) ExitButton->OnClicked.AddDynamic(this, &UReturnToMainMenu::ExitButtonClicked);

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
		if (MultiplayerSessionsSubsystem)
			MultiplayerSessionsSubsystem->MultiplayerDestroySessionCompleteDelegate.AddDynamic(this, &UReturnToMainMenu::OnDestroySessionComplete);
	}
}

void UReturnToMainMenu::MenuTeardown()
{
	RemoveFromParent();
	UWorld* World = GetWorld();
	if (World)
	{
		PlayerController = PlayerController == nullptr ? World->GetFirstPlayerController() : PlayerController;
		if (PlayerController)
		{
			FInputModeGameOnly InputModeData;
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(false);
		}
	}

	if (ReturnButton && ReturnButton->OnClicked.IsBound()) ReturnButton->OnClicked.RemoveDynamic(this, &UReturnToMainMenu::ReturnButtonClicked);
	if (ExitButton && ExitButton->OnClicked.IsBound()) ExitButton->OnClicked.RemoveDynamic(this, &UReturnToMainMenu::ExitButtonClicked);

	if (MultiplayerSessionsSubsystem && MultiplayerSessionsSubsystem->MultiplayerDestroySessionCompleteDelegate.IsBound())
		MultiplayerSessionsSubsystem->MultiplayerDestroySessionCompleteDelegate.RemoveDynamic(this, &UReturnToMainMenu::OnDestroySessionComplete);
}

void UReturnToMainMenu::ReturnButtonClicked()
{
	MenuTeardown();
}

void UReturnToMainMenu::ExitButtonClicked()
{
	ExitButton->SetIsEnabled(false); // 点击退出按钮后，禁用退出按钮，等待销毁 session

	if (MultiplayerSessionsSubsystem) MultiplayerSessionsSubsystem->DestroySession();
}

void UReturnToMainMenu::OnDestroySessionComplete(bool bWasSuccessful)
{
	if (!bWasSuccessful) // 如果销毁 session 失败，则启用退出按钮，等待下一次点击
	{
		ReturnButton->SetIsEnabled(true);
		return;
	}

	UWorld* World = GetWorld();
	if (World)
	{
		AGameModeBase* GameMode = World->GetAuthGameMode<AGameModeBase>();
		if (GameMode) GameMode->ReturnToMainMenuHost(); // 如果是 server，则直接返回主菜单
		else // 如果是 client，则先退出到主菜单，再返回主菜单
		{
			PlayerController = PlayerController == nullptr ? World->GetFirstPlayerController() : PlayerController;
			if (PlayerController) PlayerController->ClientReturnToMainMenuWithTextReason(FText());
		}
	}
}
