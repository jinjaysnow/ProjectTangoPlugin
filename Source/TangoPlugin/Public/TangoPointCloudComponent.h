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
#include "TangoDataTypes.h"
#include "TangoPointCloudComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTangoXYZijDataAvailable, float, TimeStamp);


UCLASS(ClassGroup = Tango, Blueprintable)
class TANGOPLUGIN_API UPointCloudContainer : public UObject
{
	GENERATED_BODY()
public:
	const TArray<FVector>& GetPointCloudArray(float& Timestamp);

private:
	//The Dummy Point cloud is returned in the event that the Point Cloud pointer is null.
	TArray<FVector> DummyPointCloud;
};

UENUM(BlueprintType)
namespace ETangoPointSpace
{
	enum Type
	{
		LOCAL UMETA(DisplayName = "Depth Space"),
		ADF_DEPTH UMETA(DisplayName = "ADF Space"),
		STARTOFSERVICE_DEPTH UMETA(DisplayName = "Start of Service Space")
	};
}

UCLASS(ClassGroup = Tango, Blueprintable, meta = (BlueprintSpawnableComponent))
class TANGOPLUGIN_API UTangoPointCloudComponent : public UActorComponent
{
	GENERATED_BODY()
	UTangoPointCloudComponent();

public:
	virtual void BeginPlay() override;

	/**
	* Returns the pose along the plane at the specified screen point
	*/
	UFUNCTION(Category = "Tango|Depth", meta = (ToolTip = "Returns the pose along the plane at the specified screen point", keyword = "fit, plane"), BlueprintPure)
		bool FitPlane(const FVector2D& ScreenPoint, FTransform& Pose);

	/*
	* Get the max number of points a single frame from the point cloud can contain.
	* @param Target The Unreal Engine / Tango Point Cloud interface object.
	* @return The maximum number of points the point cloud can contain.
	*/
	UFUNCTION(Category = "Tango|Depth", meta = (ToolTip = "Get max number of points the point cloud can contain.", keyword = "depth, point cloud, max, number, buffer"), BlueprintPure)
		int32 GetMaxPointCount();

	UFUNCTION(Category = "Tango|Depth", meta = (ToolTip = "Get current timestamp", keyword = "depth, point cloud, timestamp"), BlueprintPure)
		float GetPointCloudTimestamp();
};

