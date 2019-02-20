#include "UTHubAdvertiser.h"

#include "UnrealTournament.h"
#include "UTLobbyGameMode.h"
#include "UTLobbyMatchInfo.h"

AUTHubAdvertiser::AUTHubAdvertiser(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	URL = "";
	Interval = 30.f;
}

void AUTHubAdvertiser::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogBlueprintUserMessages, DEBUGLOG, TEXT("[UTHubAdvertiser] BeginPlay"));

	if (!URL.IsEmpty())
	{
		Timer();
		if ( Interval > 0.f )
			GetWorldTimerManager().SetTimer(TimerHandle, this, &AUTHubAdvertiser::Timer, Interval, true);
	}
	else
	{
		UE_LOG(LogBlueprintUserMessages, Warning, TEXT("[UTHubAdvertiser] URL is not configured !"));
	}
}

int32 AUTHubAdvertiser::NumRequestsLogged = 0;

void AUTHubAdvertiser::Timer()
{
	UE_LOG(LogBlueprintUserMessages, DEBUGLOG, TEXT("[UTHubAdvertiser] Timer"));

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	MyJsonReport(JsonObject);

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	if (FJsonSerializer::Serialize<>(JsonObject.ToSharedRef(), Writer))
	{
		//UE_LOG(LogBlueprintUserMessages, Log, TEXT("[UTHubAdvertiser] JsonReport %s"), *JsonString);

		if (NumRequestsLogged < 3)
		{
			UE_LOG(LogBlueprintUserMessages, Log, TEXT("[UTHubAdvertiser] Http POST %s"), *URL);
			if (++NumRequestsLogged == 3)
			{
				UE_LOG(LogBlueprintUserMessages, Log, TEXT("[UTHubAdvertiser] Will stop logging now"));
			}
		}

		// create request
		auto Request = FHttpModule::Get().CreateRequest();

		// default headers
		Request->SetHeader("User-Agent", "X-UnrealEngine-Agent");
		Request->SetHeader("Content-Type", "application/json");

		// configured headers
		for (int32 i = 0; i < HeaderKeys.Num(); i++)
		{
			Request->SetHeader(HeaderKeys[i], HeaderValues.IsValidIndex(i) ? HeaderValues[i] : "");
		}

		Request->SetURL(URL);
		Request->SetVerb("POST");
		Request->SetContentAsString(JsonString);
		//Request->OnProcessRequestComplete().BindUObject(this, &AUTHubAdvertiser::OnResponseReceived);
		Request->ProcessRequest();
	}
	else
	{
		UE_LOG(LogBlueprintUserMessages, DEBUGLOG, TEXT("[UTHubAdvertiser] JsonReport serialization error"));
	}
}

void AUTHubAdvertiser::MyJsonReport(TSharedPtr<FJsonObject> Result)
{
	AUTLobbyGameMode* GM = GetWorld()->GetAuthGameMode<AUTLobbyGameMode>();
	AUTLobbyGameState* GS = GetWorld()->GetGameState<AUTLobbyGameState>();
	if (!GM || !GS)
		return;

	// Schema version
	Result->SetNumberField(TEXT("__v"), 2);

	// Hub
	Result->SetStringField(TEXT("ServerName"), GS->ServerName);
	//Result->SetStringField(TEXT("ServerMOTD"), GS->ServerMOTD);	//bulky
	Result->SetNumberField(TEXT("ElapsedTime"), GS->ElapsedTime);
	Result->SetNumberField(TEXT("TimeUntilRestart"), ((GM->ServerRefreshCheckpoint * 60 * 60) - GetWorld()->GetTimeSeconds()));

	// ElapsedTime is not good, it stays at 0 after TimeUntilRestart elapsed
	// try with GetRealTimeSeconds() instead for comparing with InstanceLaunchTime
	Result->SetNumberField(TEXT("RealTimeSeconds"), GetWorld()->GetRealTimeSeconds());

	// Hub players
	{
		TArray<TSharedPtr<FJsonValue>> PlayersJson;
		for (APlayerState* PS : GS->PlayerArray)
		{
			AUTPlayerState* UTPS = Cast<AUTPlayerState>(PS);
			if (IsValid(UTPS))
			{
				TSharedPtr<FJsonObject> PlayerJson = MakeShareable(new FJsonObject);

				PlayerJson->SetStringField(TEXT("PlayerName"), UTPS->PlayerName);
				PlayerJson->SetStringField(TEXT("UniqueId"), UTPS->UniqueId.ToString());

				PlayersJson.Add(MakeShareable(new FJsonValueObject(PlayerJson)));
			}
		}
		Result->SetArrayField(TEXT("Players"), PlayersJson);
	}

		/*
		TArray<TSharedPtr<FJsonValue>> InstancesJson;
		for (AUTLobbyMatchInfo* MatchInfo : GS->AvailableMatches)
		{
			TSharedPtr<FJsonObject> MatchJson = MakeShareable(new FJsonObject);

			//GS->AvailableMatches[i]->MakeJsonReport(MatchJson);
			MatchJson->SetNumberField(TEXT("GameInstanceID"), MatchInfo->GameInstanceID);
			MatchJson->SetStringField(TEXT("GameInstanceGUID"), MatchInfo->GameInstanceGUID);
#if PLATFORM_LINUX 
			MatchJson->SetNumberField(TEXT("ProcessId"), MatchInfo->GameInstanceProcessHandle.IsValid() ? (int32)MatchInfo->GameInstanceProcessHandle.GetProcessInfo()->GetProcessId() : -1);
#endif
			MatchJson->SetStringField(TEXT("CurrentState"), MatchInfo->CurrentState.ToString());
			MatchJson->SetStringField(TEXT("OwnerId"), MatchInfo->OwnerId.ToString());
			MatchJson->SetStringField(TEXT("OwnerName"), MatchInfo->GetOwnerName());

			if (MatchInfo->CurrentRuleset.IsValid())
			{
				MatchJson->SetStringField(TEXT("Name"), MatchInfo->CustomGameName);
				MatchJson->SetStringField(TEXT("Ruleset"), MatchInfo->CurrentRuleset->Data.Title);
				MatchJson->SetStringField(TEXT("GameMode"), MatchInfo->CurrentRuleset->Data.GameMode);
				MatchJson->SetNumberField(TEXT("MaxPlayers"), MatchInfo->CurrentRuleset->Data.MaxPlayers);
				MatchJson->SetBoolField(TEXT("bTeamGame"), MatchInfo->CurrentRuleset->Data.bTeamGame);
				MatchJson->SetBoolField(TEXT("GameOptions"), MatchInfo->CurrentRuleset->Data.GameOptions);
			}
			else if ( MatchInfo->bDedicatedMatch )
			{
				MatchJson->SetStringField(TEXT("Name"), MatchInfo->DedicatedServerName);
				MatchJson->SetStringField(TEXT("GameMode"), MatchInfo->DedicatedServerGameMode);
				MatchJson->SetNumberField(TEXT("MaxPlayers"), MatchInfo->DedicatedServerMaxPlayers);
				MatchJson->SetBoolField(TEXT("bTeamGame"), MatchInfo->bDedicatedTeamGame);
			}

			MatchJson->SetStringField(TEXT("Map"), MatchInfo->InitialMap);

			MatchJson->SetBoolField(TEXT("bPrivateMatch"), MatchInfo->bPrivateMatch);
			MatchJson->SetBoolField(TEXT("bSpectatable"), MatchInfo->bSpectatable);
			MatchJson->SetBoolField(TEXT("bJoinAnyTime"), MatchInfo->bJoinAnytime);
			MatchJson->SetBoolField(TEXT("bRankLocked"), MatchInfo->bRankLocked);
			MatchJson->SetBoolField(TEXT("bBeginnerMatch"), MatchInfo->bBeginnerMatch);

			TArray<TSharedPtr<FJsonValue>> MatchPlayersJson;
			for (TWeakObjectPtr<AUTLobbyPlayerState> PS : MatchInfo->Players)
			{
				TSharedPtr<FJsonObject> PJson = MakeShareable(new FJsonObject);
				PS->MakeJsonReport(PJson);
				PlayersJson.Add(MakeShareable(new FJsonValueObject(PJson)));
			}
			MatchJson->SetArrayField(TEXT("Players"), MatchPlayersJson);

			MatchJson->SetNumberField(TEXT("NumPlayersInMatch"), MatchInfo->NumPlayersInMatch());
			MatchJson->SetNumberField(TEXT("NumSpectatorsInMatch"), MatchInfo->NumSpectatorsInMatch());
			MatchJson->SetBoolField(TEXT("IsPrivateMatch"), MatchInfo->IsPrivateMatch());
			MatchJson->SetNumberField(TEXT("InstanceLaunchTime"), MatchInfo->InstanceLaunchTime);

			MatchJson->SetNumberField(TEXT("TimeLimit"), MatchInfo->MatchUpdate.TimeLimit);
			MatchJson->SetNumberField(TEXT("GoalScore"), MatchInfo->MatchUpdate.GoalScore);
			MatchJson->SetNumberField(TEXT("GameTime"), MatchInfo->MatchUpdate.GameTime);
			MatchJson->SetNumberField(TEXT("NumPlayers"), MatchInfo->MatchUpdate.NumPlayers);
			MatchJson->SetNumberField(TEXT("NumSpectators"), MatchInfo->MatchUpdate.NumSpectators);
			TArray<TSharedPtr<FJsonValue>> TeamScoresJson;
			for (int32 score : MatchInfo->MatchUpdate.TeamScores)
			{
				TeamScoresJson.Add(FJsonValueNumber(score));
			}
			MatchJson->SetArrayField(TEXT("TeamScores"), TeamScoresJson);

			InstancesJson.Add(MakeShareable(new FJsonValueObject(MatchJson)));
		}
		Result->SetArrayField(TEXT("Instances"), InstancesJson);
		*/

	// Instances
	TArray<TSharedPtr<FServerInstanceData>> Instances;
	GM->GetInstanceData(Instances);
	TArray<TSharedPtr<FJsonValue>> InstancesJson;
	int32 OtherIndex = 0;
	for (TSharedPtr<FServerInstanceData>& Instance : Instances)
	{
		TSharedPtr<FJsonObject> InstanceJson = MakeShareable(new FJsonObject);

		InstanceJson->SetStringField(TEXT("GameInstanceID"), Instance->InstanceId.ToString());
		InstanceJson->SetStringField(TEXT("RulesTitle"), Instance->RulesTitle);
		InstanceJson->SetStringField(TEXT("RulesTag"), Instance->RulesTag);
		InstanceJson->SetStringField(TEXT("GameModeClass"), Instance->GameModeClass);
		InstanceJson->SetStringField(TEXT("MapName"), Instance->MapName);
		InstanceJson->SetNumberField(TEXT("MaxPlayers"), Instance->MaxPlayers);
		InstanceJson->SetNumberField(TEXT("Flags"), Instance->Flags);
		InstanceJson->SetNumberField(TEXT("RankCheck"), Instance->RankCheck);
		InstanceJson->SetBoolField(TEXT("bTeamGame"), Instance->bTeamGame);
		InstanceJson->SetBoolField(TEXT("bJoinableAsPlayer"), Instance->bJoinableAsPlayer);
		InstanceJson->SetBoolField(TEXT("bJoinableAsSpectator"), Instance->bJoinableAsSpectator);
		InstanceJson->SetStringField(TEXT("MutatorList"), Instance->MutatorList);
		InstanceJson->SetStringField(TEXT("CustomGameName"), Instance->CustomGameName);

		// MatchData
		InstanceJson->SetNumberField(TEXT("TimeLimit"), Instance->MatchData.TimeLimit);
		InstanceJson->SetNumberField(TEXT("GoalScore"), Instance->MatchData.GoalScore);
		InstanceJson->SetNumberField(TEXT("GameTime"), Instance->MatchData.GameTime);
		InstanceJson->SetNumberField(TEXT("NumPlayers"), Instance->MatchData.NumPlayers);
		InstanceJson->SetNumberField(TEXT("NumSpectators"), Instance->MatchData.NumSpectators);
		TArray<TSharedPtr<FJsonValue>> TeamScoresJson;
		for (int32 TeamScore : Instance->MatchData.TeamScores)
		{
			TeamScoresJson.Add(MakeShareable(new FJsonValueNumber(TeamScore)));
		}
		InstanceJson->SetArrayField(TEXT("TeamScores"), TeamScoresJson);
		InstanceJson->SetBoolField(TEXT("bMatchHasBegun"), Instance->MatchData.bMatchHasBegun);
		InstanceJson->SetBoolField(TEXT("bMatchHasEnded"), Instance->MatchData.bMatchHasEnded);

		// Players
		TArray<TSharedPtr<FJsonValue>> PlayersJson;
		for (FMatchPlayerListStruct& Player : Instance->Players)
		{
			TSharedPtr<FJsonObject> PlayerJson = MakeShareable(new FJsonObject);

			PlayerJson->SetStringField(TEXT("PlayerName"), Player.PlayerName);
			PlayerJson->SetStringField(TEXT("PlayerId"), Player.PlayerId);
			PlayerJson->SetStringField(TEXT("PlayerScore"), Player.PlayerScore);
			PlayerJson->SetNumberField(TEXT("TeamNum"), Player.TeamNum);

			PlayersJson.Add(MakeShareable(new FJsonValueObject(PlayerJson)));
		}
		InstanceJson->SetArrayField(TEXT("Players"), PlayersJson);

		// Find the FMatchInfo this FServerInstanceData was created from (need pid & InstanceLaunchTime)
		for (; OtherIndex < GS->AvailableMatches.Num(); OtherIndex++)
		{
			AUTLobbyMatchInfo* MatchInfo = GS->AvailableMatches[OtherIndex];
			if (MatchInfo && MatchInfo->ShouldShowInDock())
			{
#if PLATFORM_LINUX 
				InstanceJson->SetNumberField(TEXT("ProcessId"), MatchInfo->GameInstanceProcessHandle.IsValid() ? (int32)MatchInfo->GameInstanceProcessHandle.GetProcessInfo()->GetProcessId() : -1);
#endif
				InstanceJson->SetNumberField(TEXT("InstanceLaunchTime"), MatchInfo->InstanceLaunchTime);
				InstanceJson->SetStringField(TEXT("OwnerId"), MatchInfo->OwnerId.ToString());
				InstanceJson->SetStringField(TEXT("MatchState"), MatchInfo->MatchUpdate.MatchState.ToString());
				break;
			}
		}
		OtherIndex++;

		InstancesJson.Add(MakeShareable(new FJsonValueObject(InstanceJson)));
	}
	Result->SetArrayField(TEXT("Instances"), InstancesJson);
}

void AUTHubAdvertiser::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	// dont care
}