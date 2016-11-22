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
#include "TangoDataTypes.h"
#include "TangoFromToCObject.h"

#include <UnrealTemplate.h>

#if PLATFORM_ANDROID
//#include "Private/Android/AndroidJNI.h"
#include "Android/AndroidJNI.h"
#include "AndroidApplication.h"
#endif

void UTangoDevice::ConnectEventCallback()
{
#if PLATFORM_ANDROID
	if (TangoService_connectOnTangoEvent([](void* context, const TangoEvent* Event) {UTangoDevice::Get().OnTangoEvent(Event); }) != TANGO_SUCCESS)
	{
		UE_LOG(TangoPlugin, Error, TEXT("UTangoDevice::ConnectEventCallback: TangoService_connectOnTangoEvent failed."));
	}
#endif
}

void UTangoDevice::BroadCastConnect()
{
	for (int i = 0; i < TangoEventComponents.Num(); ++i)
	{
		while (TangoEventComponents[i] == nullptr)
		{
			TangoEventComponents.RemoveAt(i);
			if (i >= TangoEventComponents.Num())
			{
				return;
			}
		}
		TangoEventComponents[i]->OnTangoConnect.Broadcast();
	}
}

void UTangoDevice::BroadCastDisconnect()
{
	for (int i = 0; i < TangoEventComponents.Num(); ++i)
	{
		while (TangoEventComponents[i] == nullptr)
		{
			TangoEventComponents.RemoveAt(i);
			if (i >= TangoEventComponents.Num())
			{
				return;
			}
		}
		TangoEventComponents[i]->OnTangoDisconnect.Broadcast();
	}
}

void UTangoDevice::BroadCastEvents()
{
	{
		FScopeLock ScopeLock(&EventLock);
		CurrentEventsCopy = TArray<FTangoEvent>(CurrentEvents);
		CurrentEvents.Empty();
	}
	for (int e = 0; e < CurrentEventsCopy.Num(); ++e)
	{
		for (int i = 0; i < TangoEventComponents.Num(); ++i)
		{
			while (TangoEventComponents[i] == nullptr)
			{
				TangoEventComponents.RemoveAt(i);
				if (i >= TangoEventComponents.Num())
				{
					return;
				}
			}

			switch (CurrentEventsCopy[e].Key)
			{
			case ETangoEventKeyType::KEY_SERVICE_EXCEPTION:
				TangoEventComponents[i]->OnTangoServiceException.Broadcast(CurrentEventsCopy[e]);
				break;
			case ETangoEventKeyType::DESCRIPTION_FISHEYE_OVER_EXPOSED:
				TangoEventComponents[i]->OnFisheyeOverExposed.Broadcast(CurrentEventsCopy[e]);
				break;
			case ETangoEventKeyType::DESCRIPTION_FISHEYE_UNDER_EXPOSED:
				TangoEventComponents[i]->OnFisheyeUnderExposed.Broadcast(CurrentEventsCopy[e]);
				break;
			case ETangoEventKeyType::DESCRIPTION_COLOR_OVER_EXPOSED:
				TangoEventComponents[i]->OnColorOverExposed.Broadcast(CurrentEventsCopy[e]);
				break;
			case ETangoEventKeyType::DESCRIPTION_COLOR_UNDER_EXPOSED:
				TangoEventComponents[i]->OnColorUnderExposed.Broadcast(CurrentEventsCopy[e]);
				break;
			case ETangoEventKeyType::DESCRIPTION_TOO_FEW_FEATURES:
				TangoEventComponents[i]->OnTooFewFeaturesTracked.Broadcast(CurrentEventsCopy[e]);
				break;
			case ETangoEventKeyType::KEY_AREA_DESCRIPTION_SAVE_PROGRESS:
				TangoEventComponents[i]->OnAreaDescriptionSaveProgress.Broadcast(CurrentEventsCopy[e]);
				break;
			case ETangoEventKeyType::UNKOWN:
				TangoEventComponents[i]->OnUnknownEvent.Broadcast(CurrentEventsCopy[e]);
				break;
            //Events for import and export results (success, cancelled, declined)
            case ETangoEventKeyType::IMPORT_RESULT:
                    TangoEventComponents[i]->OnFileImportEvent.Broadcast((ETangoRequestResult) FCString::Atoi(*(CurrentEventsCopy[e].Message)));
                break;
            case ETangoEventKeyType::EXPORT_RESULT:
                    TangoEventComponents[i]->OnFileExportEvent.Broadcast((ETangoRequestResult) FCString::Atoi(*(CurrentEventsCopy[e].Message)));
                break;
			default:
				break;
			}
		}
	}
	CurrentEventsCopy.Empty();
}

#if PLATFORM_ANDROID
void UTangoDevice::OnTangoEvent(const TangoEvent *Event)
{
	FScopeLock ScopeLock(&EventLock);
	CurrentEvents.Emplace(FromCObject(Event));
}

void UTangoDevice::PushTangoEvent(const FTangoEvent Event)
{
    FScopeLock ScopeLock(&EventLock);
    CurrentEvents.Emplace(Event);
}
#endif

void UTangoDevice::AttachTangoEventComponent(UTangoEventComponent* Component)
{
	TangoEventComponents.Emplace(Component);
}