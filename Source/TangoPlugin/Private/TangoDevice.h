#pragma once
/*Copyright 2016 Google
Author: Opaque Media Group
 
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.*/
#include "TangoDataTypes.h"
#include "TangoDevicePointCloud.h"
#include "TangoDeviceMotion.h"
#include "TangoDeviceImage.h"
#include "TangoDeviceAreaLearning.h"
#include "TangoEventComponent.h"
#include "TangoViewExtension.h"

#include <sstream>
#include <stdlib.h>
#include <string>

#if PLATFORM_ANDROID
#include "tango_client_api.h"
#endif

#include "TangoDevice.generated.h"

class UTangoPointCloudComponent;

UCLASS(NotBlueprintable, NotPlaceable, Transient)
class UTangoDevice : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

private:

	static UTangoDevice * Instance;
	UTangoDevice();
	void ProperInitialize();
    void DeallocateResources();
	~UTangoDevice();
	virtual void BeginDestroy() override;

	//Pointers to optional Submodules
	TangoDevicePointCloud* PointCloudHelper;
	UPROPERTY(transient)
		UTangoDeviceMotion* MotionHelper;
	UPROPERTY(transient)
		UTangoDeviceImage* ImageHelper;
	TangoDeviceAreaLearning* AreaHelper;

	//FTickableGameObject interface
	virtual bool IsTickable() const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

public:
	static UTangoDevice& Get();
    
	//Getter functions for different submodules
	TangoDevicePointCloud* GetTangoDevicePointCloudPointer();
	UTangoDeviceMotion* GetTangoDeviceMotionPointer();
	UTangoDeviceImage* GetTangoDeviceImagePointer();
	TangoDeviceAreaLearning* GetTangoDeviceAreaLearningPointer();

	///////////////
	// Lifecycle //
	///////////////
public:
	
	//Tango Service Enums
	enum ServiceStatus
	{
		CONNECTED = 0,
		DISCONNECTED = 1,
		DISCONNECTED_BY_APPSERVICEPAUSE = 2,
		FAILED_TO_CONNECT = 3
	};

	//Core Tango functions
	bool IsTangoServiceRunning();
	bool IsLearningModeEnabled();
	ServiceStatus GetTangoServiceStatus();
	void RestartService(FTangoConfig& Config, FTangoRuntimeConfig& RuntimeConfig);
	void StartTangoService(FTangoConfig& Config, FTangoRuntimeConfig& RuntimeConfig);
	bool SetTangoRuntimeConfig(FTangoRuntimeConfig Configuration,bool bPreRuntime = false);
	void StopTangoService();
	
	FTangoConfig& GetCurrentConfig();
	FTangoRuntimeConfig& GetCurrentRuntimeConfig();

private:
	bool bHasBeenPropelyInitialized = false;
	//Service Status
	ServiceStatus ConnectionState;
	//Service Configuration
	FTangoConfig CurrentConfig;
	FTangoRuntimeConfig CurrentRuntimeConfig;

#if PLATFORM_ANDROID
	TangoConfig Config_; //Internal Tango Pointer
	//Core service functions
	bool ApplyConfig();
	void ConnectTangoService();
	void DisconnectTangoService(bool bByAppServicePause = false);
    //To be called during the final phase of DisconnectTangoService
    void UnbindTangoService();
	//Delegate binding functions
	void AppServiceResume();
	void AppServicePause();

public:
    //Note: last part of the new async connection method
    void BindAndCompleteConnectionToService(JNIEnv* Env, jobject IBinder);

#endif
    
	///////////////////////////
	// General functionality //
	///////////////////////////
public:
	float GetMetersToWorldScale();
	//Tango Camera Intrinsics defined here because we need the intrinsics to start the ImageDevice!
	FTangoCameraIntrinsics GetCameraIntrinsics(ETangoCameraType CameraID);

	//Area accessibility functions. Found in TangoDeviceADF.cpp
	FString GetLoadedAreaDescriptionUUID();
	TArray<FTangoAreaDescription> GetAreaDescriptions();
	TArray<FString> GetAllUUIDs();
	FTangoAreaDescriptionMetaData GetMetaData(FString UUID, bool& bIsSuccessful);
    void SaveMetaData(FString UUID, FTangoAreaDescriptionMetaData NewMetaData, bool& bIsSuccessful);
	void ImportCurrentArea(FString Filepath, bool& bIsSuccessful);
	void ExportCurrentArea(FString UUID, FString Filepath, bool& bIsSuccessful);
	/////////////////
	// Tango Event //
	/////////////////
	static void RunOnMainThread(const TFunction<void()> Runnable);
public:
	void AttachTangoEventComponent(UTangoEventComponent* Component);
#if PLATFORM_ANDROID
    void PushTangoEvent(const FTangoEvent);
#endif
    
private:
	void ConnectEventCallback();
	void BroadCastConnect();
	void BroadCastDisconnect();
	void BroadCastEvents();
#if PLATFORM_ANDROID
    void OnTangoEvent(const TangoEvent * Event);
    void PopulateAppContext();
    void DePopulateAppContext();
#endif
	UPROPERTY(transient)
	TArray<UTangoEventComponent*> TangoEventComponents;
	TArray<FTangoEvent> CurrentEvents;
	//For less blocking :(
	TArray<FTangoEvent> CurrentEventsCopy;
	FCriticalSection EventLock;
#if PLATFORM_ANDROID
    jobject AppContextReference;
#endif

	/////////////////////
	// Persistent Data //
	/////////////////////
public:
	//@TODO: use friend classes instead of public

	//ATTENTION: These properties are used by other classes.
	//They are here so persist even if the tango is being disconnected

	//TangoDeviceImage
	UPROPERTY(transient)
		UTexture2D * YTexture;
	UPROPERTY(transient)
		UTexture2D * CrTexture;
	UPROPERTY(transient)
		UTexture2D * CbTexture;
	//TangoDeviceMotion
	UPROPERTY(transient)
		TArray<UTangoMotionComponent*> MotionComponents;
	UPROPERTY(transient)
		TArray<UTangoPointCloudComponent*> PointCloudComponents;
	TArray<TArray<FTangoCoordinateFramePair>> RequestedPairs;
	void AddTangoMotionComponent(UTangoMotionComponent* Component, TArray<FTangoCoordinateFramePair>& Requests);
};