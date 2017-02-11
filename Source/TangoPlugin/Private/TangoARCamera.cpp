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
#include "TangoARCamera.h"
#include "TangoDevice.h"
#include "TangoARHelpers.h"

UTangoARCamera::~UTangoARCamera()
{
}

UTangoARCamera::UTangoARCamera(const FObjectInitializer& Init) : ARScreen(nullptr), bScreenIsVisible(true), Super(Init)
{
	PrimaryComponentTick.bCanEverTick = true;
	bWantsInitializeComponent = true;
	FrameOfReference = FTangoCoordinateFramePair(ETangoCoordinateFrameType::START_OF_SERVICE, ETangoCoordinateFrameType::CAMERA_COLOR);
}


AActor * UTangoARCamera::GetActor()
{
	return Super::GetOwner();
}

USceneComponent * UTangoARCamera::AsSceneComponent()
{
	return Cast<USceneComponent>(this);
}

void UTangoARCamera::BeginPlay()
{
	Super::BeginPlay();
}

void UTangoARCamera::InitializeComponent()
{
	Super::InitializeComponent();
}

void UTangoARCamera::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
#if PLATFORM_ANDROID
	if (UTangoDevice::Get().IsUsingAdf())
	{
		FrameOfReference = FTangoCoordinateFramePair(ETangoCoordinateFrameType::AREA_DESCRIPTION, ETangoCoordinateFrameType::CAMERA_COLOR);
	}
	else
	{
		FrameOfReference = FTangoCoordinateFramePair(ETangoCoordinateFrameType::START_OF_SERVICE, ETangoCoordinateFrameType::CAMERA_COLOR);
	}

	if (ARScreen == nullptr && TangoARHelpers::DataIsReady())
	{
		auto Intrin = TangoARHelpers::GetARCameraIntrinsics();
		FieldOfView = FMath::RadiansToDegrees<float>(2.0f * FMath::Atan(0.5f * Intrin.Width / Intrin.Fx));
		ARScreen = NewObject<UTangoARScreenComponent>(GetOwner());
		if (ARScreen != nullptr)
		{
			ARScreen->RegisterComponent();
			ARScreen->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
			FVector2D LowerLeft, UpperRight, NearFar;
			TangoARHelpers::GetNearPlane(LowerLeft, UpperRight, NearFar);
			FVector2D UVShift = TangoARHelpers::GetARUVShift();

			//NearFar.Y *= 0.99f;
			FVector LL = FVector(NearFar.Y, LowerLeft.X*(NearFar.Y / NearFar.X), LowerLeft.Y*(NearFar.Y / NearFar.X));
			FVector UR = FVector(NearFar.Y, UpperRight.X*(NearFar.Y / NearFar.X), UpperRight.Y*(NearFar.Y / NearFar.X));

			ARScreen->SetRelativeLocation((LL + UR)*0.5f);
			ARScreen->SetRelativeRotation(FQuat::Identity);
			ARScreen->SetRelativeScale3D(FVector(1, FMath::Abs(UR.Y - LL.Y) / (100.0f*(1.0f - 2.0f*UVShift.X)), FMath::Abs(LL.Z - UR.Z) / (100.0f*(1.0f - 2.0f*UVShift.Y))));
			FTransform Transform = ARScreen->GetRelativeTransform();
			UE_LOG(TangoPlugin, Log, TEXT("UTangoARCamera::TickComponent: Created ARScreen w: %d, h: %d Pos %s, Scale %s"),  Intrin.Width, Intrin.Height, *Transform.GetTranslation().ToString(), *Transform.GetScale3D().ToString());	
		}
		else
		{
			UE_LOG(TangoPlugin, Error, TEXT("UTangoARCamera::TickComponent: Could not instantiate TangoARScreen for TangoARCamera. There will be no camera passthrough."));
		}
	} 
	else if (ARScreen != nullptr && !TangoARHelpers::DataIsReady())
	{
		ARScreen->DestroyComponent();
		ARScreen = nullptr;
		UE_LOG(TangoPlugin, Log, TEXT("UTangoARCamera::TickComponent: Removed ARScreen"));
	}
	if (ARScreen != nullptr && UTangoDevice::Get().GetTangoDeviceMotionPointer())
	{
		if (bScreenIsVisible != ARScreen->bVisible)
		{
			ARScreen->bVisible = bScreenIsVisible;
		}
		UTangoDevice::Get().GetTangoDeviceImagePointer()->TickByCamera([this](double CameraTimestamp) -> void {
			//UE_LOG(TangoPlugin, Log, TEXT("Camera: %f"), CameraTimestamp);
			FTangoPoseData LatestPose = UTangoDevice::Get().GetTangoDeviceMotionPointer()->GetPoseAtTime(FrameOfReference, CameraTimestamp);
			//Only move the component if the pose status is valid.
			if (LatestPose.StatusCode == ETangoPoseStatus::VALID)
			{
				AActor* Owner = GetOwner();
				Owner->SetActorLocation(LatestPose.Position);
				Owner->SetActorRotation(LatestPose.Rotation);
				//UE_LOG(TangoPlugin, Log, TEXT("Set Pose %s %s, %s"), *Owner->GetName(), *LatestPose.Position.ToString(), *LatestPose.Rotation.ToString());
			}
			else
			{
				//UE_LOG(TangoPlugin, Warning, TEXT("Camera pose not valid: %d, frame of reference: %d, %d"), (int32)LatestPose.StatusCode, (int32)FrameOfReference.BaseFrame, (int32)FrameOfReference.TargetFrame);
			}
		});
		
	}
	else
	{
		//UE_LOG(TangoPlugin, Warning, TEXT("UTangoARCamera::TickComponent: Could not update transfrom because Tango Service is not connect or has motion tracking disabled!"));
	}
#endif
}
