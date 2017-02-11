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
#include "TangoFunctionLibrary.h"
#include "TangoDataTypes.h"
#include "TangoCoordinateConversions.h"

void UTangoFunctionLibrary::ConnectTangoService(FTangoConfig Configuration, FTangoRuntimeConfig RuntimeConfiguration)
{
	UTangoDevice::Get().StartTangoService(Configuration, RuntimeConfiguration);
}

void UTangoFunctionLibrary::DisconnectTangoService()
{
	UTangoDevice::Get().StopTangoService();
}

void UTangoFunctionLibrary::ReconnectTangoService(FTangoConfig Configuration, FTangoRuntimeConfig RuntimeConfiguration)
{
	UTangoDevice::Get().RestartService(Configuration, RuntimeConfiguration);
}

bool UTangoFunctionLibrary::IsTangoServiceRunning()
{
	return UTangoDevice::Get().IsTangoServiceRunning();
}

FTangoCameraIntrinsics UTangoFunctionLibrary::GetCameraIntrinsics(ETangoCameraType CameraID)
{
	return UTangoDevice::Get().GetCameraIntrinsics(CameraID);
}

TArray<FTangoAreaDescription> UTangoFunctionLibrary::GetAllAreaDescriptionData()
{
	return UTangoDevice::Get().GetAreaDescriptions();
}

FTangoAreaDescription UTangoFunctionLibrary::GetLoadedAreaDescription()
{
	FTangoAreaDescription CurrentAreaDescription;

	TArray<FTangoAreaDescription> AreaDescriptions = GetAllAreaDescriptionData();
	FString LoadedUUID = UTangoDevice::Get().GetLoadedAreaDescriptionUUID();
	bool Found = false;
	for (int i = 0; i < AreaDescriptions.Num(); i++)
	{
		if (AreaDescriptions[i].UUID == LoadedUUID)
		{
			CurrentAreaDescription = AreaDescriptions[i];
			Found = true;
			break;
		}
	}
	if (!Found)
	{
		UE_LOG(TangoPlugin, Error, TEXT("Loaded area description not found: %s"), *LoadedUUID);
	}
	return CurrentAreaDescription;
}

FTangoConfig UTangoFunctionLibrary::GetTangoConfig(FTangoRuntimeConfig& RuntimeConfig)
{
	RuntimeConfig = UTangoDevice::Get().GetCurrentRuntimeConfig();
	return UTangoDevice::Get().GetCurrentConfig();
}

bool UTangoFunctionLibrary::SetTangoRuntimeConfig(FTangoRuntimeConfig Configuration)
{
	return UTangoDevice::Get().SetTangoRuntimeConfig(Configuration);
}

void UTangoFunctionLibrary::GetBinaryAreaDescription(const FTangoAreaDescription& AreaDescription, TArray<uint8>& Result, bool& bSuccess)
{
	bSuccess = FFileHelper::LoadFileToArray(Result, *AreaDescription.Filename);
}

void UTangoFunctionLibrary::ConvertTransformFromTango(const FTransform& Transform, ETangoCoordinateFrameType BaseFrame, FTransform& Result)
{
  TangoSpaceConversions::TangoSpaceConversionPair Converter;
  FTangoCoordinateFramePair RefPair;
  RefPair.BaseFrame = BaseFrame;
  RefPair.TargetFrame = BaseFrame;
  TangoSpaceConversions::GetSpaceConversionPair(Converter, RefPair);
  FMatrix Target = Transform.ToMatrixWithScale();
  Result.SetFromMatrix(Converter.TargetFrameToUE * Target);
  Result.SetTranslation(Result.GetTranslation() * 100);
}

void UTangoFunctionLibrary::ConvertTransformToTango(const FTransform& Transform, ETangoCoordinateFrameType TargetFrame, FTransform& Result)
{
  TangoSpaceConversions::TangoSpaceConversionPair Converter;
  FTangoCoordinateFramePair RefPair;
  RefPair.BaseFrame = TargetFrame;
  RefPair.TargetFrame = TargetFrame;
  TangoSpaceConversions::GetSpaceConversionPair(Converter, RefPair);
  FTransform Scaled = Transform;
  Scaled.SetTranslation(Transform.GetTranslation() * .01f);
  FMatrix Source = Scaled.ToMatrixWithScale();
  Result.SetFromMatrix(Converter.UEtoBaseFrame * Source);
}


