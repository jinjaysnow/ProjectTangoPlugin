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

#include <UnrealTemplate.h>

#if PLATFORM_ANDROID
//#include "Private/Android/AndroidJNI.h"
#include "Android/AndroidJNI.h"
#include "AndroidApplication.h"
#endif

FString UTangoDevice::GetLoadedAreaDescriptionUUID()
{
	return CurrentConfig.AreaDescription.UUID;
}

TArray<FString> UTangoDevice::GetAllUUIDs()
{
	TArray<FString> ReturnValues;
#if PLATFORM_ANDROID
	char* Uuid;
	if (TangoService_getAreaDescriptionUUIDList(&Uuid) == TANGO_SUCCESS)
	{
		FString(Uuid).ParseIntoArray(ReturnValues, TEXT(","), true);
	}
#endif
	return ReturnValues;
}

TArray<FTangoAreaDescription> UTangoDevice::GetAreaDescriptions()
{
	TArray<FTangoAreaDescription> AreaDescriptions;
#if PLATFORM_ANDROID
	const char* Key = "name";

	TArray<FString> UUIDs = GetAllUUIDs();
	for (int i = 0; i < UUIDs.Num(); i++)
	{
		std::string t = TCHAR_TO_UTF8(*UUIDs[i]);
		const char* uuid = t.c_str();

		TangoAreaDescriptionMetadata Metadata;
		char* Name;
		size_t Size = 0;
		if (TangoService_getAreaDescriptionMetadata(uuid, &Metadata) == TANGO_SUCCESS
			&& TangoAreaDescriptionMetadata_get(Metadata, Key, &Size, &Name) == TANGO_SUCCESS)
		{
			AreaDescriptions.Add(FTangoAreaDescription(UUIDs[i], FString(Name)));
		}
	}
#endif
	return AreaDescriptions;
}

FTangoAreaDescriptionMetaData UTangoDevice::GetMetaData(FString UUID, bool& bIsSuccessful)
{
	//Set up return variables
	FTangoAreaDescriptionMetaData Result = FTangoAreaDescriptionMetaData();

#if PLATFORM_ANDROID

	//Set up metadata C style structures
	TangoAreaDescriptionMetadata Metadata;

	//Set up Unreal style structures

	//Prepare arguments
	std::string ConvertedUUIDKey = TCHAR_TO_UTF8(*UUID);
	const char* ConvertedUUIDKeyCString = ConvertedUUIDKey.c_str();

	std::string ConvertedNameKey = "name";
	const char* ConvertedNameKeyCString = ConvertedNameKey.c_str();
	size_t NameValueSize;
	char* NameValue;

	std::string ConvertedTransformationKey = "transformation";
	const char* ConvertedTransformationKeyCString = ConvertedTransformationKey.c_str();
	size_t TransformValueSize;
	double TransformValue[7] = { 0, 0, 0, 0, 0, 0, 1 };
	char* TransformValuePointer = reinterpret_cast<char*>(&TransformValue);

	std::string ConvertedDateMsSinceEpochKey = "date_ms_since_epoch";
	const char* ConvertedDateMsSinceEpochKeyCString = ConvertedDateMsSinceEpochKey.c_str();
	size_t DateMsSinceEpochValueSize;
	uint64 DateMsSinceEpochValue;

	char* DatePointer = reinterpret_cast<char*>(&DateMsSinceEpochValue);

	//Poll service & populate metadata pointer
	if (TangoService_getAreaDescriptionMetadata(ConvertedUUIDKeyCString, &Metadata) != TANGO_SUCCESS)
	{
		bIsSuccessful = false;
		UE_LOG(TangoPlugin, Error, TEXT("TangoDeviceAreaLearning::GetMetaData: Could not get Metadata object for %s"), *UUID);
		return FTangoAreaDescriptionMetaData();
	}

	//Poll service for metadata properties
	if (TangoAreaDescriptionMetadata_get(Metadata, ConvertedNameKeyCString, &NameValueSize, &NameValue) == TANGO_SUCCESS)
	{
		if (TangoAreaDescriptionMetadata_get(Metadata, ConvertedTransformationKeyCString, &TransformValueSize, &TransformValuePointer) == TANGO_SUCCESS)
		{
			if (TangoAreaDescriptionMetadata_get(Metadata, ConvertedDateMsSinceEpochKeyCString, &DateMsSinceEpochValueSize, &DatePointer) == TANGO_SUCCESS)
			{
				//We read everything OK, now read in the metadata
				Result.Filename = FString(NameValue);

				double* TVP = reinterpret_cast<double*>(TransformValuePointer);
				//Read in transformation
                int32 i;
                for (i = 0; i < 3; i++) {
                    Result.Position[i] = TVP[i];
                }
				UE_LOG(TangoPlugin, Log, TEXT("Area description ecef pos: %f, %f, %f, "), TVP[0], TVP[1], TVP[2]);
                for (int32 j = 0; j < 4; j++) {
                    Result.Orientation[j] = TVP[i+j];
                }
				UE_LOG(TangoPlugin, Log, TEXT("Area description ecef quat: %f, %f, %f, %f "), TVP[3], TVP[4], TVP[5], TVP[6]);
				//Read in milliseconds since epoch 
				uint64 NativeInteger = 0;
				NativeInteger = *(reinterpret_cast<uint64*>(DatePointer));
				Result.MillisecondsSinceUnixEpoch = NativeInteger;
				bIsSuccessful = true;
			}

			else
			{
				//Result was not successful!
				UE_LOG(TangoPlugin, Error, TEXT("TangoDeviceAreaLearning::GetMetaData: Could not get Date."));
				bIsSuccessful = false;
			}
		}
		else
		{
			//Result was not successful!
			UE_LOG(TangoPlugin, Error, TEXT("TangoDeviceAreaLearning::GetMetaData: Could not get Transformation."));
			bIsSuccessful = false;
		}
	}
	else
	{
		//Result was not successful!
		UE_LOG(TangoPlugin, Error, TEXT("TangoDeviceAreaLearning::GetMetaData: Could not get Name string."));

		bIsSuccessful = false;
	}

#endif
	return Result;
}


void UTangoDevice::SaveMetaData(FString UUID, FTangoAreaDescriptionMetaData NewMetadata, bool& bIsSuccessful)
{
    bIsSuccessful = false;
    
#if PLATFORM_ANDROID
    TangoAreaDescriptionMetadata Metadata;
    
    //Filename argument conversion
    const char* NameKey = "name"; //key for filename
    std::string ConvertedName = TCHAR_TO_UTF8(*(NewMetadata.Filename));
    const char* NameValue = ConvertedName.c_str();
    
    std::string ConvertedUUID = TCHAR_TO_UTF8(*UUID);
    const char* ConvertedUUIDValue = ConvertedUUID.c_str();
    
    const char* TransformationKey = "transformation";
    std::string ConvertedTransformationKey = TCHAR_TO_UTF8(TransformationKey);
    
    double TransformationElements[7] = { 0, 0, 0, 0, 0, 0, 1 };
    //@TODO: Replace these magic numbers (8: bytes per double value, 7: number of elements in array) with constant values
    uint TransformationArrayByteCount = (uint)(8 * 7);
    
    //Translation
    TransformationElements[0] = NewMetadata.Position[0];
    TransformationElements[1] = NewMetadata.Position[1];
    TransformationElements[2] = NewMetadata.Position[2];
    TransformationElements[3] = NewMetadata.Orientation[0];
    TransformationElements[4] = NewMetadata.Orientation[1];
    TransformationElements[5] = NewMetadata.Orientation[2];
    TransformationElements[6] = NewMetadata.Orientation[3];
    
    if (TangoService_getAreaDescriptionMetadata(ConvertedUUIDValue, &Metadata) != TANGO_SUCCESS)
    {
        UE_LOG(TangoPlugin, Error, TEXT("TangoDeviceAreaLearning::SaveMetaData: Call to get MetaData pointer from Tango was not successful, could not save metadata!"));
        bIsSuccessful = false;
        return;
    }
    
    if((TangoAreaDescriptionMetadata_set(Metadata, NameKey, (NewMetadata.Filename).Len(), NameValue) != TANGO_SUCCESS)
       || (TangoAreaDescriptionMetadata_set(Metadata, TransformationKey, TransformationArrayByteCount, (char*)(TransformationElements)) != TANGO_SUCCESS)
       || (TangoService_saveAreaDescriptionMetadata(ConvertedUUIDValue, Metadata) != TANGO_SUCCESS))
    {
        //Failed to save data!
        UE_LOG(TangoPlugin, Error, TEXT("TangoDeviceAreaLearning::SaveMetaData: Calls to Tango were not successful, could not save metadata!"));
        bIsSuccessful = false;
    }
    else
    {
        bIsSuccessful = true;
        //Saved data correctly.
        UE_LOG(TangoPlugin, Log, TEXT("TangoDeviceAreaLearning::SaveMetaData: successfully overwrote file metadata."));
    }
    TangoAreaDescriptionMetadata_free(Metadata);
#endif
}



void UTangoDevice::ImportCurrentArea(FString Filepath, bool &bIsSuccessful)
{
	UE_LOG(TangoPlugin, Log, TEXT("TangoDeviceAreaLearning::ImportCurrentArea: Starting import"));

#if PLATFORM_ANDROID
	jobject AppContext = NULL;

	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		//Prepare the arguments
		jstring TangoImportFilenameArg = Env->NewStringUTF(TCHAR_TO_UTF8(*Filepath));

		static jmethodID ADFImportIntentMethod = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_RequestImportPermission", "(Ljava/lang/String;)V", false);
		FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, ADFImportIntentMethod, TangoImportFilenameArg);
        bIsSuccessful = true;
	}
	else
	{
		UE_LOG(TangoPlugin, Error, TEXT("TangoDeviceAreaLearning::ImportCurrentArea: Could not get Java environment!"));
        bIsSuccessful = false;
	}

#endif
    UE_LOG(TangoPlugin, Log, TEXT("TangoDeviceAreaLearning::ExportCurrentArea: Finished import request call"));
}

void UTangoDevice::ExportCurrentArea(FString UUID, FString Filepath, bool& bIsSuccessful)
{
	UE_LOG(TangoPlugin, Log, TEXT("TangoDeviceAreaLearning::ExportCurrentArea: Starting export"));
	//Make the call to Java

#if PLATFORM_ANDROID
	jobject AppContext = NULL;

	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		//Prepare the arguments
		jstring TangoExportUUIDArg = Env->NewStringUTF(TCHAR_TO_UTF8(*UUID));
		jstring TangoExportFilenameArg = Env->NewStringUTF(TCHAR_TO_UTF8(*Filepath));

		static jmethodID ADFExportIntentMethod = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_RequestExportPermission", "(Ljava/lang/String;Ljava/lang/String;)V", false);

		FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, ADFExportIntentMethod, TangoExportUUIDArg, TangoExportFilenameArg);
        bIsSuccessful = true;
	}
	else
	{
		UE_LOG(TangoPlugin, Error, TEXT("TangoDeviceAreaLearning::ExportCurrentArea: Could not get Java environment!"));
        bIsSuccessful = false;
	}
#endif
	UE_LOG(TangoPlugin, Log, TEXT("TangoDeviceAreaLearning::ExportCurrentArea: Finished export request call"));
}