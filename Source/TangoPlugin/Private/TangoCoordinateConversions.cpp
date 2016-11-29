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
#include "TangoCoordinateConversions.h"
#include "TangoDevice.h"
#include "TangoFromToCObject.h"

#if PLATFORM_ANDROID
#include "tango_client_api.h"
#endif

namespace 
{

	static TMap<ETangoCoordinateFrameType, TMap<ETangoCoordinateFrameType, TangoSpaceConversions::TangoSpaceConversionPair>> TangoSpaceConversionPairMapper;

	static bool bMatricesArePrepared = false;

	static bool GetOffsetMatrix(FTangoCoordinateFramePair Pair,FMatrix& Matrix)
	{
#if PLATFORM_ANDROID
		TangoPoseData Result;
		TangoErrorType ResultOfServiceCall;
		ResultOfServiceCall = TangoService_getPoseAtTime(0.0, ToCObject(Pair), &Result);
		FTangoPoseData D = FromCPointer(&Result);
		Matrix = FTransform(D.QuatRotation, D.Position).ToMatrixNoScale();
		if (!(ResultOfServiceCall == TANGO_SUCCESS && D.StatusCode == ETangoPoseStatus::VALID))
		{
			UE_LOG(TangoPlugin, Warning, TEXT("TangoSpaceConversions::GetOffsetMatrix: failed for %d and %d"), (int32)(Pair.BaseFrame), (int32)(Pair.TargetFrame));
		}
		return (ResultOfServiceCall == TANGO_SUCCESS && D.StatusCode == ETangoPoseStatus::VALID);
#else
		Matrix.SetIdentity();
		return true;
#endif
	}

	static bool PrepareMatrices()
	{
		if (bMatricesArePrepared)
		{
			return true;
		}
		FMatrix ADFtoUE;
		FMatrix DEVICEtoUE;
		FMatrix IMUtoUE;
		FMatrix COLORtoUE;
		FMatrix DISPLAYtoUE;

		FMatrix IMUtoDEVICE;
		FMatrix IMUtoCOLOR;
		FMatrix IMUtoDEPTH;
		FMatrix IMUtoFISHEYE;
		//FMatrix IMUtoDISPLAY;

		bool bSuccess = GetOffsetMatrix(FTangoCoordinateFramePair(ETangoCoordinateFrameType::IMU, ETangoCoordinateFrameType::DEVICE), IMUtoDEVICE);
		bSuccess = bSuccess && GetOffsetMatrix(FTangoCoordinateFramePair(ETangoCoordinateFrameType::IMU, ETangoCoordinateFrameType::CAMERA_COLOR), IMUtoCOLOR);
		bSuccess = bSuccess && GetOffsetMatrix(FTangoCoordinateFramePair(ETangoCoordinateFrameType::IMU, ETangoCoordinateFrameType::CAMERA_FISHEYE), IMUtoFISHEYE);
		bSuccess = bSuccess && GetOffsetMatrix(FTangoCoordinateFramePair(ETangoCoordinateFrameType::IMU, ETangoCoordinateFrameType::CAMERA_DEPTH), IMUtoDEPTH);
		//bSuccess = bSuccess && GetOffsetMatrix(FTangoCoordinateFramePair(ETangoCoordinateFrameType::IMU, ETangoCoordinateFrameType::DISPLAY), IMUtoDISPLAY);

		FMatrix DEVICEtoIMU = IMUtoDEVICE.Inverse();

		ADFtoUE = FMatrix::Identity;ADFtoUE.M[0][0] = 0;ADFtoUE.M[1][1] = 0;ADFtoUE.M[2][2] = 0;
		ADFtoUE.M[0][1] = 1;
		ADFtoUE.M[1][0] = 1;
		ADFtoUE.M[2][2] = 1;
		DEVICEtoUE = FMatrix::Identity;DEVICEtoUE.M[0][0] = 0;DEVICEtoUE.M[1][1] = 0;DEVICEtoUE.M[2][2] = 0;
		DEVICEtoUE.M[0][2] = -1;
		DEVICEtoUE.M[1][0] = 1;
		DEVICEtoUE.M[2][1] = 1;
		IMUtoUE = FMatrix::Identity;IMUtoUE.M[0][0] = 0;IMUtoUE.M[1][1] = 0;IMUtoUE.M[2][2] = 0;
		IMUtoUE.M[0][2] = -1;
		IMUtoUE.M[1][1] = 1;
		IMUtoUE.M[2][0] = -1;
		COLORtoUE = FMatrix::Identity;COLORtoUE.M[0][0] = 0;COLORtoUE.M[1][1] = 0;COLORtoUE.M[2][2] = 0;
		COLORtoUE.M[0][2] = 1;
		COLORtoUE.M[1][0] = 1;
		COLORtoUE.M[2][1] = -1;
		DISPLAYtoUE = FMatrix::Identity;DISPLAYtoUE.M[0][0] = 0;DISPLAYtoUE.M[1][1] = 0;DISPLAYtoUE.M[2][2] = 0;
		DISPLAYtoUE.M[0][2] = -1;
		DISPLAYtoUE.M[1][0] = 1;
		DISPLAYtoUE.M[2][1] = 1;

		TMap<ETangoCoordinateFrameType, FMatrix> ToUESpace;
		TMap<ETangoCoordinateFrameType, FMatrix> DeviceToOffset;

		ToUESpace.Emplace(ETangoCoordinateFrameType::AREA_DESCRIPTION,		ADFtoUE);
		ToUESpace.Emplace(ETangoCoordinateFrameType::START_OF_SERVICE,		ADFtoUE);
		ToUESpace.Emplace(ETangoCoordinateFrameType::PREVIOUS_DEVICE_POSE,	DEVICEtoUE);
		ToUESpace.Emplace(ETangoCoordinateFrameType::DEVICE,				DEVICEtoUE);
		ToUESpace.Emplace(ETangoCoordinateFrameType::IMU,					IMUtoUE);
		ToUESpace.Emplace(ETangoCoordinateFrameType::DISPLAY,				DISPLAYtoUE);
		ToUESpace.Emplace(ETangoCoordinateFrameType::CAMERA_COLOR,			COLORtoUE);
		ToUESpace.Emplace(ETangoCoordinateFrameType::CAMERA_DEPTH,			COLORtoUE);
		ToUESpace.Emplace(ETangoCoordinateFrameType::CAMERA_FISHEYE,		COLORtoUE);

		DeviceToOffset.Emplace(ETangoCoordinateFrameType::CAMERA_COLOR,		IMUtoCOLOR * DEVICEtoIMU);
		DeviceToOffset.Emplace(ETangoCoordinateFrameType::CAMERA_DEPTH,		IMUtoCOLOR * DEVICEtoIMU);
		DeviceToOffset.Emplace(ETangoCoordinateFrameType::CAMERA_FISHEYE,	IMUtoFISHEYE * DEVICEtoIMU);
		//DeviceToOffset.Emplace(ETangoCoordinateFrameType::DISPLAY,			DEVICEtoIMU * IMUtoDISPLAY);
		DeviceToOffset.Emplace(ETangoCoordinateFrameType::IMU,				DEVICEtoIMU);
		DeviceToOffset.Emplace(ETangoCoordinateFrameType::DEVICE,			FMatrix::Identity);

		TangoSpaceConversionPairMapper.Empty(9);
		for (int32 i = 1; i < 10; ++i)//Iterate over ETangoCoordinateFrameType and igore GLOBAL_WGS84
		{
			for (int32 j = 1; j < 10; ++j)//Iterate over ETangoCoordinateFrameType and igore GLOBAL_WGS84
			{
				TangoSpaceConversions::TangoSpaceConversionPair P;
				P.Pair.BaseFrame = (ETangoCoordinateFrameType)i;
				P.Pair.TargetFrame = (ETangoCoordinateFrameType)j;

				if (P.Pair.BaseFrame == ETangoCoordinateFrameType::DISPLAY || P.Pair.TargetFrame == ETangoCoordinateFrameType::DISPLAY)
				{
					//Display does not work for some reasons.
					continue;
				}

				P.UEtoBaseFrame = ToUESpace[P.Pair.BaseFrame].Inverse();
				P.TargetFrameToUE = ToUESpace[P.Pair.TargetFrame];

				if (DeviceToOffset.Contains(P.Pair.BaseFrame))//If we are querying from something on the device ...
				{
					if (!DeviceToOffset.Contains(P.Pair.TargetFrame))//... to something in the world ...
					{
						//... it does not work so we ignore it
						continue;
					}
					else //... yo something on the device ...
					{
						//... we have a static query that never changes
						P.OffsetFromDevice = DeviceToOffset[P.Pair.BaseFrame].Inverse() * DeviceToOffset[P.Pair.TargetFrame];
						P.bIsStatic = true;
						P.bNeedToBeQueriedFromDevice = false;
					}
				}
				else if (DeviceToOffset.Contains(P.Pair.TargetFrame)) //If we query from a world base frame to something on the device...
				{
					// ... we need to query to device and apply the offset manually
					P.OffsetFromDevice = DeviceToOffset[P.Pair.TargetFrame];
					P.bNeedToBeQueriedFromDevice = true;
					P.bIsStatic = false;
				}
				else // Anything else is just fine
				{
					P.OffsetFromDevice = FMatrix::Identity;
					P.bIsStatic = false;
					P.bNeedToBeQueriedFromDevice = false;
				}

				TangoSpaceConversionPairMapper.FindOrAdd(P.Pair.BaseFrame).Emplace(P.Pair.TargetFrame, P);
			}
		}

		bMatricesArePrepared = bSuccess;
		return bSuccess;
	}
}

bool TangoSpaceConversions::GetSpaceConversionPair(TangoSpaceConversionPair& Pair, const FTangoCoordinateFramePair& RefPair)
{
	bool bResult = PrepareMatrices();
	bResult = bResult && TangoSpaceConversionPairMapper.Contains(RefPair.BaseFrame);
	if (bResult)
	{
		bResult = bResult && TangoSpaceConversionPairMapper[RefPair.BaseFrame].Contains(RefPair.TargetFrame);
		if (bResult)
		{
			Pair = TangoSpaceConversionPairMapper[RefPair.BaseFrame][RefPair.TargetFrame];
		}
	}
	return bResult;
}

void TangoSpaceConversions::ModifyPose(FTangoPoseData& Pose, const TangoSpaceConversionPair& Converter)
{
	if (Converter.bIsStatic)//Just querying extrinsics
	{
		FTransform Transform = FTransform(Converter.TargetFrameToUE * Converter.OffsetFromDevice * Converter.UEtoBaseFrame);
		Pose.Position = Transform.GetLocation() * UTangoDevice::Get().GetMetersToWorldScale();
		Pose.QuatRotation = Transform.GetRotation();
		Pose.Rotation = Transform.GetRotation().Rotator();
		Pose.FrameOfReference = Converter.Pair;
		Pose.StatusCode = ETangoPoseStatus::VALID;
	}
	else
	{
		FTransform Transform = FTransform(Converter.TargetFrameToUE *  Converter.OffsetFromDevice *  FTransform(Pose.QuatRotation, Pose.Position).ToMatrixNoScale()* Converter.UEtoBaseFrame);
		Pose.Position = Transform.GetLocation() * UTangoDevice::Get().GetMetersToWorldScale();
		Pose.QuatRotation = Transform.GetRotation();
		Pose.Rotation = Transform.GetRotation().Rotator();
		Pose.FrameOfReference = Converter.Pair;
	}
}