#pragma once
#include "Rendering/Core/API/IPipelineState.h"
#include "Rendering/Core/API/DeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;
	class BufferVK;

	class RayTracingPipelineStateVK : public DeviceChildBase<GraphicsDeviceVK, IPipelineState>
	{
		using TDeviceChild = DeviceChildBase<GraphicsDeviceVK, IPipelineState>;

	public:
		RayTracingPipelineStateVK(const GraphicsDeviceVK* pDevice);
		~RayTracingPipelineStateVK();

		bool Init(const RayTracingPipelineDesc& desc);

        FORCEINLINE VkPipeline GetPipeline() const
        {
            return m_Pipeline;
        }
        
        //IDeviceChild interface
		virtual void SetName(const char* pName) override;

        //IPipelineState interface
		FORCEINLINE virtual EPipelineStateType GetType() const override
        {
            return EPipelineStateType::RAY_TRACING;
        }

		VkDeviceSize FORCEINLINE GetBindingOffsetRaygenGroup()	const { return m_BindingOffsetRaygenShaderGroup; }
		VkDeviceSize FORCEINLINE GetBindingOffsetHitGroup()		const { return m_BindingOffsetHitShaderGroup; }
		VkDeviceSize FORCEINLINE GetBindingOffsetMissGroup()	const { return m_BindingOffsetMissShaderGroup; }
		VkDeviceSize FORCEINLINE GetBindingStride()				const { return m_BindingStride; }

	private:
		bool CreateShaderData(std::vector<VkPipelineShaderStageCreateInfo>& shaderStagesInfos,
			std::vector<VkSpecializationInfo>& shaderStagesSpecializationInfos,
			std::vector<std::vector<VkSpecializationMapEntry>>& shaderStagesSpecializationMaps,
			std::vector<VkRayTracingShaderGroupCreateInfoKHR>& shaderGroups,
			const RayTracingPipelineDesc& desc);

	private:
		VkPipeline m_Pipeline;
		BufferVK* m_pSBT;

		VkDeviceSize m_BindingOffsetRaygenShaderGroup;
		VkDeviceSize m_BindingOffsetHitShaderGroup;
		VkDeviceSize m_BindingOffsetMissShaderGroup;
		VkDeviceSize m_BindingStride;
	};
}
