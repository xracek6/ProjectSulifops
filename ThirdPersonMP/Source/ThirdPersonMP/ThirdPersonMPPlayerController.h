// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ThirdPersonMPPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class AThirdPersonMPPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void ToggleMenu();
	
	UFUNCTION(BlueprintCallable)
	void OpenMenu();
	
	UFUNCTION(BlueprintCallable)
	void CloseMenu();

protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;
	
	UPROPERTY(EditAnywhere, Category ="Menu")
	TSubclassOf<UUserWidget> MenuWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> MenuWidget;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

};
