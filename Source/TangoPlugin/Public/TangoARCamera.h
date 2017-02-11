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

#include "Camera/CameraComponent.h"
#include "TangoARScreenComponent.h"
#include "TangoDataTypes.h"
#include "ITangoAR.h"
#include "TangoARCamera.generated.h"
class FTangoViewExtension;

UCLASS(ClassGroup = Tango, meta = (BlueprintSpawnableComponent))
class TANGOPLUGIN_API UTangoARCamera : public UCameraComponent, public ITangoARInterface
{
	GENERATED_UCLASS_BODY()
public:
	//UTangoARCamera();
	~UTangoARCamera();//In TangoViewExtension.cpp!
	//ITangoARInterface
public:
	virtual AActor* GetActor() override;
	virtual USceneComponent* AsSceneComponent() override;
	/** Whether the camera preview is shown */
	UPROPERTY(BlueprintReadWrite, Category = "Tango|Camera")
		bool bScreenIsVisible;
protected:
	virtual void BeginPlay() override;
	virtual void InitializeComponent() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	virtual bool WantToDoAR() override { return true; }
private:
	FTangoCoordinateFramePair FrameOfReference;
	UPROPERTY()
		UTangoARScreenComponent* ARScreen;
};
