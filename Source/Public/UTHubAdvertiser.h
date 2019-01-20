#pragma once

#include "Engine.h"
#include "Http.h"

#include "UTHubAdvertiser.generated.h"

#define DEBUGLOG Verbose
//#define DEBUGLOG Log

UCLASS(Config=Game)
class AUTHubAdvertiser : public AActor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(Config) FString URL;
	UPROPERTY(Config) TArray<FString> HeaderKeys;
	UPROPERTY(Config) TArray<FString> HeaderValues;
	UPROPERTY(Config) float Interval;

	static int32 NumRequestsLogged;

	void BeginPlay() override;

	FTimerHandle TimerHandle;
	void Timer();

	void MyJsonReport(TSharedPtr<FJsonObject> JsonObject);

	void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};
