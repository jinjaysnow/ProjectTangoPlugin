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
#include "TangoEventComponent.h"
#include "TangoDevice.h"

UTangoEventComponent::UTangoEventComponent() : Super()
{
}

void UTangoEventComponent::BeginPlay()
{
	UTangoDevice::Get().AttachTangoEventComponent(this);
	Super::BeginPlay();
}

/*
 *  ADF Import/Export event functions;
 *  These are exposed to the Java layer and are used to trigger events which inform developers
 *  of the selections users have made from the ADF Import or Export UIs.
 */
#if PLATFORM_ANDROID
extern "C"
{
    //JNI Export function: Called upon ADF Export requests, returns the request result
    JNIEXPORT void JNICALL
    Java_com_projecttango_plugin_TangoInterface_OnExportRequest(JNIEnv* Env, jint, jint Result)
    {
        UE_LOG(LogTemp, Warning, TEXT("UTangoEventComponent::OnExportRequest; Result of export is %d"), Result);

        //Push the event into the events buffer
        FTangoEvent UEvent;

        UEvent.Type = static_cast<ETangoEventType>(TANGO_EVENT_GENERAL);
        UEvent.Key = ETangoEventKeyType::EXPORT_RESULT;
        
        //Convert from Android return convention (-1, 0, 1) to Unreal enum convention (0, 1, 2)
        UEvent.Message = FString::FromInt((int) Result + 1);
        
        UTangoDevice::Get().PushTangoEvent(UEvent);
		TangoDeviceAreaLearning* AreaLearning = UTangoDevice::Get().GetTangoDeviceAreaLearningPointer();
		if (AreaLearning != nullptr)
		{
			AreaLearning->OnExportResult(UEvent);
		}
    }
    
    //JNI Import function: Called upon ADF Import requests, returns the request result
    JNIEXPORT void JNICALL
    Java_com_projecttango_plugin_TangoInterface_OnImportRequest(JNIEnv* Env, jint, jint Result)
    {
        UE_LOG(LogTemp, Warning, TEXT("UTangoEventComponent::OnImportRequest; Result of import is %d"), Result);
        
        //Push the event into the events buffer
        FTangoEvent UEvent;
        UEvent.Type = static_cast<ETangoEventType>(TANGO_EVENT_GENERAL);
        UEvent.Key = ETangoEventKeyType::IMPORT_RESULT;
        
        //Convert from Android return convention (-1, 0, 1) to Unreal enum convention (0, 1, 2)
        UEvent.Message = FString::FromInt((int) Result + 1);
        
        UTangoDevice::Get().PushTangoEvent(UEvent);
		TangoDeviceAreaLearning* AreaLearning = UTangoDevice::Get().GetTangoDeviceAreaLearningPointer();
		if (AreaLearning != nullptr)
		{
			AreaLearning->OnImportResult(UEvent);
		}
    }
}
#endif