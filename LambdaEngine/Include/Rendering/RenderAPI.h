#pragma once
#include "LambdaEngine.h"

#include "Core/TSharedRef.h"


namespace LambdaEngine
{
	constexpr const bool IMGUI_ENABLED = true;

	class Renderer;
	class CommandQueue;
	class GraphicsDevice;

	class LAMBDA_API RenderAPI
	{
	public:
		DECL_STATIC_CLASS(RenderAPI);

		static bool Init();
		static bool Release();

		static void Tick();

		FORCEINLINE static GraphicsDevice* GetDevice()
		{
			return s_pGraphicsDevice;
		}

		FORCEINLINE static CommandQueue* GetGraphicsQueue()
		{
			return s_GraphicsQueue.Get();
		}

		FORCEINLINE static CommandQueue* GetComputeQueue()
		{
			return s_ComputeQueue.Get();
		}

		FORCEINLINE static CommandQueue* GetCopyQueue()
		{
			return s_CopyQueue.Get();
		}

	private:
		static GraphicsDevice* s_pGraphicsDevice;
		static TSharedRef<CommandQueue>	s_GraphicsQueue;
		static TSharedRef<CommandQueue>	s_ComputeQueue;
		static TSharedRef<CommandQueue>	s_CopyQueue;
	};
}