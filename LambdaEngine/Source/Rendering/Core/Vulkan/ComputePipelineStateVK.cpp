#include "Rendering/Core/Vulkan/ComputePipelineStateVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/PipelineLayoutVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"
#include "Rendering/Core/Vulkan/ShaderVK.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	ComputePipelineStateVK::ComputePipelineStateVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice)
		, m_Pipeline(VK_NULL_HANDLE)
	{
	}

	ComputePipelineStateVK::~ComputePipelineStateVK()
	{
		if (m_Pipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(m_pDevice->Device, m_Pipeline, nullptr);
			m_Pipeline = VK_NULL_HANDLE;
		}
	}

	bool ComputePipelineStateVK::Init(const ComputePipelineStateDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		const ShaderVK*			pShader				= reinterpret_cast<const ShaderVK*>(pDesc->Shader.pShader);
		const PipelineLayoutVK* pPipelineLayoutVk	= reinterpret_cast<const PipelineLayoutVK*>(pDesc->pPipelineLayout);
		VALIDATE(pShader			!= nullptr);
		VALIDATE(pPipelineLayoutVk	!= nullptr);

		// ShaderStageInfo
		VkPipelineShaderStageCreateInfo shaderCreateInfo = { };
		shaderCreateInfo.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderCreateInfo.pNext	= nullptr;
		shaderCreateInfo.flags	= 0;
		shaderCreateInfo.stage	= VK_SHADER_STAGE_COMPUTE_BIT;
		shaderCreateInfo.module	= pShader->GetShaderModule();
		shaderCreateInfo.pName	= pShader->GetEntryPoint().c_str();

		// Shader Constants
		VkSpecializationInfo specializationInfo = { };
		TArray<VkSpecializationMapEntry> specializationEntries;
		if (!pDesc->Shader.ShaderConstants.IsEmpty())
		{
			const uint32 constantCount = static_cast<uint32>(pDesc->Shader.ShaderConstants.GetSize());

			specializationEntries.Resize(constantCount);
			for (uint32 i = 0; i < constantCount; i++)
			{
				VkSpecializationMapEntry* pSpecializationEntry = &specializationEntries[i];
				pSpecializationEntry->constantID	= i;
				pSpecializationEntry->offset		= i * sizeof(ShaderConstant);
				pSpecializationEntry->size			= sizeof(ShaderConstant);
			}

			specializationInfo.mapEntryCount = static_cast<uint32>(specializationEntries.GetSize());
			specializationInfo.pMapEntries	 = specializationEntries.GetData();
			specializationInfo.dataSize		 = constantCount * sizeof(ShaderConstant);
			specializationInfo.pData		 = pDesc->Shader.ShaderConstants.GetData();

			shaderCreateInfo.pSpecializationInfo = &specializationInfo;
		}
		else
		{
			shaderCreateInfo.pSpecializationInfo = nullptr;
		}

		VkComputePipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType					= VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.pNext					= nullptr;
		pipelineInfo.flags					= 0;
		pipelineInfo.layout					= pPipelineLayoutVk->GetPipelineLayout();
		pipelineInfo.basePipelineHandle		= VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex		= -1;
		pipelineInfo.stage					= shaderCreateInfo;

		VkResult result = vkCreateComputePipelines(m_pDevice->Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline);
		if (result != VK_SUCCESS)
		{
			if (!pDesc->DebugName.empty())
			{
				LOG_VULKAN_ERROR(result, "vkCreateComputePipelines failed for %s", pDesc->DebugName.c_str());
			}
			else
			{
				LOG_VULKAN_ERROR(result, "vkCreateComputePipelines failed");
			}

			return false;
		}
		else
		{
			SetName(pDesc->DebugName);
			if (!pDesc->DebugName.empty())
			{
				LOG_VULKAN_INFO("Created Pipeline for %s", pDesc->DebugName.c_str());
			}
			else
			{
				LOG_VULKAN_INFO("Created Pipeline");
			}

			return true;
		}
	}

	void ComputePipelineStateVK::SetName(const String& debugName)
	{
		m_pDevice->SetVulkanObjectName(debugName, reinterpret_cast<uint64>(m_Pipeline), VK_OBJECT_TYPE_PIPELINE);
		m_DebugName = debugName;
	}
}
