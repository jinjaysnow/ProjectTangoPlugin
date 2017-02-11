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

#include "Components/ActorComponent.h"
#include "TangoImageComponent.generated.h"


UCLASS(ClassGroup = Tango, Blueprintable, meta = (BlueprintSpawnableComponent))
class TANGOPLUGIN_API UTangoImageComponent : public UActorComponent
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTangoImageAvailable, float, Timestamp);
public:
	UTangoImageComponent();

	UPROPERTY(BlueprintAssignable)
		FOnTangoImageAvailable OnTangoImageAvailable;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	
	/*
	* Get RGB camera texture. Also returns a timestamp of when the image was captured and whether the result image is valid.
	* @param Target The Unreal Engine / Tango Image interface object.
	* @param Timestamp The seconds since tango service was started, when this texture was generated.
	* @param IsValid Returns true if the resulting image is valid.
	* @return Returns a texture reference of the current RGB output from the camera.
	*/
	UFUNCTION(Category = "Tango|Camera", BlueprintPure, meta = (ToolTip = "Get camera texture view. Also returns a Timestamp of when the image was captured and whether the result image is valid.", keyword = "image, camera, view, texture"))
		UTexture* GetCameraTexture(float& Timestamp, bool& bIsValid);

	/*
	*	Returns a float which represents the time (in seconds since the Tango service was started) when the latest Image pose was retrieved.
	* @param The Unreal Engine / Tango Image interface object.
	* @return The time (in seconds since the Tango service was started) when the latest Image pose was retrieved.
	*/
	UFUNCTION(Category = "Tango|Camera", BluePrintPure, meta = (ToolTip = "Get the latest cameraimage timestamp.", keyword = "image, timestamp, time, seconds, camera"))
		float  GetLatestImageTimeStamp();
private:
	float LastBroadCastedTimestamp = 0;

};