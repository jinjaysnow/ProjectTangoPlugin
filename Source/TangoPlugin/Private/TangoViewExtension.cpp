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
#include "TangoViewExtension.h"
#include "TangoMotionComponent.h"
#include "TangoARCamera.h"
#include "TangoDevice.h"
#include "TangoImageComponent.h"
#include "TangoDataTypes.h"
#include "ITangoAR.h"
#include "TangoARHelpers.h"
#include "PrimitiveSceneInfo.h"

#if PLATFORM_ANDROID
#include "tango_client_api.h"
#endif

UTangoARInterface::UTangoARInterface(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

namespace {
	/** This is to prevent destruction of tango motion components while they are
	in the middle of being accessed by the render thread */
	FCriticalSection CritSect;
} // anonymous namespace

UTangoMotionComponent::~UTangoMotionComponent()
{
	if (ViewExtension.IsValid())
	{
		{
			// This component could be getting accessed from the render thread so it needs to wait
			// before clearing MotionControllerComponent and allowing the destructor to continue
			FScopeLock ScopeLock(&CritSect);
			ViewExtension->ARComponent = nullptr;
		}

		if (GEngine)
		{
			GEngine->ViewExtensions.Remove(ViewExtension);
		}
	}
	ViewExtension.Reset();
}

UTangoARCamera::~UTangoARCamera()
{
	if (ViewExtension.IsValid())
	{
		{
			// This component could be getting accessed from the render thread so it needs to wait
			// before clearing MotionControllerComponent and allowing the destructor to continue
			FScopeLock ScopeLock(&CritSect);
			ViewExtension->ARComponent = nullptr;
		}

		if (GEngine)
		{
			GEngine->ViewExtensions.Remove(ViewExtension);
		}
	}
	ViewExtension.Reset();
}


bool FTangoViewExtension::IdentifyViewWithCameraComponent(const FSceneView* InView,int32 Stride)
{
	FScopeLock ScopeLock(&CritSect);
	for (int32 i = 0; i < Cameras[Stride].Num(); ++i)
	{
		/*if(InView->ViewActor != nullptr)
		{
			UE_LOG(TangoPlugin, Log, TEXT("Actorname: %s compared to: %s and there also is: %s"), *InView->ViewActor->GetName(), *CameraActors[i]->GetName(),*ARComponent->GetActor()->GetName());
		}*/
		if (Cameras[Stride][i].Actor == InView->ViewActor)
		{
			//UE_LOG(TangoPlugin, Log, TEXT("FTangoViewExtension::IdentifyViewWithCameraComponent: identified Actor"));
			if (InView->ViewLocation.X == Cameras[Stride][i].Pos.X && InView->ViewLocation.Y == Cameras[Stride][i].Pos.Y &&InView->ViewLocation.Z == Cameras[Stride][i].Pos.Z &&
				InView->ViewRotation.Pitch == Cameras[Stride][i].Rot.Pitch &&InView->ViewRotation.Roll == Cameras[Stride][i].Rot.Roll && InView->ViewRotation.Yaw == Cameras[Stride][i].Rot.Yaw)
			{
				//UE_LOG(TangoPlugin, Log, TEXT("FTangoViewExtension::IdentifyViewWithCameraComponent: identified position"));
				return true;
			}
		}
	}
#if !WITH_EDITOR
	//UE_LOG(TangoPlugin, Warning, TEXT("FTangoViewExtension::IdentifyViewWithCameraComponent: Did not identify Actor"));
#endif
	return false;
}

const bool FTangoViewExtension::GetLateUpdateTransform(FTransform& Transform,int32 Stride, bool bIsAbsolute)
{
	bool bIsNew = false;
	FScopeLock ScopeLock(&CritSect);

	if (!ARComponent)
	{
		return false;
	}

	if (ARComponent->WantToDoAR())
	{
		if (UTangoDevice::Get().GetTangoDeviceImagePointer())
		{
			float CurrentTime = UTangoDevice::Get().GetTangoDeviceImagePointer()->GetImageBufferTimestamp();
			if (LatestPoseStamp < CurrentTime)
			{
				if (UTangoDevice::Get().GetTangoDeviceMotionPointer())
				{
					LatestPose = ARComponent->GetCurrentPoseRENDERTHREAD(CurrentTime);
					LatestPoseStamp = CurrentTime;
					bIsNew = true;
				}
				else
				{
					Transform = FTransform::Identity;
					return false;
				}
			}
		}
	}
	if (PoseFrame[Stride] != FrameNumber[Stride])
	{
		if (!ARComponent->WantToDoAR())
		{
			LatestPose = ARComponent->GetCurrentPoseRENDERTHREAD(0);
			LatestPoseStamp = 0;
		}
		Poses[Stride] = LatestPose;
		PoseFrame[Stride] = FrameNumber[Stride];
	}
	//Only move the component if the pose status is valid.
	if (Poses[Stride].StatusCode == ETangoPoseStatus::VALID)
	{
		if (bIsAbsolute)
		{
			Transform = ARComponent->CalcComponentToWorld(FTransform(Poses[Stride].Rotation, Poses[Stride].Position));
			return bIsNew;
		}
		else if(ARComponent)
		{
			const FTransform OldLocalToWorldTransform = ARComponent->CalcComponentToWorld(ARComponent->AsSceneComponent()->GetRelativeTransform());
			const FTransform NewLocalToWorldTransform = ARComponent->CalcComponentToWorld(FTransform(Poses[Stride].Rotation, Poses[Stride].Position));
			Transform = (OldLocalToWorldTransform.Inverse() * NewLocalToWorldTransform);
			return bIsNew;
		}
	}
#if !WITH_EDITOR
	//UE_LOG(TangoPlugin, Warning, TEXT("FTangoViewExtension::GetLateUpdateTransform: Failed because pose is invalid!"));
#endif
	Transform = FTransform::Identity;
	return false;
}

void FTangoViewExtension::PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily)
{
	if (UTangoDevice::Get().GetTangoDeviceImagePointer())
	{
		if (UTangoDevice::Get().GetTangoDeviceImagePointer()->IsNewDataAvail())
		{
			double Stamp = 0.0;
#if PLATFORM_ANDROID
				TangoService_updateTexture(TANGO_CAMERA_COLOR, &Stamp);
#endif
				UTangoDevice::Get().GetTangoDeviceImagePointer()->DataSet(Stamp);
		}
	}
	if (!ARComponent)
	{
		return;
	}

	FTransform NewTransform;
	FScopeLock ScopeLock(&CritSect);
	int32 S = FindStride(InViewFamily.FrameNumber);
	// Apply adjustment to the affected scene proxies
	if (S >= 0)
	{
		if (Cameras[S].Num() > 0)
		{
			return;
		}
		GetLateUpdateTransform(NewTransform, S, false);
		FMatrix LateUpdateMatrix = NewTransform.ToMatrixWithScale();
		for (auto PrimitiveInfo : LateUpdateSceneProxies[S])
		{
			FPrimitiveSceneInfo* RetrievedSceneInfo = InViewFamily.Scene->GetPrimitiveSceneInfo(*PrimitiveInfo.IndexAddress);
			FPrimitiveSceneInfo* CachedSceneInfo = PrimitiveInfo.SceneInfo;
			// If the retrieved scene info is different than our cached scene info then the primitive was removed from the scene
			if (CachedSceneInfo == RetrievedSceneInfo && CachedSceneInfo->Proxy)
			{
				CachedSceneInfo->Proxy->ApplyLateUpdateTransform(LateUpdateMatrix);
			}
		}
	}
	else
	{
		UE_LOG(TangoPlugin, Warning, TEXT("FTangoViewExtension::PreRenderViewFamily_RenderThread: No Stride found!"))
	}
}

void FTangoViewExtension::PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView)
{

	if (!ARComponent)
	{
		return;
	}
	FScopeLock ScopeLock(&CritSect);
	int32 S = FindStride(InView.Family->FrameNumber);
	//UE_LOG(ProjectTangoPlugin, Log, TEXT("FTangoViewExtension::PreRenderView_RenderThread: Called!"));
	if (!IdentifyViewWithCameraComponent(&InView, S))
	{
		return;
	}
	//UE_LOG(ProjectTangoPlugin, Log, TEXT("FTangoViewExtension::PreRenderView_RenderThread: View found!"));
	if (InView.Family)
	{
		if (S >= 0)
		{
			FTransform ViewTransform = FTransform(InView.ViewRotation, InView.ViewLocation);
			FTransform LateUpdateTransform;
			GetLateUpdateTransform(LateUpdateTransform, S, false);
			ViewTransform = ViewTransform * LateUpdateTransform;
			InView.ViewLocation = ViewTransform.GetLocation();
			InView.ViewRotation = ViewTransform.Rotator();
			if (ARComponent->WantToDoAR())
			{
				InView.ProjectionMatrixUnadjustedForRHI = TangoARHelpers::GetARProjectionMatrix();
				// major hack:
				*(FMatrix*)&InView.ViewMatrices.GetProjectionMatrix() = AdjustProjectionMatrixForRHI(InView.ProjectionMatrixUnadjustedForRHI);
			}
			InView.UpdateViewMatrix();
			FMatrix LateUpdateMatrix = LateUpdateTransform.ToMatrixWithScale();
			for (auto PrimitiveInfo : LateUpdateSceneProxies[S])
			{
				FPrimitiveSceneInfo* RetrievedSceneInfo = InView.Family->Scene->GetPrimitiveSceneInfo(*PrimitiveInfo.IndexAddress);
				FPrimitiveSceneInfo* CachedSceneInfo = PrimitiveInfo.SceneInfo;
				// If the retrieved scene info is different than our cached scene info then the primitive was removed from the scene
				if (CachedSceneInfo == RetrievedSceneInfo && CachedSceneInfo->Proxy)
				{
					CachedSceneInfo->Proxy->ApplyLateUpdateTransform(LateUpdateMatrix);
				}
			}
		}
		else if (ARComponent->WantToDoAR())
		{
			UE_LOG(TangoPlugin, Warning, TEXT("FTangoViewExtension::PreRenderView_RenderThread: No Stride could be found!"));
			InView.ProjectionMatrixUnadjustedForRHI = TangoARHelpers::GetARProjectionMatrix();
			// major hack:
			*(FMatrix*)&InView.ViewMatrices.GetProjectionMatrix() = AdjustProjectionMatrixForRHI(InView.ProjectionMatrixUnadjustedForRHI);

			InView.UpdateViewMatrix();
		}
	}
	else if (ARComponent->WantToDoAR())
	{
		UE_LOG(TangoPlugin, Warning, TEXT("FTangoViewExtension::PreRenderView_RenderThread: InView.Family is nullptr!"));
		InView.ProjectionMatrixUnadjustedForRHI = TangoARHelpers::GetARProjectionMatrix();
		// major hack:
		*(FMatrix*)&InView.ViewMatrices.GetProjectionMatrix() = AdjustProjectionMatrixForRHI(InView.ProjectionMatrixUnadjustedForRHI);
		InView.UpdateViewMatrix();
	}

}

void FTangoViewExtension::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{
	if (!ARComponent)
	{
		return;
	}
	FScopeLock ScopeLock(&CritSect);
	if (!ARComponent)
	{
		return;
	}
	//UE_LOG(TangoPlugin, Log, TEXT("FTangoViewExtension::BeginRenderViewFamily: Called"));
	CurrentStride = (CurrentStride + 1) % MaxCameraStride;
	LateUpdateSceneProxies[CurrentStride].Empty();
	Cameras[CurrentStride].Empty();
	FrameNumber[CurrentStride] = InViewFamily.FrameNumber;
	GatherSceneProxiesAndCameras(ARComponent->AsSceneComponent());

	//UE_LOG(TangoPlugin, Log, TEXT("FTangoViewExtension::BeginRenderViewFamily: Found %d Sceneproxies and %d Cameras!"), LateUpdateSceneProxyCount, CameraCount);
}

int32 FTangoViewExtension::FindStride(int32 Time)
{
	for (int32 i = 0; i < MaxCameraStride; ++i)
	{
		if (FrameNumber[i] == Time)
		{
			return i;
		}
	}
	return -1;
}

void FTangoViewExtension::GatherSceneProxiesAndCameras(USceneComponent* Component)
{
	// If a scene proxy is present, cache it
	UPrimitiveComponent* PrimitiveComponent = dynamic_cast<UPrimitiveComponent*>(Component);
	if (PrimitiveComponent && PrimitiveComponent->SceneProxy)
	{
		FPrimitiveSceneInfo* PrimitiveSceneInfo = PrimitiveComponent->SceneProxy->GetPrimitiveSceneInfo();
		if (PrimitiveSceneInfo)
		{
			LateUpdatePrimitiveInfo PrimitiveInfo;
			PrimitiveInfo.IndexAddress = PrimitiveSceneInfo->GetIndexAddress();
			PrimitiveInfo.SceneInfo = PrimitiveSceneInfo;
			LateUpdateSceneProxies[CurrentStride].Emplace(PrimitiveInfo);
		}
	}
	UCameraComponent* CameraComponent = dynamic_cast<UCameraComponent*>(Component);
	if (CameraComponent)
	{
		auto Transform = CameraComponent->GetComponentTransform(); //.Inverse();
		Cameras[CurrentStride].Emplace(CamData(Transform.GetTranslation(), Transform.Rotator(), CameraComponent->GetOwner()));
	}

	// Gather children proxies
	const int32 ChildCount = Component->GetNumChildrenComponents();
	for (int32 ChildIndex = 0; ChildIndex < ChildCount; ++ChildIndex)
	{
		USceneComponent* ChildComponent = Component->GetChildComponent(ChildIndex);
		if (!ChildComponent)
		{
			continue;
		}

		GatherSceneProxiesAndCameras(ChildComponent);
	}
}