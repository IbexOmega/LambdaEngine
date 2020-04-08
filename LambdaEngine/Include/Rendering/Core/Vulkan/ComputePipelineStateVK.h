#pragma once
#include "Rendering/Core/API/IPipelineState.h"
#include "Rendering/Core/API/DeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	class ComputePipelineStateVK : public DeviceChildBase<GraphicsDeviceVK, IPipelineState>
	{
		using TDeviceChild = DeviceChildBase<GraphicsDeviceVK, IPipelineState>;

	public:
		ComputePipelineStateVK(const GraphicsDeviceVK* pDevice);
		~ComputePipelineStateVK();

		bool Init(const ComputePipelineDesc& desc);

        FORCEINLINE VkPipeline GetPipeline() const 
        {
            return m_Pipeline;
        }
        
        // IDeviceChild interface
		virtual void SetName(const char* pName) override final;

        // IPipelineState interface
		FORCEINLINE virtual EPipelineStateType GetType() const override final
        {
            return EPipelineStateType::COMPUTE;
        }

	private:
		VkPipeline m_Pipeline;
	};
}
