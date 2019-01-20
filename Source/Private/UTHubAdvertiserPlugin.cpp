
#include "UTHubAdvertiser.h"

#include "ModuleManager.h"
#include "ModuleInterface.h"

#include "UnrealTournament.h"

class FUTHubAdvertiserPlugin : public IModuleInterface
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	FDelegateHandle OnActorSpawnedDelegateHandle;

	void OnPostWorldInitialization(UWorld* World, const UWorld::InitializationValues IVS);
	void OnActorSpawned(AActor* InActor);
};

IMPLEMENT_MODULE(FUTHubAdvertiserPlugin, UTHubAdvertiser)


//================================================
// Startup
//================================================

void FUTHubAdvertiserPlugin::StartupModule()
{
	UE_LOG(LogLoad, Log, TEXT("[UTHubAdvertiser] StartupModule"));

	// Early exit if we are an instance
	FString Cmd = FCommandLine::Get();
	if (Cmd.Find("-log=Instance_") >= 0)
	{
		return;
	}

	// Wait for World
	auto deleg = FWorldDelegates::FWorldInitializationEvent::FDelegate::CreateRaw(this, &FUTHubAdvertiserPlugin::OnPostWorldInitialization);
	FWorldDelegates::OnPostWorldInitialization.Add(deleg);
}


//================================================
// World
//================================================

void FUTHubAdvertiserPlugin::OnPostWorldInitialization(UWorld* World, const UWorld::InitializationValues IVS)
{
	UE_LOG(LogLoad, DEBUGLOG, TEXT("[UTHubAdvertiser] OnPostWorldInitialization"));
	if (World != nullptr)
	{
		UE_LOG(LogLoad, DEBUGLOG, TEXT("[UTHubAdvertiser] World = %s"), *World->GetName());

		// Wait for Gamemode
		auto deleg = FOnActorSpawned::FDelegate::CreateRaw(this, &FUTHubAdvertiserPlugin::OnActorSpawned);
		OnActorSpawnedDelegateHandle = World->AddOnActorSpawnedHandler(deleg);
	}
	else
	{
		UE_LOG(LogLoad, Warning, TEXT("[UTHubAdvertiser] World is null"));
	}
}

void FUTHubAdvertiserPlugin::OnActorSpawned(AActor* InActor)
{
	UE_LOG(LogLoad, DEBUGLOG, TEXT("[UTHubAdvertiser] OnActorSpawned: %s"), *InActor->GetName());

	AGameMode* Gamemode = Cast<AGameMode>(InActor);
	if ( Gamemode != nullptr )
	{
		UE_LOG(LogLoad, DEBUGLOG, TEXT("[UTHubAdvertiser] Got Gamemode: %s"), *Gamemode->GetName());

		// Stop waiting
		InActor->GetWorld()->RemoveOnActorSpawnedHandler(OnActorSpawnedDelegateHandle);

		if (Gamemode->IsA(AUTLobbyGameMode::StaticClass()))
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (!Gamemode->GetWorld()->SpawnActor<AUTHubAdvertiser>(AUTHubAdvertiser::StaticClass(), Params))
			{
				UE_LOG(LogLoad, Warning, TEXT("[UTAdminUtils] Failed to spawn UTHubAdvertiser !"));
			}
		}
		else
		{
			UE_LOG(LogLoad, Warning, TEXT("[UTHubAdvertiser] Gamemode is not a UTLobbyGameMode (%s)"), *Gamemode->GetName());
		}
	}
}


//================================================
// Shutdown
//================================================

void FUTHubAdvertiserPlugin::ShutdownModule()
{
	UE_LOG(LogLoad, Log, TEXT("[UTHubAdvertiser] ShutdownModule"));
}
