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
#include "TangoDevicePointCloud.h"
#include "TangoPointCloudComponent.h"

#include "TangoDevice.h"

#include <UnrealTemplate.h>

#if PLATFORM_ANDROID
#include "Android/AndroidJNI.h"
#include "AndroidApplication.h"
#endif

int32 TangoDevicePointCloud::GetMaxVertexCapacity()
{
	return VertCapacity;
}


void TangoDevicePointCloud::TickByDevice()
{
#if PLATFORM_ANDROID

	for (int i = 0; i < UTangoDevice::Get().PointCloudComponents.Num(); ++i)
	{
		if (UTangoDevice::Get().PointCloudComponents[i] != nullptr)
		{

		}
		else
		{
			UTangoDevice::Get().PointCloudComponents.RemoveAt(i);
			i--;
		}
	}

#endif
}

TangoDevicePointCloud::TangoDevicePointCloud(
#if PLATFORM_ANDROID
	TangoConfig Config_
#endif
	)
{
	UE_LOG(TangoPlugin, Log, TEXT("TangoDevicePointCloud::TangoDevicePointCloud: Creating TangoDevicePointCloud!"));
	//Setting up Point Cloud Buffers
	
#if PLATFORM_ANDROID

	
	int MaxPointCloudElements = 0;
	bool bSuccess = TangoConfig_getInt32(Config_, "max_point_cloud_elements", &MaxPointCloudElements) == TANGO_SUCCESS;
	uint32_t MaxPointCloudVertexCount = static_cast<uint32_t>(MaxPointCloudElements);
	point_cloud_manager_ = nullptr;
	if (bSuccess)
	{
		// Initialize TangoSupport context.
		static bool isTangoSupportInit = false;
		if (!isTangoSupportInit)
		{
			isTangoSupportInit = true;
			TangoSupport_initializeLibrary();
		}
		{
			int32 ret = TangoSupport_createPointCloudManager(MaxPointCloudElements,
				&point_cloud_manager_);
			if (ret != TANGO_SUCCESS)
			{
				UE_LOG(TangoPlugin, Error, TEXT("createPointCloudManager failed with error code: %d"), ret);
			}
			else
			{
				UE_LOG(TangoPlugin, Log, TEXT("createPointCloudManager %p"), point_cloud_manager_);
			}
		}
	}
	else
	{
		UE_LOG(TangoPlugin, Warning, TEXT("TangoDevicePointCloud::TangoDevicePointCloud: construction failed because read of max_point_cloud_elements was not successful."));
	}

#endif
	UE_LOG(TangoPlugin, Log, TEXT("TangoDevicePointCloud::TangoDevicePointCloud: Creating TangoDevicePointCloud FINISHED"));
}

void TangoDevicePointCloud::ConnectCallback()
{
	UE_LOG(TangoPlugin, Log, TEXT("TangoDevicePointCloud::ConnectCallback: RegisterCallBack!"));
#if PLATFORM_ANDROID
	//TangoService_connectOnXYZijAvailable([](void*, const TangoXYZij* XYZ_ij) {UTangoDevice::Get().GetTangoDevicePointCloudPointer()->OnXYZijAvailable(XYZ_ij); });
	TangoErrorType ret =
		TangoService_connectOnPointCloudAvailable([](void*, const TangoPointCloud* PointCloud) -> void
	{
		TangoDevicePointCloud* DevicePointCloud = UTangoDevice::Get().GetTangoDevicePointCloudPointer();
		if (DevicePointCloud != nullptr) DevicePointCloud->HandleOnPointCloudAvailable(PointCloud);
	});
#endif
}

TangoDevicePointCloud::~TangoDevicePointCloud()
{
#if PLATFORM_ANDROID
	FScopeLock ScopeLock(GetPointCloudLock());
	if (point_cloud_manager_ != nullptr)
	{
		TangoSupport_freePointCloudManager(point_cloud_manager_);
	}
#endif
}


#if PLATFORM_ANDROID

const TangoPointCloud* TangoDevicePointCloud::GetLatestPointCloud()
{
	TangoPointCloud* point_cloud = nullptr;
	if (point_cloud_manager_ != nullptr)
	{
		if (TangoSupport_getLatestPointCloud(point_cloud_manager_, &point_cloud) != TANGO_SUCCESS)
		{
			return nullptr;
		}
		UE_LOG(TangoPlugin, Log, TEXT("Got Latest Point Cloud %p: %d"), point_cloud, (point_cloud ? point_cloud->num_points : 0));
	}
	return point_cloud;
}

void TangoDevicePointCloud::HandleOnPointCloudAvailable(const TangoPointCloud* PointCloud)
{
	FScopeLock ScopeLock(GetPointCloudLock());
	if (point_cloud_manager_ != nullptr)
	{
		//UE_LOG(TangoPlugin, Log, TEXT("Updating Point Cloud %p"), PointCloud);
		TangoSupport_updatePointCloud(point_cloud_manager_, PointCloud);
		//UE_LOG(TangoPlugin, Log, TEXT("Updating Point Cloud latest => %p"), GetLatestPointCloud());
	}
	OnPointCloudAvailable.Broadcast(PointCloud);
}


bool TangoDevicePointCloud::FitPlane(float X, float Y, FTransform& Result)
{
	// Get the latest point cloud
	FScopeLock ScopeLock(GetPointCloudLock());
	const TangoPointCloud* point_cloud = GetLatestPointCloud();
	if (point_cloud == nullptr)
	{
		UE_LOG(TangoPlugin, Log, TEXT("%s: No point cloud available yet"));
		return false;
	}
	/// Calculate the conversion from the latest depth camera position to the
	/// position of the most recent color camera image. This corrects for screen
	/// lag between the two systems.
	TangoPoseData pose_color_camera_t0_T_depth_camera_t1;
	double camera_time_stamp = UTangoDevice::Get().GetTangoDeviceImagePointer()->GetLastTimestamp();
	int ret = TangoSupport_calculateRelativePose(
		camera_time_stamp, TANGO_COORDINATE_FRAME_CAMERA_COLOR,
		point_cloud->timestamp, TANGO_COORDINATE_FRAME_CAMERA_DEPTH,
		&pose_color_camera_t0_T_depth_camera_t1);
	if (ret != TANGO_SUCCESS)
	{
		UE_LOG(TangoPlugin, Error, TEXT("could not calculate relative pose"));
		return false;
	}
	UE_LOG(TangoPlugin, Log, TEXT("timestamps: camera: %f, point cloud: %f"), camera_time_stamp, point_cloud->timestamp);
	float uv[2] = { X, Y };
	double double_depth_position[3];
	double double_depth_plane_equation[4];
	if (TangoSupport_fitPlaneModelNearPoint(
		point_cloud, &pose_color_camera_t0_T_depth_camera_t1,
		uv, double_depth_position,
		double_depth_plane_equation) != TANGO_SUCCESS)
	{
		return false;
	}

	// Convert to UE conventions
	float Forward = (float)double_depth_position[2];
	float Right = (float)double_depth_position[0];
	float Up = -(float)double_depth_position[1];

	FVector DepthPosition(Forward, Right, Up);
	DepthPosition *= 100; // m to cm

	Forward = (float)double_depth_plane_equation[2];
	Right = (float)double_depth_plane_equation[0];
	Up = -(float)double_depth_plane_equation[1];

	FPlane DepthPlane(
		Forward, Right, Up,
		-(float)double_depth_plane_equation[3]
	);

	FTangoPoseData Data = UTangoDevice::Get().GetTangoDeviceMotionPointer()->
		GetPoseAtTime(FTangoCoordinateFramePair(ETangoCoordinateFrameType::AREA_DESCRIPTION,
			ETangoCoordinateFrameType::CAMERA_DEPTH), point_cloud->timestamp);

	const FTransform Transform(Data.Rotation, Data.Position);
	const FMatrix Matrix = Transform.ToMatrixNoScale();
	FVector WorldForward = DepthPlane.TransformBy(Matrix);
	FVector WorldPoint = Matrix.TransformPosition(DepthPosition);
	//UE_LOG(TangoPlugin, Log, TEXT("Fit plane World Point: %s"), *WorldPoint.ToString());
	FVector WorldUp(0, 0, 1);
	if ((WorldForward|WorldUp) > 0.5f)
	{
		WorldUp = FVector(1, 0, 0);
	}
	FVector WorldRight = WorldForward^WorldUp;
	WorldRight.Normalize();
	WorldUp = WorldForward^WorldRight;
	WorldUp.Normalize();
	Result = FTransform(WorldForward, WorldRight, WorldUp, WorldPoint);
	return true;
}

#endif