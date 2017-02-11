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

#include "TangoDataTypes.generated.h"


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//TANGO PLUGIN BLUEPRINT-FRIENDLY ENUMERATIONS BEGIN HERE
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UENUM(BlueprintType)
enum class ETangoPoseStatus : uint8
{
	INITIALIZING	UMETA(DisplayName = "Tango pose initializing"),
	VALID			UMETA(DisplayName = "Tango pose valid"),
	INVALID			UMETA(DisplayName = "Tango pose invalid"),
	UNKNOWN			UMETA(DisplayName = "Tango pose unknown")
};

UENUM(BlueprintType)
enum class ETangoPositionContext : uint8
{
	START_TO_DEVICE UMETA(DisplayName = "Device from Service Start"),
	ADF_TO_DEVICE	UMETA(DisplayName = "Device from Area Description"),
	ADF_TO_START	UMETA(DisplayName = "Service Start from Area Description"),
	PREV_TO_DEVICE	UMETA(DisplayName = "Device from Previous Frame")
};

/*
	ETangoCoordinateFrameType
	Allows users to specify from the available coordinate systems the Tango service can use.
	Can be grouped into pairs using the FTangoCoordinateFramePair structure;
*/
UENUM(BlueprintType)
enum class ETangoCoordinateFrameType : uint8
{
	GLOBAL_WGS84			UMETA(DisplayName = "Global: WGS84"),
	AREA_DESCRIPTION		UMETA(DisplayName = "Area Description"),
	START_OF_SERVICE		UMETA(DisplayName = "Start of Service"),
	PREVIOUS_DEVICE_POSE	UMETA(DisplayName = "Previous Device Pose"),
	DEVICE					UMETA(DisplayName = "Device"),
	IMU						UMETA(DisplayName = "IMU"),
	DISPLAY					UMETA(DisplayName = "Display"),
	CAMERA_COLOR			UMETA(DisplayName = "Camera: Colour"),
	CAMERA_DEPTH			UMETA(DisplayName = "Camera: Depth"),
	CAMERA_FISHEYE			UMETA(DisplayName = "Camera: Fisheye")
};

/*
	ETangoPermissionType
*/
UENUM(BlueprintType)
enum class ETangoPermissionType : uint8
{
	AREA_LEARNING	UMETA(DisplayName = "Area Learning"),
	ADF_LOAD_SAVE	UMETA(DisplayName = "Area Description File Load/Save"),
	MOTION_TRACKING	UMETA(DisplayName = "IMU")
};

/*
	ETangoCalibrationType
*/
UENUM(BlueprintType)
enum class ETangoCalibrationType : uint8
{
	EQUIDISTANT				UMETA(DisplayName = "Equidistant"),
	POLYNOMIAL_2_PARAMETERS	UMETA(DisplayName = "Polynomial; 2 parameters"),
	POLYNOMIAL_3_PARAMETERS	UMETA(DisplayName = "Polynomial; 3 parameters"),
	POLYNOMIAL_5_PARAMETERS	UMETA(DisplayName = "Polynomial; 5 parameters"),
	UNKNOWN					UMETA(DisplayName = "Unknown")
};

/*
	ETangoDatasetRecordingMode
*/
UENUM(BlueprintType)
enum class ETangoDatasetRecordingMode : uint8
{
	MOTION_TRACKING			UMETA(DisplayName = "Motion Tracking"),
	SCENE_RECONSTRUCTION	UMETA(DisplayName = "Scene Reconstruction")
};

/*
	ETangoCameraType
*/
UENUM(BlueprintType)
enum class ETangoCameraType : uint8
{
	COLOR	UMETA(DisplayName = "Color Camera"),
	DEPTH	UMETA(DisplayName = "Depth Camera"),
	FISHEYE	UMETA(DisplayName = "Fisheye Camera"),
	RGBIR	UMETA(DisplayName = "RGBIR Camera"),
};

/*
	ETangoConfigType
*/
UENUM(BlueprintType)
enum class ETangoConfigType : uint8
{
	AREA_DESCRIPTION	UMETA(DisplayName = "Area Description"),
	CURRENT				UMETA(DisplayName = "Current"),
	DEFAULT				UMETA(DisplayName = "Default"),
	MOTION_TRACKING		UMETA(DisplayName = "Motion Tracking"),
	RUNTIME				UMETA(DisplayName = "Runtime")
};

/*
	ETangoEventType
*/
UENUM(BlueprintType)
enum class ETangoEventType : uint8
{
	AREA_LEARNING		UMETA(DisplayName = "Area Learning Event"),
	COLOR_CAMERA		UMETA(DisplayName = "Color Camera Event"),
	FEATURE_TRACKING	UMETA(DisplayName = "Feature Tracking Event"),
	FISHEYE_CAMERA		UMETA(DisplayName = "Fisheye Camera Event"),
	GENERAL				UMETA(DisplayName = "General Event"),
	IMU					UMETA(DisplayName = "IMU Event"),
	UNKNOWN				UMETA(DisplayName = "Unknown Event")
};

/*
	ETangoEventValue
*/
UENUM(BlueprintType)
enum class ETangoEventValue : uint8
{
	SERVICE_FAULT		UMETA(DisplayName = "Tango Service Fault")
};

/*
	ETangoEventKeyType
*/
UENUM(BlueprintType)
enum class ETangoEventKeyType : uint8
{
	KEY_AREA_DESCRIPTION_SAVE_PROGRESS		UMETA(DisplayName = "Area Description Save Progress"),
	KEY_SERVICE_EXCEPTION					UMETA(DisplayName = "Service Exception"),
	DESCRIPTION_COLOR_OVER_EXPOSED			UMETA(DisplayName = "Colour Over Exposed"),
	DESCRIPTION_COLOR_UNDER_EXPOSED			UMETA(DisplayName = "Colour Under Exposed"),
	DESCRIPTION_FISHEYE_OVER_EXPOSED		UMETA(DisplayName = "Fisheye Over Exposed"),
	DESCRIPTION_FISHEYE_UNDER_EXPOSED		UMETA(DisplayName = "Fisheye Under Exposed"),
	DESCRIPTION_TOO_FEW_FEATURES			UMETA(DisplayName = "Too Few Features Tracked"),
	UNKOWN									UMETA(DisplayName = "Unkown Event"),
	IMPORT_RESULT                           UMETA(DisplayName = "Import Completed"),
	EXPORT_RESULT                           UMETA(DisplayName = "Import Completed")
};


/*
	ETangoRequestResult
 */
UENUM(BlueprintType)
enum class ETangoRequestResult : uint8
{
	Success		UMETA(DisplayName = "Success- user agrees."),
	Cancelled	UMETA(DisplayName = "Cancelled- user has cancelled the request."),
	Denied		UMETA(DisplayName = "Denied- user does not agree.")
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//TANGO PLUGIN DATA STRUCTURES BEGIN HERE
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
	FTangoEvent
	Unreal wrapper for the C TangoEvent object.
*/
USTRUCT(BlueprintType)
struct TANGOPLUGIN_API FTangoEvent
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Tango event type"))
		ETangoEventKeyType Key;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Information on which Tango subsystem fired the event"))
		ETangoEventType Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Message from the Tango device"))
		FString Message;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Event time in seconds since the Tango device started"))
		float TimeStamp;
};

/*
	FTangoCoordinateFramePair
	Unreal wrapper for the C TangoCoordinateFramePair object.
*/

USTRUCT(BlueprintType)
struct TANGOPLUGIN_API FTangoCoordinateFramePair
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Source frame of reference"))
		ETangoCoordinateFrameType BaseFrame;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Target frame of reference"))
		ETangoCoordinateFrameType TargetFrame;

	//Assume a valid pairing- the frame pair should not default to the first elements of the enumeration
	FTangoCoordinateFramePair(const ETangoCoordinateFrameType NewBaseFrame = (ETangoCoordinateFrameType::START_OF_SERVICE),
		const ETangoCoordinateFrameType NewTargetFrame = (ETangoCoordinateFrameType::DEVICE));

	friend bool operator== (const FTangoCoordinateFramePair& A, const FTangoCoordinateFramePair& B)
	{
		return A.BaseFrame == B.BaseFrame && A.TargetFrame == B.TargetFrame;
	}

	friend uint32 GetTypeHash(const FTangoCoordinateFramePair& Other)
	{
		return (static_cast<uint32>(Other.BaseFrame) << 16) + static_cast<uint32>(Other.TargetFrame);
	}
};


/*
	FTangoAreaDescription
	Data structure which holds information about Tango Area Description Files.
	Note that it does not contain an ADF, and can not be converted to an ADF-
	it simply holds the information needed to manipulate the ADFs in a convenient
	manner.
*/
USTRUCT(BlueprintType)
struct TANGOPLUGIN_API FTangoAreaDescription
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Unique identifier of the ADF file"))
		FString UUID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Filename of the area description file"))
		FString Filename;

	FTangoAreaDescription(const FString InUUID, const FString InFileName);
	FTangoAreaDescription() {};

	void SetUUID(const FString NewValue);

	void SetFilename(const FString NewValue);
};

/*
FTangoRuntimeConfig
Structure for the Tango Runtime settings
Used by developers to set their desired Configuration for the Tango service at runtime.
*/
USTRUCT(BlueprintType)
struct TANGOPLUGIN_API FTangoRuntimeConfig
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Should the devices colour camera be enabled"))
		bool bEnableColorCamera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Should the devices depth sensor be enabled"))
		bool bEnableDepth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "The frame rate of the depth sensor"))
		int32 RuntimeDepthFramerate;
};

/*
	FTangoConfig
	Wrapper structure for the TangoConfig object
	Used by developers to set their desired Configuration for the Tango service.
*/
USTRUCT(BlueprintType)
struct TANGOPLUGIN_API FTangoConfig
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango")
		bool bEnableAutoRecovery;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Whether to use the cloud ADF service"))
		bool bUseCloudAdf;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Allow for activation of color camera"))
		bool bEnableColorCameraCapabilities;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Automatically adjust ISO and Exposure"))
		bool bColorModeAuto;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Allow for activation of depth camera"))
		bool bEnableDepthCapabilities;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Whether the pose should be tracked in high frequency mode"))
		bool bHighRatePose;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Whether ADF learning capabilities should be enabled"))
		bool bEnableLearningMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Whether COM/ADF learning capabilities should be enabled"))
		bool bEnableDriftCorrection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Should low latency IMU be activated"))
		bool bLowLatencyIMUIntegration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Whether motion tracking should be activated"))
		bool bEnableMotionTracking;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Whether the pose should be smoothed. Can result in a little bit of lag and inaccuracies"))
		bool bSmoothPose;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Sets exposure of the color camera"))
		int32 ColorExposure;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Sets ISO value of color camera"))
		int32 ColorISO;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Determines which area description file should be loaded"))
		FTangoAreaDescription AreaDescription;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "The scaling factor from Tango coordinates (meters) to Unreal coordiantes"))
		float MetersToWorldScale = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "The maximum number of points"))
		int32 MaxPointCloudElements;

};
USTRUCT(BlueprintType)
struct TANGOPLUGIN_API FWGS_84_PoseData
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Frame of reference of this pose"))
		FTangoCoordinateFramePair FrameOfReference;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Status code of this pose"))
		ETangoPoseStatus StatusCode = ETangoPoseStatus::UNKNOWN;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Pose time in seconds since the device was started"))
		float Timestamp;
	
	/** Maintains required precision */
	double Position[3];
	double Orientation[4];
};

/*
	FTangoPose
	Used to represent information from a TangoPose object.
	Contains the representations of where the tango is within a particular co-ordinate system.
*/
USTRUCT(BlueprintType)
struct TANGOPLUGIN_API FTangoPoseData
{
	GENERATED_USTRUCT_BODY()
		//@NOTE: Accuracy will be supported in a future update of the Tango API
		//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango")
		//	float Accuracy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Position in the current frame of reference"))
		FVector Position;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Rotation in the current frame of reference"))
		FRotator Rotation;

	//@NOTE: This property is not exposed to Blueprint. Blueprint has no
	//representation of Quaternions, only FRotators.

	FQuat QuatRotation;

	//@NOTE: Confidence will be supported in a future update of the Tango API
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango")
	//	int32 Confidence;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Frame of reference of this pose"))
		FTangoCoordinateFramePair FrameOfReference;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Status code of this pose"))
		ETangoPoseStatus StatusCode = ETangoPoseStatus::UNKNOWN;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Pose time in seconds since the device was started"))
		float Timestamp;

	FTangoPoseData(FVector NewPosition = FVector(), FRotator NewRotation = FRotator(), FQuat NewQuatRotation = FQuat::Identity, FTangoCoordinateFramePair NewFrameOfReference = FTangoCoordinateFramePair(),
		ETangoPoseStatus NewStatusCode = ETangoPoseStatus::UNKNOWN, float NewTimestamp = 0.0f);
};

/*
	FTangoCameraIntrinsics
*/

USTRUCT(BlueprintType)
struct TANGOPLUGIN_API FTangoCameraIntrinsics
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tango", meta = (ToolTip = "Calibration type of the camera"))
		ETangoCalibrationType CalibrationType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Determines to which camera these intrinsics belong"))
		ETangoCameraType CameraID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Horizontal center offset in pixels"))
		int32 Cx;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Vertical center offset in pixels"))
		int32 Cy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Distortion parameters"))
		TArray<float> Distortion;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Horizontal focal length in pixels"))
		float Fx;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Vertical focal length in pixels"))
		float Fy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Width in pixels"))
		int32 Width;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Height in pixels"))
		int32 Height;
};


/*
	FTangoAreaDescriptionMetaData
*/
USTRUCT(BlueprintType)
struct TANGOPLUGIN_API FTangoAreaDescriptionMetaData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "File name of the ADF file"))
	FString Filename;


	uint64 MillisecondsSinceUnixEpoch;

	/** Maintains required precision */
	double Position[3];
	double Orientation[4];

	FTangoAreaDescriptionMetaData(const FString InFileName, const int64 InMillisecondsSinceUnixEpoch, double const InPosition[], double const InOrientation[]);
	FTangoAreaDescriptionMetaData() {};
};

UENUM(BlueprintType)
enum class ETangoImageFormat : uint8
{
	TANGO_HAL_PIXEL_FORMAT_RGBA_8888,
	TANGO_HAL_PIXEL_FORMAT_YV12,
	TANGO_HAL_PIXEL_FORMAT_YCrCb_420_SP,
};

USTRUCT(BlueprintType)
struct TANGOPLUGIN_API FTangoImageBuffer
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Tango camera type"))
		ETangoCameraType CameraType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Image width"))
		int32 Width;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Image height"))
		int32 Height;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Image timestamp"))
		FDateTime Timestamp;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Image frame number"))
		int32 FrameNumber;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Image format"))
		ETangoImageFormat ImageFormat;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Image data"))
		TArray<uint8> Data;
};


