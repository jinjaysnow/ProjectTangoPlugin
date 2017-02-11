// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#if PLATFORM_ANDROID
#include "tango_client_api.h"
#include "tango_support_api.h"
#include "tango_3d_reconstruction_api.h"
#endif
#include "TangoMeshReconstructionComponent.generated.h"

UCLASS(BlueprintType)
class UTangoMeshSection: public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite)
		int32 SectionIndex;
	UPROPERTY(BlueprintReadWrite)
		TArray<FVector> Vertices;
	UPROPERTY(BlueprintReadWrite)
		TArray<int32> Triangles;
	UPROPERTY(BlueprintReadWrite)
		TArray<FVector> Normals;
	UPROPERTY(BlueprintReadWrite)
		TArray<FLinearColor> VertexColors;
	void InitVertexAttrs();
#if PLATFORM_ANDROID
		TArray<FColor> TangoColors;
		FThreadSafeBool bOnGameThread;
		
#endif	
};

struct FSectionAddress
{
	int32 X;
	int32 Y;
	int32 Z;
	friend bool operator== (const FSectionAddress& A, const FSectionAddress& B)
	{
		return A.X == B.X && A.Y == B.Y && A.Z == B.Z;
	}

	friend uint32 GetTypeHash(const FSectionAddress& Other)
	{
		return FCrc::MemCrc_DEPRECATED(&Other, sizeof(Other));
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTangoMeshSectionUpdated, UTangoMeshSection*, MeshSection);

UCLASS( ClassGroup=(Tango),  meta=(BlueprintSpawnableComponent) )
class TANGOPLUGIN_API UTangoMeshReconstructionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTangoMeshReconstructionComponent();

	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void BeginPlay2();
	
	// Called every frame
	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;

	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	/** Base frame for vertices */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango|Mesh Reconstruction")
		ETangoCoordinateFrameType BaseFrame;
	/** Whether to generate per-vertex colors */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango|Mesh Reconstruction")
		bool bGenerateColor;
	/** Mesh resolution in cm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango|Mesh Reconstruction")
		int32 Resolution;
	/** Whether to remove empty space */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango|Mesh Reconstruction")
		bool bUseSpaceClearing;
	UPROPERTY(BlueprintAssignable)
		FOnTangoMeshSectionUpdated OnMeshSectionUpdated;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango|Mesh Reconstruction")
		bool bEnabled;

	void Run();
	void RunGen();

	UPROPERTY(Transient)
		TArray<UTangoMeshSection*> Sections;

#if PLATFORM_ANDROID

	void ReleaseResources();
	void ProcessImageBuffer(const TangoImageBuffer* buffer);
	FDelegateHandle ImageListener;
	FTransform ImageTransform;
	FTransform PointCloudTransform;
	
	Tango3DR_ConfigH t3dr_config_;
	Tango3DR_Context t3dr_context_;
	Tango3DR_CameraCalibration t3dr_intrinsics_;
	FCriticalSection ImageBufferMutex; // Protects above state related to processing image buffers
	// The point cloud of the most recent depth received.  Stored
	// as float tuples (X,Y,Z,C).
	void OnImageBufferAvailable(const TangoImageBuffer* ImageBuffer);

	TangoSupportImageBufferManager* ImageBufferManager;

	
	FCriticalSection UpdatedIndicesMutex; // protects UpdatedIndices
	TSet<FSectionAddress> UpdatedIndices;
	
	TMap<FSectionAddress, int32> SectionAddressMap;
	bool bPlaying;
	TArray<FSectionAddress> UpdatedIndicesLocal;
	FRunnableThread* Thread1;
	FRunnableThread* Thread2;
#endif
};
