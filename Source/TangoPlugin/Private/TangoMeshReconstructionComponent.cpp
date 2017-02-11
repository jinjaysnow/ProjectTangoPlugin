// Fill out your copyright notice in the Description page of Project Settings.
#include "TangoPluginPrivatePCH.h"
#include "TangoMeshReconstructionComponent.h"

class MeshRunner : public FRunnable
{
	TWeakObjectPtr<UTangoMeshReconstructionComponent> TargetPtr;
public:
	MeshRunner(UTangoMeshReconstructionComponent *Target) : TargetPtr(Target) {}

	uint32 Run() override
	{
		UTangoMeshReconstructionComponent* Target = TargetPtr.Get();
		if (Target)
		{
			Target->Run();
		}
		return 0;
	}
};

class MeshGenerator : public FRunnable
{
	TWeakObjectPtr<UTangoMeshReconstructionComponent> TargetPtr;
public:
	MeshGenerator(UTangoMeshReconstructionComponent *Target) : TargetPtr(Target) {}

	uint32 Run() override
	{
		UTangoMeshReconstructionComponent* Target = TargetPtr.Get();
		if (Target)
		{
			Target->RunGen();
		}
		return 0;
	}
};

// Sets default values for this component's properties
UTangoMeshReconstructionComponent::UTangoMeshReconstructionComponent() :
	Resolution(5),
	bGenerateColor(true),
	bUseSpaceClearing(true),
	bEnabled(true),
	BaseFrame(ETangoCoordinateFrameType::AREA_DESCRIPTION)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
#if PLATFORM_ANDROID
	t3dr_config_ = nullptr;
	t3dr_context_ = nullptr;
	Thread1 = nullptr;
	Thread2 = nullptr;
	ImageBufferManager = nullptr;
#endif
	// ...
}

void UTangoMeshReconstructionComponent::BeginPlay()
{
#if PLATFORM_ANDROID
	bPlaying = false; // have to wait for tango
#endif
	Super::BeginPlay();
}


void UTangoMeshReconstructionComponent::BeginPlay2()
{
#if PLATFORM_ANDROID
	if (!bEnabled)
	{
		return;
	}
	FScopeLock ScopeLock(&ImageBufferMutex);
	if (bPlaying)
	{
		return;
	}
	if (UTangoDevice::Get().GetTangoDevicePointCloudPointer() == nullptr || UTangoDevice::Get().GetTangoDeviceImagePointer() == nullptr)
	{
		return;
	}
	Tango3DR_Status t3dr_err;
	if (t3dr_config_ == nullptr)
	{
		t3dr_config_ =
			Tango3DR_Config_create(TANGO_3DR_CONFIG_CONTEXT);
	}
	t3dr_err = Tango3DR_Config_setDouble(t3dr_config_, "resolution", Resolution / 100.0f);
	if (t3dr_err != TANGO_3DR_SUCCESS) {
		UE_LOG(TangoPlugin, Error, TEXT("Tango3DR_Config set resolution failed with error code: %d"), t3dr_err);
		return;
	}
	t3dr_err = Tango3DR_Config_setBool(t3dr_config_, "generate_color", bGenerateColor);
	if (t3dr_err != TANGO_3DR_SUCCESS) {
		UE_LOG(TangoPlugin, Error, TEXT("Tango3DR_Config  generate_color failed with error code: %d"), t3dr_err);
		return;
	}
	t3dr_err = Tango3DR_Config_setBool(t3dr_config_, "use_space_clearing", bUseSpaceClearing);
	if (t3dr_err != TANGO_3DR_SUCCESS) {
		UE_LOG(TangoPlugin, Warning, TEXT("Tango3DR_Config  use_space_clearing failed with error code: %d"), t3dr_err);
	}
	if (t3dr_context_ == nullptr)
	{
		t3dr_context_ = Tango3DR_create(t3dr_config_);
		if (t3dr_context_ == nullptr) {
			UE_LOG(TangoPlugin, Error, TEXT("Tango3DR_create failed with error code: %d"), t3dr_err);
			return;
		}
	}
	// Update the camera intrinsics too.
	TangoCameraIntrinsics intrinsics;
	int32 err = TangoService_getCameraIntrinsics(TANGO_CAMERA_COLOR, &intrinsics);
	if (err != TANGO_SUCCESS) {
		UE_LOG(TangoPlugin, Error, TEXT(
			"TangoMeshReconstructionComponent: Failed to get camera intrinsics with error code: %d"),
			err);
		return;
	}
	t3dr_intrinsics_.calibration_type =
		static_cast<Tango3DR_TangoCalibrationType>(intrinsics.calibration_type);
	t3dr_intrinsics_.width = intrinsics.width;
	t3dr_intrinsics_.height = intrinsics.height;
	t3dr_intrinsics_.fx = intrinsics.fx;
	t3dr_intrinsics_.fy = intrinsics.fy;
	t3dr_intrinsics_.cx = intrinsics.cx;
	t3dr_intrinsics_.cy = intrinsics.cy;
	// Configure the color intrinsics to be used with updates to the mesh.
	t3dr_err = Tango3DR_setColorCalibration(t3dr_context_, &t3dr_intrinsics_);
	if (t3dr_context_ == nullptr)
	{
		UE_LOG(TangoPlugin, Error, TEXT("Tango3DR_setColorCalibration failed with error code: %d"), t3dr_err);
		return;
	}
	bPlaying = true;
	Thread1 = FRunnableThread::Create(new MeshRunner(this), TEXT("MeshRunner"));
	Thread2 = FRunnableThread::Create(new MeshGenerator(this), TEXT("MeshGenerator"));
	if (!ImageListener.IsValid())
	{
		ImageListener = UTangoDevice::Get().GetTangoDeviceImagePointer()->OnImageBufferAvailable.AddUObject(this, &UTangoMeshReconstructionComponent::OnImageBufferAvailable);
	}
#endif
}
#if PLATFORM_ANDROID
void UTangoMeshReconstructionComponent::ReleaseResources()
{
	bPlaying = false;
	if (Thread1 != nullptr)
	{
		Thread1->WaitForCompletion();
		delete Thread1;
		Thread1 = nullptr;
	}
	if (Thread2 != nullptr)
	{
		Thread2->WaitForCompletion();
		delete Thread2;
		Thread2 = nullptr;
	}

	if (ImageListener.IsValid())
	{
		if (UTangoDevice::Get().GetTangoDeviceImagePointer() != nullptr)
		{
			UTangoDevice::Get().GetTangoDeviceImagePointer()->OnImageBufferAvailable.Remove(ImageListener);
		}
		ImageListener.Reset();
	}
	if (t3dr_config_ != nullptr)
	{
		Tango3DR_Config_destroy(t3dr_config_);
		t3dr_config_ = nullptr;
	}
	if (ImageBufferManager != nullptr)
	{
		TangoSupportImageBufferManager* Tmp = ImageBufferManager;
		ImageBufferManager = nullptr;
		TangoSupport_freeImageBufferManager(Tmp);
	}
	SectionAddressMap.Reset();
	Sections.Reset();
	
}
#endif

void UTangoMeshReconstructionComponent::EndPlay(EEndPlayReason::Type Reason)
{
#if PLATFORM_ANDROID
	ReleaseResources();
#endif
	Super::EndPlay(Reason);
}

void UTangoMeshSection::InitVertexAttrs()
{
#if PLATFORM_ANDROID
	const int kInitialVertexCount = 100;
	const int kInitialIndexCount = 99;
	Vertices.SetNumUninitialized(kInitialVertexCount);
	Normals.SetNumUninitialized(kInitialVertexCount);
	Triangles.SetNumUninitialized(kInitialIndexCount);
	TangoColors.SetNumUninitialized(kInitialVertexCount);
#endif
}

// Called every frame
void UTangoMeshReconstructionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
#if PLATFORM_ANDROID
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bEnabled)
	{
		if (bPlaying)
		{
			ReleaseResources();
		}
		return;
	}
	if (!UTangoDevice::Get().IsTangoServiceRunning())
	{
		if (bPlaying)
		{
			ReleaseResources();
		}
		return;
	}
	if (!bPlaying)
	{
		BeginPlay2();
	}
#endif
}

void UTangoMeshReconstructionComponent::Run()
{
#if PLATFORM_ANDROID
	while (bPlaying)
	{
		{
			FScopeLock ScopeLock(&UpdatedIndicesMutex);
			TSet<FSectionAddress>::TConstIterator It(this->UpdatedIndices);
			for (; It; ++It)
			{
				UpdatedIndicesLocal.Add(*It);
			}
			this->UpdatedIndices.Reset();
		}
		if (UpdatedIndicesLocal.Num() == 0)
		{
			FGenericPlatformProcess::Sleep(1.0 / 60);
		}
		const double Start = FPlatformTime::Seconds();
		for (int32 i = UpdatedIndicesLocal.Num() - 1; i >= 0; --i)
		{
			const FSectionAddress& SectionAddress = UpdatedIndicesLocal[i];
			int32* SectionIndexPtr = SectionAddressMap.Find(SectionAddress);
			int32 SectionIndex;
			if (SectionIndexPtr == nullptr)
			{
				SectionIndex = Sections.Num();
				UTangoMeshSection* NewSection = NewObject<UTangoMeshSection>();
				Sections.Add(NewSection);
				NewSection->SectionIndex = SectionIndex;
				NewSection->InitVertexAttrs();
				SectionAddressMap.Add(SectionAddress, SectionIndex);
			}
			else
			{
				SectionIndex = *SectionIndexPtr;
			}
			UTangoMeshSection* SectionPtr = Sections[SectionIndex];
			if (SectionPtr->bOnGameThread) 
			{
				continue;
			}
			UTangoMeshSection& Section = *SectionPtr;
			Section.SectionIndex = SectionIndex;
			Tango3DR_Mesh tango_mesh =
			{
				/* timestamp */ 0.0,
				/* num_vertices */ 0u,
				/* num_faces */ 0u,
				/* num_textures */ 0u,
				/* max_num_vertices */ static_cast<uint32_t>(
					Section.Vertices.Max()),
				/* max_num_faces */ static_cast<uint32_t>(
					Section.Triangles.Max() / 3),
				/* max_num_textures */ 0u,
				/* vertices */ reinterpret_cast<Tango3DR_Vector3*>(
					Section.Vertices.GetData()),
				/* faces */ reinterpret_cast<Tango3DR_Face*>(
					Section.Triangles.GetData()),
				/* normals */ reinterpret_cast<Tango3DR_Vector3*>(
					Section.Normals.GetData()),
				/* colors */ reinterpret_cast<Tango3DR_Color*>(
					bGenerateColor ? Section.TangoColors.GetData() : nullptr),
				/* texture_coords */ nullptr,
				/*texture_ids */ nullptr,
				/* textures */ nullptr
			};

			Tango3DR_Status err = Tango3DR_extractPreallocatedMeshSegment(
				t3dr_context_, (int*)&SectionAddress, &tango_mesh);
			if (err == TANGO_3DR_ERROR)
			{
				UE_LOG(TangoPlugin, Error, TEXT("extractPreallocatedMeshSegment failed with error code: %d"), err);
				continue;
			}
			else if (err == TANGO_3DR_INSUFFICIENT_SPACE)
			{
				UE_LOG(TangoPlugin, Log, TEXT("extractPreallocatedMeshSegment ran out of space with room for %d vertices, %d indices."),
					Section.Vertices.Max(),
					Section.Triangles.Max());
				int32 Num = Section.Vertices.Num();
				Section.Vertices.SetNumUninitialized(Num * 2, false);
				Section.Normals.SetNumUninitialized(Num * 2, false);
				Section.Triangles.SetNumUninitialized(Num * 6, false);
				Section.TangoColors.SetNumUninitialized(Num * 2, false);
				++i;
			}
			else
			{
				int32 num_vertices = tango_mesh.num_vertices;
				if (num_vertices > 0)
				{
					int32 num_triangles = tango_mesh.num_faces * 3;
					Section.Vertices.SetNumUninitialized(num_vertices, false);
					Section.Normals.SetNumUninitialized(num_vertices, false);
					Section.Triangles.SetNumUninitialized(num_triangles, false);
					if (bGenerateColor)
					{
						Section.TangoColors.SetNumUninitialized(num_vertices, false);
					}
					else
					{
						Section.TangoColors.Empty();
					}
					//UE_LOG(TangoPlugin, Log, TEXT("MeshSection id: %d, Verts: %d, Tris: %d, Colors %d"), Section.SectionIndex, Section.Vertices.Num(), Section.Triangles.Num(), Section.TangoColors.Num());
					// Convert to UE conventions
					if (bGenerateColor)
					{
						Section.VertexColors.SetNumUninitialized(num_vertices, false);
					}
					else
					{
						Section.VertexColors.Empty();
					}
					for (int32 j = 0; j < Section.Vertices.Num(); j++)
					{
						FVector& Vert = Section.Vertices[j];
						FVector& Norm = Section.Normals[j];
						switch (BaseFrame)
						{
						case ETangoCoordinateFrameType::AREA_DESCRIPTION:
							Vert = FVector(Vert.Y, Vert.X, Vert.Z) * 100;
							Norm = FVector(Norm.Y, Norm.X, Norm.Z);
							break;
						case ETangoCoordinateFrameType::START_OF_SERVICE:
							Vert = FVector(-Vert.Z, Vert.X, Vert.Y) * 100;
							Norm = FVector(-Norm.Z, Norm.X, Norm.Y);
							break;
						default:
							UE_LOG(TangoPlugin, Error, TEXT("Unsupported base frame %d, should be AREA_DESCRIPTION or START_OF_SERVICE"), BaseFrame);
						}
						if (bGenerateColor)
						{
							FColor Color = Section.TangoColors[j];
							// convert RGBA to BGRA
							//UE_LOG(TangoPlugin, Log, TEXT("Color %d, %d, %d, %d"), Color.R, Color.B, Color.G, Color.A);
							Section.VertexColors[j] = FLinearColor(FColor(Color.B, Color.G, Color.R, Color.A));
						}
					}
					SectionPtr->bOnGameThread.AtomicSet(true);
					UTangoDevice::RunOnGameThread([=]()->void {
						OnMeshSectionUpdated.Broadcast(SectionPtr);
						SectionPtr->bOnGameThread.AtomicSet(false);
					});
				}
				else
				{
					if (SectionIndexPtr == nullptr)
					{
						Sections.SetNum(Sections.Num() - 1);
						SectionAddressMap.Remove(SectionAddress);
					}
				}
				UpdatedIndicesLocal.RemoveAt(i);
			}
		}
	}
#endif

}
#if PLATFORM_ANDROID

static void extract3DRPose(const TangoPoseData* data, Tango3DR_Pose* pose)
{
	pose->translation[0] = data->translation[0];
	pose->translation[1] = data->translation[1];
	pose->translation[2] = data->translation[2];
	pose->orientation[0] = data->orientation[0];
	pose->orientation[1] = data->orientation[1];
	pose->orientation[2] = data->orientation[2];
	pose->orientation[3] = data->orientation[3];
}

void UTangoMeshReconstructionComponent::OnImageBufferAvailable(const TangoImageBuffer* buffer)
{
	
	if (!bPlaying) return;
	if (UTangoDevice::Get().GetTangoDevicePointCloudPointer() == nullptr)
	{
		return;
	}
	if (t3dr_context_ == nullptr)
	{
		return;
	}
	if (ImageBufferManager == nullptr)
	{
		if (TangoSupport_createImageBufferManager(buffer->format, buffer->width, buffer->height, &ImageBufferManager) != TANGO_SUCCESS)
		{
			UE_LOG(TangoPlugin, Error, TEXT("Couldn't create image buffer manager"));
			return;
		}
	}
	if (TangoSupport_updateImageBuffer(ImageBufferManager, buffer) != TANGO_SUCCESS)
	{
		UE_LOG(TangoPlugin, Error, TEXT("updateImageBuffer failed"));
	}
}

void UTangoMeshReconstructionComponent::RunGen()
{
	while (bPlaying)
	{
		if (ImageBufferManager == nullptr)
		{
			FGenericPlatformProcess::Sleep(0.25);
			continue;
		}
		bool NewImage = false;
		TangoImageBuffer* image_buffer;
		if (TangoSupport_getLatestImageBufferAndNewDataFlag(
			ImageBufferManager,
			&image_buffer,
			&NewImage
		) != TANGO_SUCCESS)
		{
			UE_LOG(TangoPlugin, Error, TEXT("getLatestImageBuffer failed"));
		}
		else
		{
			if (NewImage)
			{
				ProcessImageBuffer(image_buffer);
				continue;
			}
		}
		FGenericPlatformProcess::Sleep(1.0/60);
	}
}

void UTangoMeshReconstructionComponent::ProcessImageBuffer(const TangoImageBuffer* buffer)
{
	
	//UE_LOG(TangoPlugin, Log, TEXT("OnImageBufferAvailable bPlaying: %d %p image %p"), bPlaying, t3dr_context_, buffer);
	Tango3DR_ImageBuffer t3dr_image;
	t3dr_image.width = buffer->width;
	t3dr_image.height = buffer->height;
	t3dr_image.stride = buffer->stride;
	t3dr_image.timestamp = buffer->timestamp;
	t3dr_image.format = static_cast<Tango3DR_ImageFormatType>(buffer->format);
	t3dr_image.data = buffer->data;

	Tango3DR_Pose t3dr_image_pose;
	TangoCoordinateFrameType TangoBaseFrame = (TangoCoordinateFrameType)BaseFrame;
	TangoPoseData Data;
	if (TangoSupport_getPoseAtTime(
		buffer->timestamp, TangoBaseFrame,
		TANGO_COORDINATE_FRAME_CAMERA_COLOR, TANGO_SUPPORT_ENGINE_TANGO,
		ROTATION_0, &Data) != TANGO_SUCCESS)
	{
		UE_LOG(TangoPlugin, Error, TEXT("getPoseAtTime failed"));
		return;
	}
	extract3DRPose(&Data, &t3dr_image_pose);
	TangoDevicePointCloud* DevicePointCloud = UTangoDevice::Get().GetTangoDevicePointCloudPointer();
	if (DevicePointCloud == nullptr)
	{
		return;
	}
	FScopeLock PointCloudScopeLock(DevicePointCloud->GetPointCloudLock());
	const TangoPointCloud* _front_cloud = DevicePointCloud->GetLatestPointCloud();
	if (_front_cloud == nullptr)
	{
		return;
	}
	//UE_LOG(TangoPlugin, Log, TEXT("OnImageBufferAvailable front_cloud_->num_points: %d"), _front_cloud->num_points);
	if (_front_cloud->num_points == 0)
	{
		return;
	}
	Tango3DR_PointCloud t3dr_depth;
	t3dr_depth.timestamp = _front_cloud->timestamp;
	t3dr_depth.num_points = _front_cloud->num_points;
	t3dr_depth.points = reinterpret_cast<Tango3DR_Vector4*>(_front_cloud->points);

	Tango3DR_Pose t3dr_depth_pose;
	if (TangoSupport_getPoseAtTime(
		_front_cloud->timestamp, TangoBaseFrame,
		TANGO_COORDINATE_FRAME_CAMERA_DEPTH, TANGO_SUPPORT_ENGINE_TANGO,
		ROTATION_0, &Data) != TANGO_SUCCESS)
	{
		UE_LOG(TangoPlugin, Error, TEXT("getPoseAtTime failed"));
		return;
	}
	extract3DRPose(&Data, &t3dr_depth_pose);
	Tango3DR_GridIndexArray* t3dr_updated = nullptr;
	Tango3DR_Status t3dr_err =
		Tango3DR_update(t3dr_context_, &t3dr_depth, &t3dr_depth_pose, &t3dr_image,
			&t3dr_image_pose, &t3dr_updated);
	if (t3dr_err != TANGO_3DR_SUCCESS)
	{
		UE_LOG(TangoPlugin, Error, TEXT("Tango3DR_update failed with error code: %d"), t3dr_err);
		return;
	}
	{
		//UE_LOG(TangoPlugin, Log, TEXT("OnImageBufferAvailable T3DR_update succeeded %d"), t3dr_updated->num_indices);
		if (t3dr_updated->num_indices > 0)
		{
			FScopeLock ScopeLock(&UpdatedIndicesMutex);
			const FSectionAddress* Arr = (const FSectionAddress*)&t3dr_updated->indices[0][0];
			for (int32 i = 0; i < t3dr_updated->num_indices; i++)
			{
				UpdatedIndices.Add(Arr[i]);
			}
		}
	}
	Tango3DR_GridIndexArray_destroy(t3dr_updated);
}


#endif