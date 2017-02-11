/*Copyright 2016 Google
Author: Opaque Media Group
 
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and\
limitations under the License.*/

#pragma once

#include "TangoDataTypes.h"
#include "LatentActions.h"


#if PLATFORM_ANDROID
#include "tango_client_api.h"
#include <pthread.h>
#endif

class FSaveAreaDescriptionAction : public FPendingLatentAction
{
	float PercentDone;
	bool &bIsSuccessful;
	FString Filename;
	FTangoAreaDescription& Result;
	FName ExecutionFunction;
	int32 OutputLink;
	FWeakObjectPtr CallbackTarget;
	FRunnableThread* Thread;
	bool bStarted;
	class MyRunner : public FRunnable
	{
		FSaveAreaDescriptionAction* Target;
	public:
		MyRunner(FSaveAreaDescriptionAction* InTarget) : Target(InTarget) {}
		virtual uint32 Run() override
		{
			Target->Run();
			return 0;
		}
	};
	MyRunner* Runner;
public:
	FSaveAreaDescriptionAction(const FString& InFilename, FTangoAreaDescription& InResult, bool& InIsSuccessful, const FLatentActionInfo& LatentInfo)
		: PercentDone(0.0f)
		, Filename(InFilename)
		, Result(InResult)
		, bIsSuccessful(InIsSuccessful)
		, bStarted(false)
		, ExecutionFunction(LatentInfo.ExecutionFunction)
		, OutputLink(LatentInfo.Linkage)
		, CallbackTarget(LatentInfo.CallbackTarget)
	{
		
	}

	virtual ~FSaveAreaDescriptionAction()
	{
		Finish();
	}

	void Run();

	void SetIsSuccessful(bool bValue)
	{
		bIsSuccessful = bValue;
	}

	void UpdatePercentDone(float NewValue)
	{
		PercentDone = NewValue;
	}

	void Finish()
	{
		if (Thread != nullptr)
		{
			Thread->WaitForCompletion();
			delete Thread;
			Thread = nullptr;
		}
		if (Runner != nullptr)
		{
			delete Runner;
			Runner = nullptr;
		}
	}

	virtual void UpdateOperation(FLatentResponse& Response) override
	{
#if PLATFORM_ANDROID
		if (!bStarted)
		{
			bStarted = true;
			Runner = new MyRunner(this);
			Thread = FRunnableThread::Create(Runner, TEXT("SaveAreaDescription"));
		}
		if (PercentDone == 1.0)
		{
			Finish();
		}
		Response.FinishAndTriggerIf(PercentDone==1.0, ExecutionFunction, OutputLink, CallbackTarget);
#else
		bIsSuccessful = false;
		Response.FinishAndTriggerIf(true, ExecutionFunction, OutputLink, CallbackTarget);
#endif
	}

#if WITH_EDITOR
	// Returns a human readable description of the latent operation's current state
	virtual FString GetDescription() const override
	{
		return FString::Printf(*NSLOCTEXT("SaveAreaDescriptionAction", "PercentDone", "Saving (%.3f done)").ToString(), PercentDone);
	}
#endif
};

#if 0
class FImportDescriptionAction : public FPendingLatentAction
{
	float PercentDone;
	bool &bIsSuccessful;
	FString Filename;
	FString UUID;
	FTangoAreaDescription& Result;
	FName ExecutionFunction;
	int32 OutputLink;
	FWeakObjectPtr CallbackTarget;
	FRunnableThread* Thread;
	bool bStarted;
	class MyRunner : public FRunnable
	{
		FImportAreaDescriptionAction* Target;
	public:
		MyRunner(FImportAreaDescriptionAction* InTarget) : Target(InTarget) {}
		virtual uint32 Run() override
		{
			Target->Run();
			return 0;
		}
	};
	MyRunner* Runner;
public:
	FImportAreaDescriptionAction(const FString& InFilename, const FString& InUUID, FTangoAreaDescription& InResult, bool& InIsSuccessful, const FLatentActionInfo& LatentInfo)
		: PercentDone(0.0f)
		, Filename(InFilename)
		, UUID(InUUID)
		, Result(InResult)
		, bIsSuccessful(InIsSuccessful)
		, bStarted(false)
		, ExecutionFunction(LatentInfo.ExecutionFunction)
		, OutputLink(LatentInfo.Linkage)
		, CallbackTarget(LatentInfo.CallbackTarget)
	{
	}

	virtual ~FImportAreaDescriptionAction()
	{
		Finish();
	}

	void Run();

	void SetIsSuccessful(bool bValue)
	{
		bIsSuccessful = bValue;
	}

	void UpdatePercentDone(float NewValue)
	{
		PercentDone = NewValue;
	}

	void Finish()
	{
		if (Thread != nullptr)
		{
			Thread->WaitForCompletion();
			delete Thread;
			Thread = nullptr;
		}
		if (Runner != nullptr)
		{
			delete Runner;
			Runner = nullptr;
		}
	}

	virtual void UpdateOperation(FLatentResponse& Response) override
	{
#if PLATFORM_ANDROID
		if (!bStarted)
		{
			bStarted = true;
			Runner = new MyRunner(this);
			Thread = FRunnableThread::Create(Runner, TEXT("SaveAreaDescription"));
		}
		if (PercentDone == 1.0)
		{
			Finish();
		}
		Response.FinishAndTriggerIf(PercentDone == 1.0, ExecutionFunction, OutputLink, CallbackTarget);
#else
		bIsSuccessful = false;
		Response.FinishAndTriggerIf(true, ExecutionFunction, OutputLink, CallbackTarget);
#endif
	}

#if WITH_EDITOR
	// Returns a human readable description of the latent operation's current state
	virtual FString GetDescription() const override
	{
		return FString::Printf(*NSLOCTEXT("ImportAreaDescriptionAction", "PercentDone", "Saving (%.3f done)").ToString(), PercentDone);
	}
#endif
};

#endif

class TangoDeviceAreaLearning
{
public:
	TangoDeviceAreaLearning();
	~TangoDeviceAreaLearning();

	//Tango Area Learning functions
	bool DeleteAreaDescription(FString UUID);
	FTangoAreaDescription SaveCurrentArea(FString Filename, bool& bIsSuccessful);
	void OnImportResult(const FTangoEvent& Result);
	void OnExportResult(const FTangoEvent& Result);
private:
	static FString AreaLearningDir;
	FSaveAreaDescriptionAction* CurrentSaveAction;
	friend class FSaveAreaDescriptionAction;
};
