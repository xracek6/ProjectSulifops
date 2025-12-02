// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSubsystem.h"
#include "Online/OnlineSessionNames.h"

void PrintString(const FString& String)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, String);
	}
}

UMultiplayerSessionsSubsystem::UMultiplayerSessionsSubsystem()
{
	PrintString("MSS Constructor");
	bCreateServerAfterDestroy = false;
	DestroyServerName = "";
	ServerNameToFind = "";
	MySessionName = "Test coop session";
}

void UMultiplayerSessionsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	PrintString("MSS Initialize");
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		FString SubsystemName = OnlineSubsystem->GetSubsystemName().ToString();
		PrintString(SubsystemName);
		
		SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			PrintString("Session Interface is valid");
			SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &UMultiplayerSessionsSubsystem::OnCreateSessionComplete);
			SessionInterface->OnDestroySessionCompleteDelegates.AddUObject(this, &UMultiplayerSessionsSubsystem::OnDestroySessionComplete);
			SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this, &UMultiplayerSessionsSubsystem::OnFindSessionsComplete);
			SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &UMultiplayerSessionsSubsystem::OnJoinSessionComplete);
		}
	}
}

void UMultiplayerSessionsSubsystem::Deinitialize()
{
	UE_LOG(LogTemp, Warning, TEXT("MSS Deinitialize"));
}

void UMultiplayerSessionsSubsystem::CreateServer(FString ServerName)
{
	PrintString("Creating server");
	if (ServerName.IsEmpty())
	{
		PrintString("Server name cannot be empty!");
		return;
	}
	
	FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(MySessionName);
	if (ExistingSession)
	{
		FString Msg = FString::Printf(TEXT("Session with name %s already exists, destroying it"), *MySessionName.ToString());
		PrintString(Msg);
		
		bCreateServerAfterDestroy = true;
		DestroyServerName = ServerName;
		SessionInterface->DestroySession(MySessionName);
		return;
	}
	
	FOnlineSessionSettings SessionSettings;
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.bIsDedicated = false;
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.NumPublicConnections = 4;
	SessionSettings.bUseLobbiesIfAvailable = true;
	SessionSettings.bUsesPresence = true;
	SessionSettings.bAllowJoinViaPresence = true;
	SessionSettings.bIsLANMatch = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL";
	
	SessionSettings.Set(FName("SERVER_NAME"), ServerName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	
	SessionInterface->CreateSession(0, MySessionName, SessionSettings);
}

void UMultiplayerSessionsSubsystem::FindServer(FString ServerName)
{
	PrintString("Finding server");
	
	if (ServerName.IsEmpty())
	{
		PrintString("Server name cannot be empty!");
		return;
	}
	
	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	SessionSearch->bIsLanQuery = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL";
	SessionSearch->MaxSearchResults = 9999;
	
	SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	
	ServerNameToFind = ServerName;
	
	SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
}

void UMultiplayerSessionsSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		PrintString(FString::Printf(TEXT("Successfully created a session with name: %s"), *SessionName.ToString()));
		GetWorld()->ServerTravel("/Game/ThirdPerson/Lvl_ThirdPerson?listen");
	} else
	{
		PrintString(FString::Printf(TEXT("Failed to create a session with name: %s"), *SessionName.ToString()));
	}
}

void UMultiplayerSessionsSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		PrintString(FString::Printf(TEXT("Successfully destroyed a session with name: %s"), *SessionName.ToString()));
		
		if (bCreateServerAfterDestroy)
		{
			bCreateServerAfterDestroy = false;
			CreateServer(DestroyServerName);
		}
	} else
	{
		PrintString(FString::Printf(TEXT("Failed to destroy a session with name: %s"), *SessionName.ToString()));
	}
}

void UMultiplayerSessionsSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (!bWasSuccessful)
	{
		PrintString("Failed to find sessions");
		return;
	}
	
	if (ServerNameToFind.IsEmpty())
	{
		PrintString("ServerNameToFind is empty!");
		return;
	}
	
	
	TArray<FOnlineSessionSearchResult> Results = SessionSearch->SearchResults;
	if (Results.Num() == 0)
	{
		PrintString("Zero sessions were found");
		return;
	}
	
	FString Msg = FString::Printf(TEXT("Found %d sessions"), Results.Num());
	PrintString(Msg);
	
	FOnlineSessionSearchResult* CorrectResult = nullptr;
	
	for (FOnlineSessionSearchResult Result : Results)
	{
		if (!Result.IsValid())
		{
			continue;
		}
		
		FString ServerName = "No-name";
		Result.Session.SessionSettings.Get(FName("SERVER_NAME"), ServerName);
			
		if (ServerName.Equals(ServerNameToFind))
		{
			CorrectResult = &Result;
			FString Msg2 = FString::Printf(TEXT("Found server with name: %s"), *ServerName);
			PrintString(Msg2);
			break;
		}
	}
	
	if (CorrectResult == nullptr)
	{
		FString Msg3 = FString::Printf(TEXT("Couldn't find server with name: %s"), *ServerNameToFind);
		PrintString(Msg3);
		
		ServerNameToFind = "";
		return;
	}
	
	CorrectResult->Session.SessionSettings.bUsesPresence = true;
	CorrectResult->Session.SessionSettings.bUseLobbiesIfAvailable = true;
	SessionInterface->JoinSession(0, MySessionName, *CorrectResult);
}

void UMultiplayerSessionsSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		FString MsgSuccess = FString::Printf(TEXT("Successfully joined session %s"), *SessionName.ToString());
		PrintString(MsgSuccess);
		
		FString Address = "";
		bool bSuccess = SessionInterface->GetResolvedConnectString(SessionName, Address);
		if (bSuccess)
		{
			FString Msg = FString::Printf(TEXT("Address: %s"), *Address);
			PrintString(Msg);
			
			APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
			if (PlayerController)
			{
				PlayerController->ClientTravel(Address, TRAVEL_Absolute);
			}
		} else
		{
			PrintString("GetResolvedConnectString returned false!");
		}
		
	} else
	{
		FString MsgError = FString::Printf(TEXT("OnJoinSessionComplete failed for session with name: %s, error code: %d"), *SessionName.ToString(), Result);
		PrintString(MsgError);
	}
}
