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

#if PLATFORM_ANDROID
#include "tango_client_api.h"
#endif

#include "TangoDeviceImage.generated.h"


#if PLATFORM_ANDROID
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTangoImageBufferAvailable, const TangoImageBuffer*);
#endif

UCLASS(NotBlueprintable, NotPlaceable, Transient)
class UTangoDeviceImage : public UObject
{
	GENERATED_BODY()
public:
	virtual void BeginDestroy() override;

	void Init(
#if PLATFORM_ANDROID
		TangoConfig Config_
#endif
		);

	bool CreateTexture(
#if PLATFORM_ANDROID
		TangoConfig Config_
#endif
	);
	void ConnectCallback();
	bool DisconnectCallback();

	void TickByDevice();

	void TickByCamera(TFunction<void(double)> OnCameraTimestamp);

	//Tango Image functions
	bool bIsImageBufferSet;
	float GetImageBufferTimestamp();

	bool setRuntimeConfig(FTangoRuntimeConfig& RuntimeConfig);

private:

	enum ConnectionState
	{
		DISCONNECTED,
		WANTTOCONNECT,
		CONNECTSHEDULED,
		CONNECTED
	};

	ConnectionState State;

	bool TexturesReady();
	void OnNewDataAvailable();
	void CheckConnectCallback();

	bool bNewDataAvailable;
public:
	
	UPROPERTY(Transient)
		UTexture* VideoTexture;
	double GetLastTimestamp()
	{
		return LastTimestamp;
	}
	uint32 RGBOpenGLPointer; 
#if PLATFORM_ANDROID
	FOnTangoImageBufferAvailable OnImageBufferAvailable;
#endif
private:
	
	bool IsNewDataAvail();
	void DataSet(double Stamp) { bNewDataAvailable = false;  GameThreadTimestamp = Stamp; LastTimestamp = Stamp; }
	bool bNeedsAllocation;	
	double LastTimestamp;
	double GameThreadTimestamp;
	
#if PLATFORM_ANDROID
	
	TangoImageBuffer TangoBuffer;
	void OnImageBuffer(const TangoImageBuffer* Buffer);
#endif
};
