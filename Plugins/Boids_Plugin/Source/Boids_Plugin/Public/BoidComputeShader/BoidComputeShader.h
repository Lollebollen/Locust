#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialRenderProxy.h"

#include "BoidComputeShader.generated.h"

struct BOIDS_PLUGIN_API FBoidComputeShaderDispatchParams
{
	int X, Y, Z;

	float DeltaTime;
	FVector3f Pointer;

	FBoidComputeShaderDispatchParams(int x, int y, int z)
	{
		X = x;
		Y = y;
		Z = z;
	}
};

class BOIDS_PLUGIN_API FBoidComputeShaderInterface
{
public:
	static void DispatchRenderThread(FRHICommandListImmediate& RHEICmdlist,
		FBoidComputeShaderDispatchParams Params,
		TFunction<void(TArray<FVector3f> OutputVal)> AsyncCallback); 

	static void DispatchGameThread(FBoidComputeShaderDispatchParams Params,
		TFunction<void(TArray<FVector3f> OutputVal)> AsyncCallback)
	{
		ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)(
			[Params, AsyncCallback](FRHICommandListImmediate& RHICmdList)
			{
				DispatchRenderThread(RHICmdList, Params, AsyncCallback);
			});
	}

	static void Dispatch(FBoidComputeShaderDispatchParams Params,
		TFunction<void(TArray<FVector3f> OutputVal)> AsyncCallback)
	{
		if (IsInRenderingThread())
		{
			DispatchRenderThread(GetImmediateCommandList_ForRenderCommand(), Params, AsyncCallback);
		}
		else
		{
			DispatchGameThread(Params, AsyncCallback);
		}
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBoidComputeShaderLibrary_AsyncExecutionCompleted, const TArray<FVector3f>&, Value);

UCLASS(Blueprintable)
class BOIDS_PLUGIN_API UBoidComputeShaderLibrary_AsyncExecution : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:

	virtual void Activate() override
	{
		FBoidComputeShaderDispatchParams Params(4, 4, 4);
		Params.DeltaTime = Arg1;
		Params.Pointer = Arg2;

		FBoidComputeShaderInterface::Dispatch(Params, [this](TArray<FVector3f> OutputVal) {
			this->Completed.Broadcast(OutputVal);
			});
	}

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", BlueprintPure = "false",
		Category = "ComputeShader", WorldContext = "WorldContextObject"))
	static UBoidComputeShaderLibrary_AsyncExecution* ExecuteBoidComputeShader(
		UObject* WorldContextObject, float Arg1, FVector3f Arg2)
	{
		UBoidComputeShaderLibrary_AsyncExecution* Action = NewObject<UBoidComputeShaderLibrary_AsyncExecution>();
		Action->Arg1 = Arg1;
		Action->Arg2 = Arg2;
		if (WorldContextObject)
		{
			Action->RegisterWithGameInstance(WorldContextObject);
		}

		return Action;
	}

	UPROPERTY(BlueprintAssignable)
	FOnBoidComputeShaderLibrary_AsyncExecutionCompleted Completed;

	float Arg1 = 0;
	FVector3f Arg2 = FVector3f(0,0,0);
};