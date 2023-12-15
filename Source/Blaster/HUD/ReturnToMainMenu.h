#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReturnToMainMenu.generated.h"


UCLASS()
class BLASTER_API UReturnToMainMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	void MenuSetup();
	void MenuTeardown();

private:
	UFUNCTION()
	void ReturnButtonClicked();
	UFUNCTION()
	void ExitButtonClicked();

	UFUNCTION()
	void OnDestroySessionComplete(bool bWasSuccessful);

	UFUNCTION()
	void OnPlayerLeftGame();

private:
	UPROPERTY(meta=(BindWidget))
	class UButton* ReturnButton;
	UPROPERTY(meta=(BindWidget))
	class UTextBlock* ReturnText;

	UPROPERTY(meta=(BindWidget))
	UButton* ExitButton;
	UPROPERTY(meta=(BindWidget))
	UTextBlock* ExitText;

	UPROPERTY()
	class APlayerController* PlayerController;
	UPROPERTY()
	class UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;
};
