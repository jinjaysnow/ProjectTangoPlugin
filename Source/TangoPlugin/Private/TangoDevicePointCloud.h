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

#include "Object.h"
#if PLATFORM_ANDROID
#include "tango_client_api.h"
#include "tango_support_api.h"
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPointCloudAvailable, const TangoPointCloud*);
#endif

class TangoDevicePointCloud
{
	
public:
	int32 GetMaxVertexCapacity();
	void TickByDevice();
#if PLATFORM_ANDROID
	FOnPointCloudAvailable OnPointCloudAvailable;
#endif
	TangoDevicePointCloud(
#if PLATFORM_ANDROID
		TangoConfig Config_
#endif
		);
	void ConnectCallback();
	~TangoDevicePointCloud();
	

#if PLATFORM_ANDROID	
	FCriticalSection* GetPointCloudLock()
	{
		return &PointCloudMutex;
	}
	const TangoPointCloud* GetLatestPointCloud();
	bool FitPlane(float X, float Y, FTransform& Result);
	
#endif
	
private:

#if PLATFORM_ANDROID
	FCriticalSection PointCloudMutex;
	TangoSupportPointCloudManager* point_cloud_manager_;
	void HandleOnPointCloudAvailable(const TangoPointCloud* PointCloud);
#endif
	uint32_t VertCapacity;
};
