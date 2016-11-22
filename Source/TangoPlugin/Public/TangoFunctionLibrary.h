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

#include "Kismet/BlueprintFunctionLibrary.h"
#include "TangoDataTypes.h"
#include "TangoAreaLearningComponent.h"
#include "TangoFunctionLibrary.generated.h"

/**
 *
 */
UCLASS()
class TANGOPLUGIN_API UTangoFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/*
	*	Returns structures representing the current Tango Config and Tango Runtime Config.
	* @param RuntimeConfig A struct containing the current non-runtime-settable Configuration options.
	* @return A struct containing the current runtime-settable Configuration options.
	*/
	UFUNCTION(Category = "Tango|Core", BlueprintPure, meta = (ToolTip = "Gets the current Configuration of the Tango device", Keywords = "tango, Configuration, Config, device"))
		static FTangoConfig GetTangoConfig(FTangoRuntimeConfig& RuntimeConfig);

	/*
	*	Sets the runtime-settable parameters of the Tango to the values of the FTangoRuntimeConfig struct passed into the Configuration argument.
	* @param RuntimeConfig The runtime-settable Configuration parameters to pass the Tango service.
	* @return returns true if the runtime Configuration values were successfully set.
	*/
	UFUNCTION(Category = "Tango|Core", BlueprintCallable, meta = (ToolTip = "Sets the current runtime Configuration of the Tango device", Keywords = "tango, Configuration, Config, device"))
		static bool SetTangoRuntimeConfig(FTangoRuntimeConfig Configuration);

	/*
	*	Sets the runtime-settable parameters of the Tango to the values of the FTangoRuntimeConfig struct passed into the Configuration argument.
	* @param Configuration The runtime-settable Configuration parameters to pass the Tango service.
	* @return returns true if the runtime Configuration values were successfully set.
	*/
	UFUNCTION(Category = "Tango|Core", BlueprintCallable, meta = (ToolTip = "Starts the Tango service, using the Config that is passed through", Keywords = "tango, connect, run, Config, sevice, device"))
		static void ConnectTangoService(FTangoConfig Configuration, FTangoRuntimeConfig RuntimeConfiguration);

	/*
	* Disconnects the Tango service.
	* Note that it's not required to call this function on EndPlay- the Tango plugin will automatically perform a disconnection when the app is paused or closed, and will automatically resume if the app is brought back into the foreground.
	*/
	UFUNCTION(Category = "Tango|Core", BlueprintCallable, meta = (ToolTip = "Stop the Tango service", Keywords = "tango, disconnect, sevice, device"))
		static void DisconnectTangoService();

	/*
	*	Disconnects the currently connected Tango service and reconnects the Tango service with the given Configuration parameters.
	* @param Configuration The Configuration parameters to pass the Tango service.
	*/
	UFUNCTION(Category = "Tango|Core", BlueprintCallable, meta = (ToolTip = "Restarts the Tango service, using the Config that is passed through", Keywords = "tango, disconnect, reconnect, connect, refresh, sevice, device"))
		static void ReconnectTangoService(FTangoConfig Configuration, FTangoRuntimeConfig RuntimeConfiguration);

	/*
	*	Returns true if the Tango Service is currently connected and running, otherwise returns false.
	* @return True if the Tango Service is currently connected and running, otherwise false.
	*/
	UFUNCTION(Category = "Tango|Core", BlueprintPure, meta = (ToolTip = "Inidicates if the Tango service is currently running", Keywords = "tango, connect, sevice, device"))
		static bool IsTangoServiceRunning();

	/*
	* Returns an array containing all of the Area Description files which are stored within this devices's Tango Core Repository. Please note that this does not include ADFs stored in other areas on the file system of the device.
	*	@return An array of all the UUID/Filename pairs stored within this device's Tango Core Repository.
	*
	*/
	UFUNCTION(Category = "Tango|Core", BlueprintPure, meta = (ToolTip = "Get current ADF UUID list", Keywords = "tango, adf, area description, get, data, uuid, list"))
		static TArray<FTangoAreaDescription> GetAllAreaDescriptionData();

	/*
	*	Returns the currently loaded Area Definition file.
	* @return the ADF that is currently loaded. If no ADF is currently loaded, this will return a struct with blank values for UUID and Filename.
	*/
	UFUNCTION(Category = "Tango|Core", BlueprintPure, meta = (ToolTip = "Get the name of the currently loaded area description", Keywords = "tango, adf, area description, get, device"))
		static FTangoAreaDescription GetLoadedAreaDescription();

	/*
	*	Returns a structure containing information about the nature of the selected camera. This is useful for many Augmented Reality and camera alignment calculations.
	* @param CameraID an enumeration which denotes which camera's intrinsics information should be returned in the output FTangoCameraIntrinsics struct.
	* @return An array of all the UUID/Filename pairs stored within this device's Tango Core repository.
	*/
	UFUNCTION(BlueprintCallable, Category = "Tango|Core", meta = (Keywords = "tango, camera, camera type, device"))
		static FTangoCameraIntrinsics GetCameraIntrinsics(ETangoCameraType CameraID);

	/*
	* Utility to get a rotation as a quaternion
	*/
	UFUNCTION(BlueprintCallable, Category = "Tango|Util", BlueprintPure)
		static FQuat GetRotationAsQuaternion(const FRotator& Rotator)
	{
		return Rotator.Quaternion();
	}

	/*
	* Utility to get a rotation as a Rotator
	*/
	UFUNCTION(BlueprintCallable, Category = "Tango|Util", BlueprintPure)
		static FRotator GetRotationAsRotator(const FQuat& Quat)
	{
		return FRotator(Quat);
	}

	/*
	* Utility to convert from UE Coordinates to a Tango Coordinate frame
	*/
	UFUNCTION(BlueprintCallable, Category = "Tango|Util", BlueprintPure)
		static void ConvertTransformToTango(const FTransform& Transform, ETangoCoordinateFrameType TargetFrame, FTransform& Result);

	/*
	* Utility to convert to UE Coordinates from a Tango Coordinate frame
	*/
	UFUNCTION(BlueprintCallable, Category = "Tango|Util", BlueprintPure)
		static void ConvertTransformFromTango(const FTransform& Transform, ETangoCoordinateFrameType BaseFrame, FTransform& Result);

};
