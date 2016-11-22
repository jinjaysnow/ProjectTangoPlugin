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
#include "TangoViewExtension.h"
#include "TangoMotionComponent.h"
#include "TangoImageComponent.h"
#include "TangoDevice.h"

UTangoMotionComponent::UTangoMotionComponent() : Super()
{
	LatestPoseTimeStamp = 0.0;
	bWantsInitializeComponent = true;
	bWantsBeginPlay = false;
	PrimaryComponentTick.bCanEverTick = true;
}

void UTangoMotionComponent::InitializeComponent()
{
	UE_LOG(TangoPlugin, Warning, TEXT("UTangoMotionComponent::InitializeComponent: Called!"));
	Super::InitializeComponent();
}

void UTangoMotionComponent::BeginDestroy()
{
	Super::BeginDestroy();
}

void UTangoMotionComponent::SetupPoseEvents(TArray<FTangoCoordinateFramePair> FramePairs)
{
	UTangoDevice::Get().AddTangoMotionComponent(this, FramePairs);
}

FTangoPoseData UTangoMotionComponent::GetTangoPoseAtTime(FTangoCoordinateFramePair FrameOfReference, float Timestamp)
{
	return UTangoDevice::Get().GetTangoDeviceMotionPointer() != nullptr ? UTangoDevice::Get().GetTangoDeviceMotionPointer()->GetPoseAtTime(FrameOfReference, Timestamp) : FTangoPoseData();
}

FTransform UTangoMotionComponent::GetComponentTransformAtTime(float Timestamp)
{
	auto Pose = GetTangoPoseAtTime(MotionComponentFrameOfReference, Timestamp);
	return CalcNewComponentToWorld(FTransform(Pose.QuatRotation, Pose.Position,RelativeScale3D));
}

ETangoPoseStatus UTangoMotionComponent::GetTangoPoseStatus(float& Timestamp)
{
	FTangoPoseData LatestPose = UTangoDevice::Get().GetTangoDeviceMotionPointer() != nullptr ? UTangoDevice::Get().GetTangoDeviceMotionPointer()->GetPoseAtTime(MotionComponentFrameOfReference, 0) : FTangoPoseData();
	Timestamp = LatestPose.Timestamp;
	return LatestPose.StatusCode;
}

void UTangoMotionComponent::ResetMotionTracking()
{
	if(UTangoDevice::Get().GetTangoDeviceMotionPointer() != nullptr)
		UTangoDevice::Get().GetTangoDeviceMotionPointer()->ResetMotionTracking();
}

bool UTangoMotionComponent::IsLocalized()
{
	if (UTangoDevice::Get().GetTangoDeviceMotionPointer())
	{
		return UTangoDevice::Get().GetTangoDeviceMotionPointer()->IsLocalized();
	}
	else
	{
		UE_LOG(TangoPlugin, Warning, TEXT("UTangoMotionComponent::IsLocalized: Tango Motion tracking not enabled"));
		return false;
	}
}

void UTangoMotionComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (UTangoDevice::Get().GetTangoDeviceMotionPointer())
	{
		FTangoPoseData LatestPose = UTangoDevice::Get().GetTangoDeviceMotionPointer()->GetPoseAtTime(MotionComponentFrameOfReference, 0);
		//Only move the component if the pose status is valid.
		if (LatestPose.StatusCode == ETangoPoseStatus::VALID)
		{
			SetRelativeLocation(LatestPose.Position);
			SetRelativeRotation(FRotator(LatestPose.QuatRotation));
		}
	}
	else
	{
#if PLATFORM_ANDROID
		UE_LOG(TangoPlugin, Warning, TEXT("UTangoMotionComponent::LateUpdate: Could not update transfrom because Tango Service is not connect or has motion tracking disabled!"));
#endif
	}

	if (!ViewExtension.IsValid() && GEngine)
	{
		TSharedPtr< FTangoViewExtension, ESPMode::ThreadSafe > NewViewExtension(new FTangoViewExtension(Cast<ITangoARInterface>(this)));
		ViewExtension = NewViewExtension;
		GEngine->ViewExtensions.Add(ViewExtension);
	}
}

FTangoPoseData UTangoMotionComponent::GetCurrentPoseRENDERTHREAD(float TimeStamp)
{
	if (UTangoDevice::Get().GetTangoDeviceMotionPointer() == nullptr)
	{
		return FTangoPoseData();
	}
	return UTangoDevice::Get().GetTangoDeviceMotionPointer()->GetPoseAtTime(MotionComponentFrameOfReference, 0);
}

AActor * UTangoMotionComponent::GetActor()
{
	return GetOwner();
}

USceneComponent * UTangoMotionComponent::AsSceneComponent()
{
	return Cast<USceneComponent>(this);
}

FTransform UTangoMotionComponent::CalcComponentToWorld(FTransform Transform)
{
	return CalcNewComponentToWorld(Transform);
}
