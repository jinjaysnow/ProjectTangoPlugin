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

#pragma once

#include "TangoMotionComponent.h"
#include "TangoCoordinateConversions.h"

#if PLATFORM_ANDROID
#include "tango_client_api.h"
#endif

#include "TangoDeviceMotion.generated.h"

/**
 * 
 */
UCLASS(NotBlueprintable, NotPlaceable, Transient)
class UTangoDeviceMotion : public UObject, public FTickableGameObject
{
	GENERATED_BODY()
public:
	UTangoDeviceMotion();
	void ProperInitialize();
	void ConnectCallback();
	virtual void BeginDestroy() override;

	//FTickableGameObject interface
	virtual bool IsTickable() const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	//Tango Motion functions
	FTangoPoseData GetPoseAtTime(FTangoCoordinateFramePair FrameOfReference, float Timestamp);
	FWGS_84_PoseData GetWGS_84_PoseAtTime(const ETangoCoordinateFrameType TargetFrame, float Timestamp);
	
	void ResetMotionTracking();

	bool IsLocalized();

	void CheckForChangeInRequests();

private:
	bool bIsProperlyInitialized = false;

#if PLATFORM_ANDROID
	void OnPoseAvailable(const TangoPoseData * Pose);
#endif

	FCriticalSection PoseMutex;


	bool bCallbackIsConnected = false;

	struct MotionEventRequestedFramePair
	{
		TangoSpaceConversions::TangoSpaceConversionPair RequestedSpace;
		TArray<int32> ComponentIDs;
	};

	TMap<FTangoCoordinateFramePair, TMap<FTangoCoordinateFramePair,MotionEventRequestedFramePair>> RequestedPairs;
	//Stuff used in the Tangothread:
	TMap<FTangoCoordinateFramePair, FTangoPoseData> BroadcastTangoPoseData;
};
