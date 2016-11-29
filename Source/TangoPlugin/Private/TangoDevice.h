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
	bool SetTangoRuntimeConfig(FTangoRuntimeConfig Configuration, bool bPreRuntime = false);
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
	bool IsUsingAdf() const;
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

	// ARHelpers data

	bool bDataIsFilled = false;
	FTangoCameraIntrinsics ColorCameraIntrinsics;
	FMatrix ProjectionMatrix;
	FVector2D UVShift;
	FVector2D NearPlaneLowerLeft, NearPlaneUpperRight;
	FVector2D NearFarDistance;

	static FMatrix FrustumMatrix(float Left, float Right, float Bottom, float Top, float NearVal, float FarVal)
	{
		FMatrix Result;
		Result.SetIdentity();
		Result.M[0][0] = (2.0f * NearVal) / (Right - Left);
		Result.M[1][1] = (2.0f * NearVal) / (Top - Bottom);
		Result.M[2][0] = -(Right + Left) / (Right - Left);
		Result.M[2][1] = -(Top + Bottom) / (Top - Bottom);
		Result.M[2][2] = FarVal / (FarVal - NearVal);
		Result.M[2][3] = 1.0f;
		Result.M[3][2] = -(FarVal * NearVal) / (FarVal - NearVal);
		Result.M[3][3] = 0;

		return Result;
	}

	void GetNearProjectionPlane(const FIntPoint ViewPortSize)
	{
		float WidthRatio = (float)ViewPortSize.X / (float)ColorCameraIntrinsics.Width;
		float HeightRatio = (float)ViewPortSize.Y / (float)ColorCameraIntrinsics.Height;

		float UOffset, VOffset;
		if (WidthRatio >= HeightRatio)
		{
			UOffset = 0;
			VOffset = (1 - (HeightRatio / WidthRatio)) / 2;
		}
		else
		{
			UOffset = (1 - (WidthRatio / HeightRatio)) / 2;
			VOffset = 0;
		}
		UVShift.X = UOffset;
		UVShift.Y = VOffset;
		UE_LOG(TangoPlugin, Log, TEXT("FTangoViewExtension::GetNearProjectionPlane: UVShift: %f %f"), UOffset, VOffset);
		UOffset = 0.05f;
		VOffset = 0.0f;

		float XScale = NearFarDistance.X / ColorCameraIntrinsics.Fx;
		float YScale = NearFarDistance.X / ColorCameraIntrinsics.Fy;

		NearPlaneLowerLeft.X = (-ColorCameraIntrinsics.Cx + (UOffset * ColorCameraIntrinsics.Width))*XScale;
		NearPlaneUpperRight.X = (ColorCameraIntrinsics.Width - ColorCameraIntrinsics.Cx - (UOffset * ColorCameraIntrinsics.Width))*XScale;
		// OpenGL coordinates has y pointing downwards so we negate this term.
		NearPlaneLowerLeft.Y = (-ColorCameraIntrinsics.Height + ColorCameraIntrinsics.Cy + (VOffset * ColorCameraIntrinsics.Height))*YScale;
		NearPlaneUpperRight.Y = (ColorCameraIntrinsics.Cy - (VOffset * ColorCameraIntrinsics.Height))*YScale;
	}

	FMatrix GetProjectionMatrix()
	{
		FMatrix OffAxisProjectionMatrix = FrustumMatrix(NearPlaneLowerLeft.X, NearPlaneUpperRight.X, NearPlaneLowerLeft.Y, NearPlaneUpperRight.Y, NearFarDistance.X, NearFarDistance.Y);

		FMatrix MatFlipZ;
		MatFlipZ.SetIdentity();
		MatFlipZ.M[2][2] = -1.0f;
		MatFlipZ.M[3][2] = 1.0f;

		FMatrix result = OffAxisProjectionMatrix * MatFlipZ;
		result.M[2][2] = 0.0f;
		result.M[3][0] = 0.0f;
		result.M[3][1] = 0.0f;
		result *= 1.0f / result.M[0][0];
		result.M[3][2] = NearFarDistance.X;
		return result;
	}

	bool FillARData()
	{
		if (bDataIsFilled)
		{
			return true;
		}
		ColorCameraIntrinsics = UTangoDevice::Get().GetCameraIntrinsics(ETangoCameraType::COLOR);
		if (UTangoDevice::Get().IsTangoServiceRunning() && GEngine)
		{
			if (UTangoDevice::Get().GetTangoDeviceMotionPointer() && GEngine->GameViewport)
			{
				if (GEngine->GameViewport->Viewport)
				{
					NearFarDistance.X = GNearClippingPlane;
					NearFarDistance.Y = 12000.0f;

					GetNearProjectionPlane(GEngine->GameViewport->Viewport->GetSizeXY());

					ProjectionMatrix = GetProjectionMatrix();
					bDataIsFilled = true;
				}
			}
		}
		return false;
	}


	FMatrix GetUnadjustedProjectionMatrix()
	{
		FillARData();
		return FrustumMatrix(NearPlaneLowerLeft.X, NearPlaneUpperRight.X, NearPlaneLowerLeft.Y, NearPlaneUpperRight.Y, NearFarDistance.X, NearFarDistance.Y);
	}

	FTangoCameraIntrinsics GetARCameraIntrinsics()
	{
		FillARData();
		return ColorCameraIntrinsics;
	}

	FVector2D GetARUVShift()
	{
		FillARData();
		return UVShift;
	}

	FMatrix GetARProjectionMatrix()
	{
		FillARData();
		return ProjectionMatrix;
	}

	void GetNearPlane(FVector2D& LowerLeft, FVector2D& UpperRight, FVector2D& NearFarPlaneDistance)
	{
		FillARData();
		LowerLeft = NearPlaneLowerLeft;
		UpperRight = NearPlaneUpperRight;
		NearFarPlaneDistance = NearFarDistance;
	}

	bool DataIsReady()
	{
		if (!IsTangoServiceRunning())
		{
			return bDataIsFilled = false;
		}
		FillARData();
		return bDataIsFilled;
	}

};
