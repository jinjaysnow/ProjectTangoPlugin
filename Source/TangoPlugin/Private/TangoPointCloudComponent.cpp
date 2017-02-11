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
#include "TangoPointCloudComponent.h"

UTangoPointCloudComponent::UTangoPointCloudComponent() : Super()
{
}

void UTangoPointCloudComponent::BeginPlay()
{
	UTangoDevice::Get().PointCloudComponents.Add(this);
	Super::BeginPlay();
}

int32 UTangoPointCloudComponent::GetMaxPointCount()
{
	if (UTangoDevice::Get().GetTangoDevicePointCloudPointer() == nullptr)
	{
		return 0;
	}
	else
	{
		return UTangoDevice::Get().GetTangoDevicePointCloudPointer()->GetMaxVertexCapacity();
	}
}

float UTangoPointCloudComponent::GetPointCloudTimestamp()
{
	if (UTangoDevice::Get().GetTangoDevicePointCloudPointer() == nullptr)
	{
		return 0;
	}
	else
	{
#if PLATFORM_ANDROID
		auto PointCloud = UTangoDevice::Get().GetTangoDevicePointCloudPointer()->GetLatestPointCloud();
		if (PointCloud) 
		{
			return PointCloud->timestamp;
		}
#endif
	}
	return 0;
}

bool UTangoPointCloudComponent::FitPlane(const FVector2D& ScreenPoint, FTransform& Result)
{
#if PLATFORM_ANDROID

	if (UTangoDevice::Get().GetTangoDevicePointCloudPointer() != nullptr)
	{
		UWorld* World = GetOwner()->GetWorld();
		if (World && World->IsGameWorld())
		{
			if (UGameViewportClient* ViewportClient = World->GetGameViewport())
			{
				FVector2D ViewportSize;
				ViewportClient->GetViewportSize(ViewportSize);
				return UTangoDevice::Get().GetTangoDevicePointCloudPointer()->FitPlane(1.0f - ScreenPoint.X / ViewportSize.X, 1.0f - ScreenPoint.Y / ViewportSize.Y, Result);
            }
        }	
	}
#endif
	return false;
}

