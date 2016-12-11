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


static bool bTexturesHaveDataInThem;
static double ImageBufferTimestamp;

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

	bool CreateYUVTextures(
#if PLATFORM_ANDROID
		TangoConfig Config_
#endif
	);
	void ConnectCallback();
	bool DisconnectCallback();

	void TickByDevice();

	UTexture* GetYTexture();

	UTexture * GetCrTexture();

	UTexture * GetCbTexture();

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
	
	bool IsNewDataAvail();
	void DataSet(double Stamp) { bNewDataAvailable = false; bTexturesHaveDataInThem = true; ImageBufferTimestamp = Stamp; }
	// hack: workarounds for bugs in TangoService_Experimental_connectTextureIdUnity
	bool bNeedsAllocation;
	uint32 YOpenGLPointer;
	uint32 CrOpenGLPointer;
	uint32 CbOpenGLPointer;
	TArray<uint8> Buffer;
	FCriticalSection BufferLock;
#if PLATFORM_ANDROID
	TangoImageBuffer TangoBuffer;
	void RenderImageBuffer();
	void CopyImageBuffer(const TangoImageBuffer* Buffer);
#endif
};
