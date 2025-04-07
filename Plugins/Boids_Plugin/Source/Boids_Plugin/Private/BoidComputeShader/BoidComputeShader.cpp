#include "BoidComputeShader.h"
#include "Boids_Plugin/Public/BoidComputeShader/BoidComputeShader.h"
#include "PixelShaderUtils.h"
#include "MeshPassProcessor.h"
#include "StaticMeshResources.h"
#include "DynamicMeshBuilder.h"
#include "RenderGraphResources.h"
#include "GlobalShader.h"
#include "UnifiedBuffer.h"
#include "CanvasTypes.h"
#include "MeshDrawShaderBindings.h"
#include "RHIGPUReadback.h"
#include "MeshPassUtils.h"
#include "MaterialShader.h"

DECLARE_STATS_GROUP(TEXT("BoidComputeShader"), STATGROUP_BoidComputeShader, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("BoidComputeSHader Execute"), STAT_BoidComputeShader_Execute, STATGROUP_BoidComputeShader)

class BOIDS_PLUGIN_API FBoidComputeShader : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FBoidComputeShader);
	SHADER_USE_PARAMETER_STRUCT(FBoidComputeShader, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )

		SHADER_PARAMETER(float, deltaTime)
		SHADER_PARAMETER(FVector3f, pointer)

		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<FVector2f>, positions)

	END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		const FPermutationDomain PermutationVector(Parameters.PermutationId);
		return true;
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters,
		FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		const FPermutationDomain PermutationVector(Parameters.PermutationId);

		OutEnvironment.SetDefine(TEXT("THREADS_X"), NUM_THREADS_BoidComputeShader_X);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), NUM_THREADS_BoidComputeShader_Y);
		OutEnvironment.SetDefine(TEXT("THREADS_Z"), NUM_THREADS_BoidComputeShader_Z);
	}
private:
};


IMPLEMENT_GLOBAL_SHADER(FBoidComputeShader, "/Boids_PluginShaders/BoidComputeShader/BoidComputeShader.usf",
	"BoidComputeShader", SF_Compute);

void FBoidComputeShaderInterface::DispatchRenderThread(FRHICommandListImmediate& RHICmdList,
	FBoidComputeShaderDispatchParams Params, TFunction<void(TArray<FVector3f> OutputVal)> AsyncCallback)
{
	FRDGBuilder GraphBuilder(RHICmdList);
	{
		SCOPE_CYCLE_COUNTER(STAT_BoidComputeShader_Execute);
		DECLARE_GPU_STAT(BoidComputeShader)
		RDG_EVENT_SCOPE(GraphBuilder, "ExampleComputeShader");
		RDG_GPU_STAT_SCOPE(GraphBuilder, BoidComputeShader);

		typename FBoidComputeShader::FPermutationDomain PermutationVector;

		TShaderMapRef<FBoidComputeShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);


		if (ComputeShader.IsValid())
		{
			UE_LOG(LogTemp, Log, TEXT("ComputeShader is valid"));
			FBoidComputeShader::FParameters* PassParameters = GraphBuilder.AllocParameters<FBoidComputeShader::FParameters>();
			UE_LOG(LogTemp, Log, TEXT("PassParameters Defined"));

			FRDGBufferRef OutputBuffer = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateBufferDesc(
				sizeof(float) * 3, 64), TEXT("positions")); // TODO Dynamic Buffersize
			UE_LOG(LogTemp, Log, TEXT("OutPut Buffer Defined"));

			PassParameters->positions = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(OutputBuffer, PF_R32_SINT));

			UE_LOG(LogTemp, Log, TEXT("PassParameters positions Defined"));

			FInt32Vector3 GroupCount = FComputeShaderUtils::GetGroupCount(FInt32Vector3(
				Params.X, Params.Y, Params.Z), FInt32Vector3(4, 4, 4));
			UE_LOG(LogTemp, Log, TEXT("Group Count Defined"));

			GraphBuilder.AddPass(RDG_EVENT_NAME("ExecuteBoidComputeShader"), //TODO PROBLEM AREA
				PassParameters,
				ERDGPassFlags::AsyncCompute,
				[&PassParameters, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList)
				{
					FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParameters, GroupCount);
				});
			UE_LOG(LogTemp, Log, TEXT("Pass Added"));

			FRHIGPUBufferReadback* GPUBufferReadback = new FRHIGPUBufferReadback(TEXT("ExecuteBoidComputeShaderPositions")); // TODO PROBLEM AREA
			UE_LOG(LogTemp, Log, TEXT("Readback buffer init"));

			AddEnqueueCopyPass(GraphBuilder, GPUBufferReadback, OutputBuffer, 0u);
			UE_LOG(LogTemp, Log, TEXT("Enqueue Added, past max"));
			GraphBuilder.Execute();

			auto RunnerFunc = [GPUBufferReadback, AsyncCallback](auto&& RunnerFunc) -> void
				{
					UE_LOG(LogTemp, Log, TEXT("Pre Readback"));
					if (GPUBufferReadback->IsReady())
					{
						UE_LOG(LogTemp, Log, TEXT("Readback ready"));

						TArray<FVector3f>* Buffer = (TArray<FVector3f>*)GPUBufferReadback->Lock(sizeof(float) * 3 * 64); // TODO PROBLEM AREA
						TArray<FVector3f> OutValue = *Buffer;
						GPUBufferReadback->Unlock();

						AsyncTask(ENamedThreads::GameThread, [AsyncCallback, OutValue]()
							{
								AsyncCallback(OutValue);
							});
					}
					else
					{
						UE_LOG(LogTemp, Log, TEXT("Readback not ready"));

						AsyncTask(ENamedThreads::ActualRenderingThread, [RunnerFunc]()
							{
								RunnerFunc(RunnerFunc);
							});
					}

				};
			UE_LOG(LogTemp, Log, TEXT("End of if check"));

			AsyncTask(ENamedThreads::ActualRenderingThread, [RunnerFunc]()
				{
					RunnerFunc(RunnerFunc);
				});
		}
		else
		{
#if WITH_EDITOR
			GEngine->AddOnScreenDebugMessage((uint64)42145125184, 6.f, FColor::Red, FString(TEXT("The Compute shader has a problem.")));
#endif
		}
	}

}