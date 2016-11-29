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
#include "TangoPluginPrivatePCH.h"
#include "TangoDevice.h"
#include "TangoFromToCObject.h"
#include "ITangoAR.h"
#include "Tickable.h"

#include <UnrealTemplate.h>

#if PLATFORM_ANDROID
//#include "Private/Android/AndroidJNI.h"
#include "Android/AndroidJNI.h"
#include "AndroidApplication.h"
#endif

UTangoARInterface::UTangoARInterface(const FObjectInitializer& Init) : Super(Init) {}

class RunOnMainThread : public FTickableGameObject
{
public:
	virtual void Tick(float DeltaSeconds) override
	{
		Run();
		delete this;
	}
	virtual bool IsTickable() const override
	{
		return true;
	}

	/** return the stat id to use for this tickable **/
	virtual TStatId GetStatId() const override
	{
		return StatId;
	}
	virtual ~RunOnMainThread() {}
	RunOnMainThread(const TFunction<void()> InRun) : Run(InRun) {}
private:
	TFunction<void()> Run;
	static TStatId StatId;

};

TStatId RunOnMainThread::StatId;

void UTangoDevice::RunOnMainThread(const TFunction<void()> Runnable)
{
	new ::RunOnMainThread(Runnable);
}

UTangoDevice* UTangoDevice::Instance;

UTangoDevice::UTangoDevice() : UObject(), FTickableGameObject()
{
	UE_LOG(TangoPlugin, Log, TEXT("UTangoDevice::UTangoDevice: Instantiating TangoDevice"));
	ConnectionState = DISCONNECTED;
	PointCloudHelper = nullptr;
	MotionHelper = nullptr;
	ImageHelper = nullptr;
	AreaHelper = nullptr;
#if PLATFORM_ANDROID
	AppContextReference = nullptr;
#endif
	UE_LOG(TangoPlugin, Log, TEXT("UTangoDevice::UTangoDevice: Instantiating TangoDevice FINISHED"));
}

void UTangoDevice::ProperInitialize()
{
	UE_LOG(TangoPlugin, Log, TEXT("UTangoDevice::ProperInitialize: called"));
	ConnectionState = DISCONNECTED;
	PointCloudHelper = nullptr;
	MotionHelper = nullptr;
	ImageHelper = nullptr;
	AreaHelper = nullptr;

#if PLATFORM_ANDROID
	FCoreDelegates::ApplicationHasEnteredForegroundDelegate.AddUObject(this, &UTangoDevice::AppServiceResume);
	FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(this, &UTangoDevice::AppServicePause);
	Config_ = nullptr;
#endif

	bHasBeenPropelyInitialized = true;
	UE_LOG(TangoPlugin, Log, TEXT("UTangoDevice::ProperInitialize: FINISHED"));
}

#if PLATFORM_ANDROID
/*
 *  This function will populate the AppContextReference object with the Application context using a call to Java code via the JNI.
 *  @TODO: Investigate whether we need to call this on every connection or whether we can get away with only
 * triggering it once on app startup (i.e. will the application context switch? Is the bad global reference we
 * experienced when calling only at TangoDevice startup due to local references from the Unreal JNI pipeline being
 * converted into globals incorrectly, or because the Application context which is being pointed to has changed?)
 */
void UTangoDevice::PopulateAppContext()
{
	jobject AppContext = NULL;

	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		//We identify the method we need to call, by passing the name of the function.
		static jmethodID Method = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_GetAppContext", "()Landroid/content/Context;", false);
		//Once jmethodID has been identified, we supply it as an argument to CallObjectMethod.
		//Use CallObjectMethod for functions which return a jobject, CallIntMethod for functions which return an int, etc.
		AppContext = FJavaWrapper::CallObjectMethod(Env, FJavaWrapper::GameActivityThis, Method);

		if (AppContext == NULL)
		{
			UE_LOG(LogTemp, Warning, TEXT("UTangoDevice::PopulateAppContext: Error - app context is still NULL after retrieval call!"));
			AppContextReference = nullptr;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("UTangoDevice::PopulateAppContext: Success - appcontext jobject is not null!!"));
			//Now we cache the activity for later use to prevent garbage collection
			AppContextReference = Env->NewGlobalRef(AppContext);
		}
	}

	return;
}

void UTangoDevice::DePopulateAppContext()
{
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		if (AppContextReference != nullptr)
		{
			Env->DeleteGlobalRef(AppContextReference);
			AppContextReference = nullptr;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("UTangoDevice::PopulateAppContext: Success - appcontext jobject is not null!!"));
		}
	}

}
#endif

void UTangoDevice::DeallocateResources()
{
	UE_LOG(TangoPlugin, Log, TEXT("UTangoDevice::DeallocateResources: Called"));
#if PLATFORM_ANDROID
	if (Config_ != nullptr)
	{
		TangoConfig_free(Config_);
		Config_ = nullptr;
	}
	DePopulateAppContext();
#endif
	if (GetTangoDevicePointCloudPointer() != nullptr)
	{
		delete GetTangoDevicePointCloudPointer();
		PointCloudHelper = nullptr;
	}
	if (GetTangoDeviceMotionPointer() != nullptr)
	{
		GetTangoDeviceMotionPointer()->ConditionalBeginDestroy();
		MotionHelper = nullptr;
	}
	if (GetTangoDeviceImagePointer() != nullptr)
	{
		GetTangoDeviceImagePointer()->ConditionalBeginDestroy();
		ImageHelper = nullptr;
	}
	YTexture = nullptr;
	CbTexture = nullptr;
	bDataIsFilled = false;
}

UTangoDevice::~UTangoDevice()
{
	UE_LOG(TangoPlugin, Log, TEXT("UTangoDevice::Destructor: Called"));
	if (bHasBeenPropelyInitialized)
	{
		DeallocateResources();
	}
}

void UTangoDevice::BeginDestroy()
{
	Super::BeginDestroy();
	DeallocateResources();
}

bool UTangoDevice::IsTickable() const
{
	return bHasBeenPropelyInitialized;
}

void UTangoDevice::Tick(float DeltaTime)
{
	if (GetTangoDeviceImagePointer())
	{
		GetTangoDeviceImagePointer()->TickByDevice();
	}
	if (GetTangoDevicePointCloudPointer())
	{
		GetTangoDevicePointCloudPointer()->TickByDevice();
	}
	BroadCastEvents();
}

TStatId UTangoDevice::GetStatId() const
{
	return TStatId();
}

UTangoDevice& UTangoDevice::Get()
{
	if (!Instance)
	{
		Instance = NewObject<UTangoDevice>(UTangoDevice::StaticClass());
		Instance->ProperInitialize();
		Instance->SetFlags(RF_Transient);
		//This flag ensures the singleton is never garbage collected.
		if (Instance->IsSafeForRootSet())
		{
			Instance->SetInternalFlags(Instance->GetInternalFlags() | EInternalObjectFlags::RootSet);
		}
		else
		{
			UE_LOG(TangoPlugin, Error, TEXT("TangoDevice is not save for RootSet!"));
		}
	}

	return *Instance;
}

TangoDevicePointCloud * UTangoDevice::GetTangoDevicePointCloudPointer()
{
	return PointCloudHelper;
}

UTangoDeviceMotion * UTangoDevice::GetTangoDeviceMotionPointer()
{
	return MotionHelper;
}

UTangoDeviceImage * UTangoDevice::GetTangoDeviceImagePointer()
{
	return ImageHelper;
}

TangoDeviceAreaLearning * UTangoDevice::GetTangoDeviceAreaLearningPointer()
{
	return AreaHelper;
}

//START - Core Tango functions
bool UTangoDevice::IsTangoServiceRunning()
{
	return GetTangoServiceStatus() == CONNECTED;
}

UTangoDevice::ServiceStatus UTangoDevice::GetTangoServiceStatus()
{
	return ConnectionState;
}

void UTangoDevice::RestartService(FTangoConfig & Config, FTangoRuntimeConfig& RuntimeConfig)
{
	UE_LOG(TangoPlugin, Log, TEXT("UTangoDevice::RestartService: Called"));
	StopTangoService();
	StartTangoService(Config, RuntimeConfig);
}

void UTangoDevice::StartTangoService(FTangoConfig & Config, FTangoRuntimeConfig& RuntimeConfig)
{
	CurrentConfig = Config;
	CurrentRuntimeConfig = RuntimeConfig;
#if PLATFORM_ANDROID
	ConnectTangoService();
#endif
}


bool UTangoDevice::IsUsingAdf() const
{
	return CurrentConfig.AreaDescription.UUID.Len() > 0 || CurrentConfig.bUseCloudAdf;
}

bool UTangoDevice::SetTangoRuntimeConfig(FTangoRuntimeConfig Configuration, bool bPreRuntime)
{
	UE_LOG(TangoPlugin, Log, TEXT("UTangoDevice::SetTangoRuntimeConfig: Called."));
#if PLATFORM_ANDROID
	if (Config_ == nullptr)
	{
		UE_LOG(TangoPlugin, Error, TEXT("UTangoDevice::SetTangoRuntimeConfig: Unable to set runtime Config Tango Config pointer is nullptr."));
		return false;
	}
	if (!bPreRuntime && !IsTangoServiceRunning())
	{
		UE_LOG(TangoPlugin, Error, TEXT("UTangoDevice::SetTangoRuntimeConfig: Unable to set runtime Config since Tango Service is not running."));
		return false;
	}

#endif
	bool bSuccess = true;
	if (Configuration.bEnableDepth && !CurrentConfig.bEnableDepthCapabilities)
	{
		Configuration.bEnableDepth = false;
		UE_LOG(TangoPlugin, Warning, TEXT("UTangoDevice::SetTangoRuntimeConfig: Unable to set EnableDepth to RuntimeConfig since Config has no DepthCapabilites enabled"));
	}
#if PLATFORM_ANDROID
	UE_LOG(TangoPlugin, Log, TEXT("UTangoDevice::SetTangoRuntimeConfig: Rate is: %d "), (Configuration.bEnableDepth ? Configuration.RuntimeDepthFramerate : 0));
	bSuccess = TangoConfig_setInt32(Config_, "config_runtime_depth_framerate", Configuration.bEnableDepth ? Configuration.RuntimeDepthFramerate : 0) == TANGO_SUCCESS && bSuccess;
	if (!bPreRuntime)
	{
		UE_LOG(TangoPlugin, Log, TEXT("UTangoDevice::SetTangoRuntimeConfig: TangoService_setRuntimeConfig"));
		bSuccess = TangoService_setRuntimeConfig(Config_) && bSuccess;
	}
#endif


	if (Configuration.bEnableColorCamera && !CurrentConfig.bEnableColorCameraCapabilities)
	{
		Configuration.bEnableColorCamera = false;
		UE_LOG(TangoPlugin, Warning, TEXT("UTangoDevice::SetTangoRuntimeConfig: Unable to set EnableColorCamera to RuntimeConfig since Config has no ColorCameraCapabilites enabled"));
	}
	else if (CurrentConfig.bEnableColorCameraCapabilities && GetTangoDeviceImagePointer())
	{
		GetTangoDeviceImagePointer()->setRuntimeConfig(Configuration);
	}
	CurrentRuntimeConfig = Configuration;


	return bSuccess;
}

// Disconnect Tango Service.
void UTangoDevice::StopTangoService()
{
	UE_LOG(TangoPlugin, Log, TEXT("UTangoDevice::StopTangoService: Called"));
#if PLATFORM_ANDROID
	DisconnectTangoService();
#endif
}

FTangoConfig& UTangoDevice::GetCurrentConfig()
{
	return CurrentConfig;
}

FTangoRuntimeConfig& UTangoDevice::GetCurrentRuntimeConfig()
{
	return CurrentRuntimeConfig;
}

float UTangoDevice::GetMetersToWorldScale()
{
	return CurrentConfig.MetersToWorldScale;
}

FTangoCameraIntrinsics UTangoDevice::GetCameraIntrinsics(ETangoCameraType CameraID)
{
#if PLATFORM_ANDROID

	if (IsTangoServiceRunning())
	{
		TangoCameraIntrinsics DeviceIntrinsics;
		TangoErrorType RequestResult = TangoService_getCameraIntrinsics((TangoCameraId)(int)CameraID, &DeviceIntrinsics);
		if (RequestResult == TANGO_SUCCESS)
		{
			return FromCObject(DeviceIntrinsics);
		}
	}
#endif

	//If we're not on Android, or the request result was not successful, just return an empty struct
	return FTangoCameraIntrinsics();
}

bool UTangoDevice::IsLearningModeEnabled()
{
	return CurrentConfig.bEnableLearningMode && IsTangoServiceRunning();
}
//END - Core Tango functions

//BEGIN - Platform Only Functions


#if PLATFORM_ANDROID

template <typename T>
TangoErrorType DoTangoConfigSet(
	TangoErrorType(*Set)(TangoConfig, const char*, T),
	TangoConfig config, const char* key, T value)
{
	TangoErrorType Result = Set(config, key, value);
	if (Result != TANGO_SUCCESS)
	{
		UE_LOG(TangoPlugin, Warning, TEXT("Failed to set tango config: %s"), *(FString(key)));
	}
	return Result;
}

bool UTangoDevice::ApplyConfig()
{
	if (IsTangoServiceRunning())
	{
		UE_LOG(TangoPlugin, Error, TEXT(" UTangoDevice::ApplyConfig: Cannot ApplyConfig while TangoService is running!"));
		return false;
	}
	//Setting Up the Config:
	if (Config_ == nullptr)
	{
		Config_ = TangoService_getConfig(TANGO_CONFIG_DEFAULT);
		if (Config_ == nullptr)
		{
			UE_LOG(TangoPlugin, Error, TEXT(" UTangoDevice::ApplyConfig: SetConfig FAILED because TangoService_getConfig did return nullptr!"));
			return false;
		}
	}

	bool bSuccess = true;
	bSuccess = DoTangoConfigSet(&TangoConfig_setBool, Config_, "config_enable_auto_recovery", CurrentConfig.bEnableAutoRecovery) == TANGO_SUCCESS	&& bSuccess;
	bSuccess = DoTangoConfigSet(&TangoConfig_setBool, Config_, "config_enable_color_camera", CurrentConfig.bEnableColorCameraCapabilities) == TANGO_SUCCESS	&& bSuccess;
	bSuccess = DoTangoConfigSet(&TangoConfig_setBool, Config_, "config_color_mode_auto", CurrentConfig.bColorModeAuto) == TANGO_SUCCESS	&& bSuccess;
	bSuccess = DoTangoConfigSet(&TangoConfig_setBool, Config_, "config_enable_depth", CurrentConfig.bEnableDepthCapabilities) == TANGO_SUCCESS	&& bSuccess;
	bSuccess = DoTangoConfigSet(&TangoConfig_setBool, Config_, "config_high_rate_pose", CurrentConfig.bHighRatePose) == TANGO_SUCCESS	&& bSuccess;
	bSuccess = DoTangoConfigSet(&TangoConfig_setBool, Config_, "config_enable_learning_mode", CurrentConfig.bEnableLearningMode) == TANGO_SUCCESS	&& bSuccess;
	bSuccess = DoTangoConfigSet(&TangoConfig_setBool, Config_, "config_enable_low_latency_imu_integration", CurrentConfig.bLowLatencyIMUIntegration) == TANGO_SUCCESS	&& bSuccess;
	bSuccess = DoTangoConfigSet(&TangoConfig_setBool, Config_, "config_enable_motion_tracking", CurrentConfig.bEnableMotionTracking) == TANGO_SUCCESS	&& bSuccess;
	bSuccess = DoTangoConfigSet(&TangoConfig_setBool, Config_, "config_smooth_pose", CurrentConfig.bSmoothPose) == TANGO_SUCCESS	&& bSuccess;
	bSuccess = DoTangoConfigSet(&TangoConfig_setInt32, Config_, "config_color_exp", CurrentConfig.ColorExposure) == TANGO_SUCCESS	&& bSuccess;
	bSuccess = DoTangoConfigSet(&TangoConfig_setInt32, Config_, "config_color_iso", CurrentConfig.ColorISO) == TANGO_SUCCESS	&& bSuccess;
	if (CurrentConfig.bUseCloudAdf)
	{
		bool bEnabledCloudAdf = DoTangoConfigSet(&TangoConfig_setBool, Config_, "config_experimental_use_cloud_adf", CurrentConfig.bUseCloudAdf) == TANGO_SUCCESS;
		if (bEnabledCloudAdf)
		{
			UE_LOG(TangoPlugin, Log, TEXT("UTangoDevice: successfully enabled Cloud ADF"));
		}
		else
		{
			UE_LOG(TangoPlugin, Error, TEXT("UTangoDevice: failed to enable Cloud ADF"));
		}
		bSuccess = bEnabledCloudAdf && bSuccess;
	}
	if (!CurrentConfig.bUseCloudAdf && CurrentConfig.AreaDescription.UUID.Len() > 0)
	{
		bool bLoadedAreaDesc = TangoConfig_setString(Config_, "config_load_area_description_UUID", TCHAR_TO_ANSI(*CurrentConfig.AreaDescription.UUID)) == TANGO_SUCCESS;
		if (!bLoadedAreaDesc)
		{
			UE_LOG(TangoPlugin, Error, TEXT("UTangoDevice: Couldn't load area description %s"), *CurrentConfig.AreaDescription.UUID);
			bSuccess = false;
		}
		else
		{
			UE_LOG(TangoPlugin, Log, TEXT("UTangoDevice: Loaded area description %s: %s"), *CurrentConfig.AreaDescription.Filename, *CurrentConfig.AreaDescription.UUID);
		}
	}

	if (!bSuccess)
	{
		UE_LOG(TangoPlugin, Warning, TEXT(" UTangoDevice::ApplyConfig: ApplyConfig FAILED because the Config parameters could not be set."));
	}
	else
	{

	}

	FString ConfigString(TangoConfig_toString(Config_));
	UE_LOG(TangoPlugin, Log, TEXT("Tango Config: %d: %s"), bSuccess, *ConfigString);
	bSuccess = SetTangoRuntimeConfig(CurrentRuntimeConfig, true) && bSuccess;

	return bSuccess;
}

/*
 * This function sends a request to the Java layer to start the Tango service
 */
void UTangoDevice::ConnectTangoService()
{

	//@TODO: Call the Java RequestConnectToService function
	UE_LOG(TangoPlugin, Log, TEXT("UTangoDevice::ConnectTangoService: starting service binding request call."));
	//Make the call to Java

#if PLATFORM_ANDROID
	jobject AppContext = NULL;

	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{

		static jmethodID RequestTangoServiceMethod = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_RequestTangoService", "()V", false);

		FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, RequestTangoServiceMethod);
	}
	else
	{
		UE_LOG(TangoPlugin, Error, TEXT("UTangoDevice::ConnectTangoService: Could not get Java environment!"));
	}
#endif
	UE_LOG(TangoPlugin, Log, TEXT("UTangoDevice::ConnectTangoService: Finished sevice binding request call."));

}

/*
 * This function sends a request to the Java layer to start the Tango service
 */
void UTangoDevice::UnbindTangoService()
{

	//@TODO: Call the Java RequestConnectToService function
	UE_LOG(TangoPlugin, Log, TEXT("UTangoDevice::UnbindTangoService: starting service unbinding request call."));
	//Make the call to Java

#if PLATFORM_ANDROID
	jobject AppContext = NULL;

	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{

		static jmethodID TestJavaRoundTripMethod = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_UnbindTangoService", "()V", false);

		FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, TestJavaRoundTripMethod);
	}
	else
	{
		UE_LOG(TangoPlugin, Error, TEXT("UTangoDevice::UnbindTangoService: Could not get Java environment!"));
	}
#endif
	UE_LOG(TangoPlugin, Log, TEXT("UTangoDevice::UnbindTangoService: Finished sevice unbinding request call."));

}


#if PLATFORM_ANDROID
extern "C"
{
	//JavaThunkCpp: Native function which is called by Java code. Completes connection to the Tango service.
	JNIEXPORT void JNICALL
		Java_com_projecttango_plugin_TangoInterface_OnJavaTangoServiceConnected(JNIEnv* Env, jobject, jobject IBinder)
	{
		UE_LOG(LogTemp, Warning, TEXT("UTangoDevice::OnJavaServiceConnected: Success- C++ function called from Java!"));
		//Java service connection finished, complete C API connection	
		UTangoDevice::Get().BindAndCompleteConnectionToService(Env, IBinder);
	}
}
#endif



#if PLATFORM_ANDROID
/*
 *  This function should be called after the Tango service has been bound at the Java level.
 */
void UTangoDevice::BindAndCompleteConnectionToService(JNIEnv* Env, jobject IBinder)
{
	FScopeLock ScopeLock(&EventLock);
	//Bind to the Tango service
	if (TangoService_setBinder(Env, IBinder) != TANGO_SUCCESS)
	{
		UE_LOG(TangoPlugin, Error, TEXT(" UTangoDevice::BindAndCompleteConnectionToService: could not bind to Tango Service."));
	}
	else
	{
		UE_LOG(TangoPlugin, Log, TEXT(" UTangoDevice::BindAndCompleteConnectionToService: successfully bound to Tango Service."));
	}

	//Apply the service Configuration
	ApplyConfig();

	UE_LOG(TangoPlugin, Log, TEXT("UTangoDevice::BindAndCompleteConnectionToService: Called"));
	if (Config_ == nullptr)
	{
		UE_LOG(TangoPlugin, Error, TEXT(" UTangoDevice::BindAndCompleteConnectionToService: failed because Config is NULL."));
		return;
	}

	//Refresh Java global reference here
	/*@TODO: This is a little hacky, see if we can get a reference to a state which won't be garbage collected so we don't need to do this.*/
	PopulateAppContext();

	//Attempt to connect to the now bound service
	UE_LOG(TangoPlugin, Log, TEXT("UTangoDevice::BindAndCompleteConnectionToService: Connecting!"));
	ConnectionState = TangoService_connect(AppContextReference, Config_) == TANGO_SUCCESS ? CONNECTED : FAILED_TO_CONNECT;

	//Creating and deleting of additional class components that register callbacks and do their own thing
	if (CurrentConfig.bEnableDepthCapabilities && GetTangoDevicePointCloudPointer() == nullptr)
	{
		PointCloudHelper = new TangoDevicePointCloud(Config_);
	}
	else if (!CurrentConfig.bEnableDepthCapabilities && GetTangoDevicePointCloudPointer() != nullptr)
	{
		delete PointCloudHelper;
		PointCloudHelper = nullptr;
	}

	if (CurrentConfig.bEnableMotionTracking && GetTangoDeviceMotionPointer() == nullptr)
	{
		MotionHelper = NewObject<UTangoDeviceMotion>(UTangoDeviceMotion::StaticClass());
		MotionHelper->ProperInitialize();
	}
	else if (!CurrentConfig.bEnableMotionTracking && GetTangoDeviceMotionPointer() != nullptr)
	{
		MotionHelper->ConditionalBeginDestroy();
		MotionHelper = nullptr;
	}

	if (CurrentConfig.bEnableColorCameraCapabilities && GetTangoDeviceImagePointer() == nullptr)
	{
		ImageHelper = NewObject<UTangoDeviceImage>(UTangoDeviceImage::StaticClass());
		ImageHelper->Init(Config_);
	}
	else if (!CurrentConfig.bEnableColorCameraCapabilities && GetTangoDeviceImagePointer() != nullptr)
	{
		ImageHelper->ConditionalBeginDestroy();
		ImageHelper = nullptr;
	}

	//@todo: Investigate if 'AreaHelper' is overloaded, or if we need to move 'IsLocalized' into the motion component
	if (CurrentConfig.bEnableLearningMode && GetTangoDeviceAreaLearningPointer() == nullptr)
	{
		AreaHelper = new TangoDeviceAreaLearning();
	}
	else if (!CurrentConfig.bEnableLearningMode && GetTangoDeviceAreaLearningPointer() != nullptr)
	{
		delete AreaHelper;
		AreaHelper = nullptr;
	}

	if (GetTangoServiceStatus() == FAILED_TO_CONNECT)
	{
		UE_LOG(TangoPlugin, Error, TEXT("UTangoDevice::BindAndCompleteConnectionToService: Unable to connect!"));
	}
	else
	{
		UE_LOG(TangoPlugin, Log, TEXT("UTangoDevice::BindAndCompleteConnectionToService: Connection succesfull! Now connecting callbacks!"));
		if (GetTangoDeviceMotionPointer() != nullptr)
		{
			GetTangoDeviceMotionPointer()->ConnectCallback();
		}
		if (GetTangoDevicePointCloudPointer() != nullptr)
		{
			GetTangoDevicePointCloudPointer()->ConnectCallback();
		}
		if (GetTangoDeviceImagePointer() != nullptr && CurrentRuntimeConfig.bEnableColorCamera)
		{
			GetTangoDeviceImagePointer()->ConnectCallback();
		}
	}
	ConnectEventCallback();//Activate Events
	UE_LOG(TangoPlugin, Log, TEXT("UTangoDevice::BindAndCompleteConnectionToService: FINISHED"));
	RunOnMainThread([this]()-> void {
		BroadCastConnect();
	});
	//Call a second time to make depth disabling at startup work.
	if (!CurrentRuntimeConfig.bEnableDepth && CurrentConfig.bEnableDepthCapabilities)
	{
		SetTangoRuntimeConfig(CurrentRuntimeConfig);
	}
}
#endif

void UTangoDevice::DisconnectTangoService(bool bByAppServicePause)
{
	UE_LOG(TangoPlugin, Log, TEXT("UTangoDevice::DisconnectTangoService: Called"));

	if (GetTangoServiceStatus() == CONNECTED)
	{
		UE_LOG(TangoPlugin, Log, TEXT(" UTangoDevice::DisconnectTangoService: will now disconnect!"));

		//Disconnect from the C service
		TangoService_disconnect();

		//Unbind from the Java-level service after the TangoService_disconnect call
		UnbindTangoService();

		//Mark the current connection state and broadcast the disconnection
		ConnectionState = bByAppServicePause ? DISCONNECTED_BY_APPSERVICEPAUSE : DISCONNECTED;
		BroadCastDisconnect();
	}
	else
	{
		DeallocateResources();
		UE_LOG(TangoPlugin, Warning, TEXT(" UTangoDevice::DisconnectTangoService: Tango is not registered as connected, and is therefore not disconnecting properly!!"));
	}
	UE_LOG(TangoPlugin, Log, TEXT("UTangoDevice::DisconnectTangoService: FINISHED"));
}

//END - Platform Only Functions

void UTangoDevice::AppServiceResume()
{
	UE_LOG(TangoPlugin, Log, TEXT("UTangoDevice::AppServiceResume: Called"));
	if (GetTangoServiceStatus() == DISCONNECTED_BY_APPSERVICEPAUSE)//We only reconnect when we disconnected the Tango by AppServicePause first!
	{
		UE_LOG(TangoPlugin, Log, TEXT(" UTangoDevice::AppServiceResume: Connect service called"));
		//ApplyConfig();
		ConnectTangoService();
	}
	UE_LOG(TangoPlugin, Log, TEXT("UTangoDevice::AppServiceResume: FINISHED"));

}

void UTangoDevice::AppServicePause()
{
	UE_LOG(TangoPlugin, Log, TEXT("UTangoDevice::AppServicePause: Called"));
	if (false && IsTangoServiceRunning()) // temporary hack: motion tracking doesn't recover in resume in Tango Z-release
	{
		DeallocateResources();
		DisconnectTangoService(true);
	}
	UE_LOG(TangoPlugin, Log, TEXT("UTangoDevice::AppServicePause: FINISHED"));

}

#endif

//TangoDeviceMotion Helper

void UTangoDevice::AddTangoMotionComponent(UTangoMotionComponent* Component, TArray<FTangoCoordinateFramePair>& Requests)
{
	UE_LOG(TangoPlugin, Log, TEXT("UTangoDevice::AddTangoMotionComponent: Started"));
	//Check if component is already added and remove components that have been garbage collected
	bool bComponentFound = false;
	for (int i = 0; i < MotionComponents.Num(); ++i)
	{
		while (MotionComponents[i] == nullptr)
		{
			MotionComponents.RemoveAt(i);
			RequestedPairs.RemoveAt(i);
			if (i >= MotionComponents.Num())
			{
				break;
			}
		}
		if (i >= MotionComponents.Num())
		{
			break;
		}
		if (MotionComponents[i] == Component)
		{
			bComponentFound = true;
			RequestedPairs[i] = Requests;
		}
	}
	if (!bComponentFound)
	{
		UE_LOG(TangoPlugin, Log, TEXT("UTangoDevice::AddTangoMotionComponent: Not Found"));
		MotionComponents.Add(Component);
		RequestedPairs.Add(Requests);
	}
	else
	{
		UE_LOG(TangoPlugin, Log, TEXT("UTangoDevice::AddTangoMotionComponent: Found"));
	}
	if (GetTangoDeviceMotionPointer() != nullptr)
	{
		UE_LOG(TangoPlugin, Log, TEXT("UTangoDevice::AddTangoMotionComponent: Initiating Rebuild"));
		GetTangoDeviceMotionPointer()->CheckForChangeInRequests();
	}
}