// Fill out your copyright notice in the Description page of Project Settings.


#include "MenuWidget.h"

// todo: move menu opening and closing implementation here and set UMenuWidget as parent of WBP_Menu

void UMenuWidget::OpenMenu()
{
	// if (Menu->IsInViewport())
	// {
	// 	Menu->RemoveFromParent();
	//
	// 	FInputModeGameOnly GameInputMode;
	// 	MyController->SetInputMode(GameInputMode);
	// 	MyController->SetShowMouseCursor(false);
	// 	return;
	// }
	//
	// Menu->AddToViewport(0);
	//
	// FInputModeGameAndUI Mode;
	// Mode.SetWidgetToFocus(Menu->TakeWidget());
	// Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	//
	// MyController->SetInputMode(Mode);
	// MyController->SetShowMouseCursor(true);
}

void UMenuWidget::CloseMenu()
{
	// if (!IsInViewport())
	// {
	// 	return;
	// }
	//
	// RemoveFromParent();
	//
	// FInputModeGameOnly GameInputMode;
	// MyController->SetInputMode(GameInputMode);
	// MyController->SetShowMouseCursor(false);
}
