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
#include "TangoMotionComponent.h"
#include "TangoImageComponent.h"
#include "TangoDevice.h"

UTangoMotionComponent::~UTangoMotionComponent()
{
}

UTangoMotionComponent::UTangoMotionComponent() : Super()
{
	LatestPoseTimeStamp = 0.0;
	bWantsInitializeComponent = true;
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

FWGS_84_PoseData UTangoMotionComponent::GetWGS_84_PoseAtTime(ETangoCoordinateFrameType TargetFrame, float Timestamp)
{
	return UTangoDevice::Get().GetTangoDeviceMotionPointer() != nullptr ? UTangoDevice::Get().GetTangoDeviceMotionPointer()->GetWGS_84_PoseAtTime(TargetFrame, Timestamp) : FWGS_84_PoseData();
}

FTangoPoseData UTangoMotionComponent::GetTangoPoseAtTime(FTangoCoordinateFramePair FrameOfReference, float Timestamp)
{
	return UTangoDevice::Get().GetTangoDeviceMotionPointer() != nullptr ? UTangoDevice::Get().GetTangoDeviceMotionPointer()->GetPoseAtTime(FrameOfReference, Timestamp) : FTangoPoseData();
}

FTransform UTangoMotionComponent::GetComponentTransformAtTime(float Timestamp)
{
	auto Pose = GetTangoPoseAtTime(MotionComponentFrameOfReference, Timestamp);
	return CalcNewComponentToWorld(FTransform(Pose.QuatRotation, Pose.Position, RelativeScale3D));
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


bool UTangoMotionComponent::IsCurrentlyTracking()
{
	if (UTangoDevice::Get().GetTangoDeviceMotionPointer())
	{
		return UTangoDevice::Get().GetTangoDeviceMotionPointer()->IsLocalized(false);
	}
	else
	{
		//UE_LOG(TangoPlugin, Warning, TEXT("UTangoMotionComponent::IsLocalized: Tango Motion tracking not enabled"));
		return false;
	}
}

bool UTangoMotionComponent::IsLocalized()
{
	if (UTangoDevice::Get().GetTangoDeviceMotionPointer())
	{
		return UTangoDevice::Get().GetTangoDeviceMotionPointer()->IsLocalized(true);
	}
	else
	{
		//UE_LOG(TangoPlugin, Warning, TEXT("UTangoMotionComponent::IsLocalized: Tango Motion tracking not enabled"));
		return false;
	}
}

void UTangoMotionComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
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
