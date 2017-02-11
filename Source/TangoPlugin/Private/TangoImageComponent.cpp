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
#include "TangoImageComponent.h"
#include "TangoDevice.h"
#include "TangoDataTypes.h"

UTangoImageComponent::UTangoImageComponent() : Super()
{
	bWantsInitializeComponent = false;
	PrimaryComponentTick.bCanEverTick = true;
}

void UTangoImageComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (UTangoDevice::Get().GetTangoDeviceImagePointer())
	{
		if (UTangoDevice::Get().GetTangoDeviceImagePointer()->VideoTexture)
		{
			float TimeStamp = UTangoDevice::Get().GetTangoDeviceImagePointer()->GetImageBufferTimestamp();
			if (TimeStamp != LastBroadCastedTimestamp)
			{
				LastBroadCastedTimestamp = TimeStamp;
				OnTangoImageAvailable.Broadcast(TimeStamp);
			}
		}
	}
}

UTexture* UTangoImageComponent::GetCameraTexture(float& Timestamp, bool& bIsValid)
{
	UTexture * Texture = nullptr;
	bIsValid = false;
	Timestamp = 0.0;

	if (UTangoDevice::Get().GetTangoDeviceImagePointer())
	{
		Timestamp = UTangoDevice::Get().GetTangoDeviceImagePointer()->GetImageBufferTimestamp();
		Texture = UTangoDevice::Get().GetTangoDeviceImagePointer()->VideoTexture;
		bIsValid = Texture != nullptr;
	}

	return Texture;
}

float UTangoImageComponent::GetLatestImageTimeStamp()
{
	if (UTangoDevice::Get().GetTangoDeviceImagePointer() == nullptr)
	{
		UE_LOG(TangoPlugin, Warning, TEXT("UTangoImageComponent::GetLatestImageTimeStamp: Color Camera is not enabled"));
		return 0.0f;
	}
	else
	{
		return UTangoDevice::Get().GetTangoDeviceImagePointer()->GetImageBufferTimestamp();
	}
}
