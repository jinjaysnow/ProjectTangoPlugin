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
#include "TangoARScreenComponent.h"
#include "TangoDevice.h"


UTangoARScreenComponent::UTangoARScreenComponent() : Super()
{
	PrimaryComponentTick.bCanEverTick = true;
	auto MeshFinder = ConstructorHelpers::FObjectFinder<UStaticMesh>(TEXT("StaticMesh'/TangoPlugin/TangoPlane.TangoPlane'"));
	auto RGBMaterialFinder = ConstructorHelpers::FObjectFinder<UMaterial>(TEXT("Material'/TangoPlugin/TangoCameraPassthroughRGBAMaterial.TangoCameraPassthroughRGBAMaterial'"));
	if (MeshFinder.Succeeded())
	{
		FoundMesh = MeshFinder.Object;
	}
	else
	{
		FoundMesh = nullptr;
		UE_LOG(TangoPlugin, Warning, TEXT("UTangoARScreenComponent::UTangoARScreenComponent: Unable to retrieve mesh from common folder!"));
	}
	if (RGBMaterialFinder.Succeeded())
	{
		RGBMaterial = RGBMaterialFinder.Object;
	}
	else
	{
		RGBMaterial = nullptr;
		UE_LOG(TangoPlugin, Warning, TEXT("UTangoARScreenComponent::UTangoARScreenComponent: Unable to retrieve material from common folder!"));
	}
	bCastDynamicShadow = false;

}

float UTangoARScreenComponent::GetLatestImageTimeStamp()
{
	if (UTangoDevice::Get().GetTangoDeviceImagePointer())
	{
		return UTangoDevice::Get().GetTangoDeviceImagePointer()->GetLastTimestamp();
	}
	else
	{
		UE_LOG(TangoPlugin, Warning, TEXT("UTangoARScreenComponent::GetLatestImageTimeStamp: Unable to retrieve timestamp!"));
		return 0.0f;
	}
}

void UTangoARScreenComponent::BeginPlay()
{
	Super::BeginPlay();
}

/*
 *  UTangoARScreenComponent::SetupMaterial()
 *  This function sets up the components of the ARCamera component to allow for the passthrough camera functionality to take place without the user needing to perform boilerplace setup.
 *  It references materials and meshes stored within the content folder of the plugin, and adds them to the scene in the correct arrangement in order to achieve the passthrough camera effect.
 */
void UTangoARScreenComponent::SetupMaterial()
{
	UE_LOG(TangoPlugin, Log, TEXT("UTangoARScreenComponent::SetupMaterial: Called!"));
	if (GetStaticMesh() == nullptr)
	{
		SetStaticMesh(FoundMesh);
		if (FoundMesh == nullptr)
		{
			UE_LOG(TangoPlugin, Error, TEXT("UTangoARScreenComponent::SetupMaterial: Mesh is null and unable to locate default mesh!"));
			return;
		}
	}

	UMaterial* FoundMaterial = RGBMaterial;
	if (FoundMaterial)
	{
		auto Inst = UMaterialInstanceDynamic::Create(FoundMaterial, this);
		Inst->SetTextureParameterValue(FName("VideoTexture"), UTangoDevice::Get().GetTangoDeviceImagePointer()->VideoTexture);
		SetMaterial(0, Inst);
		bInitializedMaterial = true;
		UE_LOG(TangoPlugin, Log, TEXT("UTangoARScreenComponent::SetupMaterial: Success!"));
	}
	else
	{
		UE_LOG(TangoPlugin, Error, TEXT("UTangoARScreenComponent::SetupMaterial: Material could not be located!"));
		return;
	}
	
}

void UTangoARScreenComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	if (!bInitializedMaterial)
	{
		if (UTangoDevice::Get().GetTangoDeviceImagePointer())
		{
			if (UTangoDevice::Get().GetTangoDeviceImagePointer()->VideoTexture && UTangoDevice::Get().GetTangoDeviceImagePointer()->GetLastTimestamp() != 0)
			{
				SetupMaterial();
			}
		}
	}
	else if (UTangoDevice::Get().GetTangoDeviceImagePointer() == nullptr)
	{
		bInitializedMaterial = false;
	}
}