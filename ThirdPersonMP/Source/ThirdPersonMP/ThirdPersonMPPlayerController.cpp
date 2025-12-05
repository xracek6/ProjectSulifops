// Copyright Epic Games, Inc. All Rights Reserved.


#include "ThirdPersonMPPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "ThirdPersonMP.h"
#include "Widgets/Input/SVirtualJoystick.h"

void AThirdPersonMPPlayerController::ToggleMenu()
{
	if (MenuWidget == nullptr)
	{
		UE_LOG(LogThirdPersonMP, Error, TEXT("Menu Widget is null in AThirdPersonMPPlayerController::ToggleMenu()"));
		
		const FString Message = FString(TEXT("Player Controller is null in AThirdPersonMPPlayerController::ToggleMenu()"));
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, Message);
		return;
	}
	MenuWidget->IsInViewport() ? CloseMenu() : OpenMenu();
}

void AThirdPersonMPPlayerController::OpenMenu()
{
	if (MenuWidget->IsInViewport())
	{
		return;
	}
	
	MenuWidget->AddToViewport(0);
	
	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(MenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	SetShowMouseCursor(true);
	SetIgnoreMoveInput(true);
}

void AThirdPersonMPPlayerController::CloseMenu()
{
	if (!MenuWidget->IsInViewport())
	{
		return;
	}
	
	MenuWidget->RemoveFromParent();

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	SetShowMouseCursor(false);
	SetIgnoreMoveInput(false);
}

void AThirdPersonMPPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	MenuWidget = CreateWidget<UUserWidget>(this, MenuWidgetClass);
	if (MenuWidget == nullptr)
	{
		UE_LOG(LogThirdPersonMP, Error, TEXT("Could not spawn MenuWidget in AThirdPersonMPPlayerController::BeginPlay()"));
		return;
	}
	
	// only spawn touch controls on local player controllers
	if (SVirtualJoystick::ShouldDisplayTouchInterface() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogThirdPersonMP, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void AThirdPersonMPPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!SVirtualJoystick::ShouldDisplayTouchInterface())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}
