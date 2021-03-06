#include "Log/Log.h"

#include "Rendering/Core/Vulkan/RayTracingPipelineStateVK.h"
#include "Rendering/Core/Vulkan/CommandAllocatorVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/PipelineLayoutVK.h"
#include "Rendering/Core/Vulkan/CommandListVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"
#include "Rendering/Core/Vulkan/BufferVK.h"
#include "Rendering/Core/Vulkan/ShaderVK.h"

#include "Math/MathUtilities.h"

namespace LambdaEngine
{
	RayTracingPipelineStateVK::RayTracingPipelineStateVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice)
	{
	}

	RayTracingPipelineStateVK::~RayTracingPipelineStateVK()
	{
		if (m_Pipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(m_pDevice->Device, m_Pipeline, nullptr);
			m_Pipeline = VK_NULL_HANDLE;
		}
	}

	bool RayTracingPipelineStateVK::Init(const RayTracingPipelineStateDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		if (pDesc->RaygenShader.pShader == nullptr)
		{
			LOG_ERROR("pRaygenShader cannot be nullptr!");
			return false;
		}

		if (pDesc->HitGroupShaders.IsEmpty())
		{
			LOG_ERROR("ClosestHitShaderCount cannot be zero!");
			return false;
		}

		if (pDesc->MissShaders.IsEmpty())
		{
			LOG_ERROR("MissShaderCount cannot be zero!");
			return false;
		}

		// Define shader stage create infos
		TArray<VkPipelineShaderStageCreateInfo>			shaderStagesInfos;
		TArray<VkSpecializationInfo>					shaderStagesSpecializationInfos;
		TArray<TArray<VkSpecializationMapEntry>>		shaderStagesSpecializationMaps;
		TArray<VkRayTracingShaderGroupCreateInfoKHR>	shaderGroups;

		// Raygen Shader
		{
			CreateShaderStageInfo(&pDesc->RaygenShader, shaderStagesInfos, shaderStagesSpecializationInfos, shaderStagesSpecializationMaps);

			VkRayTracingShaderGroupCreateInfoKHR shaderGroupCreateInfo = {};
			shaderGroupCreateInfo.sType					= VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			shaderGroupCreateInfo.type					= VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			shaderGroupCreateInfo.generalShader			= 0;
			shaderGroupCreateInfo.intersectionShader	= VK_SHADER_UNUSED_NV;
			shaderGroupCreateInfo.anyHitShader			= VK_SHADER_UNUSED_NV;
			shaderGroupCreateInfo.closestHitShader		= VK_SHADER_UNUSED_NV;
			shaderGroups.EmplaceBack(shaderGroupCreateInfo);
		}

		// Closest-Hit Shaders
		for (const HitGroupShaderModuleDescs& HitGroupShaders : pDesc->HitGroupShaders)
		{
			VkRayTracingShaderGroupCreateInfoKHR shaderGroupCreateInfo = {};
			shaderGroupCreateInfo.sType					= VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			shaderGroupCreateInfo.type					= VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
			shaderGroupCreateInfo.generalShader			= VK_SHADER_UNUSED_NV;

			CreateShaderStageInfo(&HitGroupShaders.ClosestHitShaders, shaderStagesInfos, shaderStagesSpecializationInfos, shaderStagesSpecializationMaps);
			shaderGroupCreateInfo.closestHitShader		= static_cast<uint32>(shaderStagesInfos.GetSize() - 1);

			if (HitGroupShaders.AnyHitShaders.pShader != nullptr)
			{
				CreateShaderStageInfo(&HitGroupShaders.AnyHitShaders, shaderStagesInfos, shaderStagesSpecializationInfos, shaderStagesSpecializationMaps);
				shaderGroupCreateInfo.anyHitShader	= static_cast<uint32>(shaderStagesInfos.GetSize() - 1);
			}
			else
			{
				shaderGroupCreateInfo.anyHitShader	= VK_SHADER_UNUSED_NV;
			}

			if (HitGroupShaders.IntersectionShaders.pShader != nullptr)
			{
				CreateShaderStageInfo(&HitGroupShaders.IntersectionShaders, shaderStagesInfos, shaderStagesSpecializationInfos, shaderStagesSpecializationMaps);
				shaderGroupCreateInfo.intersectionShader = static_cast<uint32>(shaderStagesInfos.GetSize() - 1);
			}
			else
			{
				shaderGroupCreateInfo.intersectionShader = VK_SHADER_UNUSED_NV;
			}

			shaderGroups.EmplaceBack(shaderGroupCreateInfo);
		}
		m_HitShaderCount = pDesc->HitGroupShaders.GetSize();

		// Miss Shaders
		for (const ShaderModuleDesc& missShader : pDesc->MissShaders)
		{
			CreateShaderStageInfo(&missShader, shaderStagesInfos, shaderStagesSpecializationInfos, shaderStagesSpecializationMaps);

			VkRayTracingShaderGroupCreateInfoKHR shaderGroupCreateInfo = {};
			shaderGroupCreateInfo.sType					= VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			shaderGroupCreateInfo.type					= VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			shaderGroupCreateInfo.generalShader			= static_cast<uint32>(shaderStagesInfos.GetSize() - 1);
			shaderGroupCreateInfo.intersectionShader	= VK_SHADER_UNUSED_NV;
			shaderGroupCreateInfo.anyHitShader			= VK_SHADER_UNUSED_NV;
			shaderGroupCreateInfo.closestHitShader		= VK_SHADER_UNUSED_NV;
			shaderGroups.EmplaceBack(shaderGroupCreateInfo);

			m_MissShaderCount = pDesc->MissShaders.GetSize();
		}

		VkPipelineLibraryCreateInfoKHR rayTracingPipelineLibrariesInfo = {};
		rayTracingPipelineLibrariesInfo.sType		 = VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR;
		rayTracingPipelineLibrariesInfo.libraryCount = 0;
		rayTracingPipelineLibrariesInfo.pLibraries	 = nullptr;

		const PipelineLayoutVK* pPipelineLayoutVk = reinterpret_cast<const PipelineLayoutVK*>(pDesc->pPipelineLayout);

		VkRayTracingPipelineCreateInfoKHR rayTracingPipelineInfo = {};
		rayTracingPipelineInfo.sType			 = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		rayTracingPipelineInfo.flags			 = VK_PIPELINE_CREATE_RAY_TRACING_NO_NULL_CLOSEST_HIT_SHADERS_BIT_KHR | VK_PIPELINE_CREATE_RAY_TRACING_NO_NULL_MISS_SHADERS_BIT_KHR;
		rayTracingPipelineInfo.stageCount		 = static_cast<uint32>(shaderStagesInfos.GetSize());
		rayTracingPipelineInfo.pStages			 = shaderStagesInfos.GetData();
		rayTracingPipelineInfo.groupCount		 = static_cast<uint32>(shaderGroups.GetSize());
		rayTracingPipelineInfo.pGroups			 = shaderGroups.GetData();
		rayTracingPipelineInfo.maxRecursionDepth = pDesc->MaxRecursionDepth;
		rayTracingPipelineInfo.layout			 = pPipelineLayoutVk->GetPipelineLayout();
		rayTracingPipelineInfo.libraries		 = rayTracingPipelineLibrariesInfo;

		VkResult result = m_pDevice->vkCreateRayTracingPipelinesKHR(m_pDevice->Device, VK_NULL_HANDLE, 1, &rayTracingPipelineInfo, nullptr, &m_Pipeline);
		if (result != VK_SUCCESS)
		{
			if (!pDesc->DebugName.empty())
			{
				LOG_VULKAN_ERROR(result, "vkCreateRayTracingPipelinesKHR failed for \"%s\"", pDesc->DebugName.c_str());
			}
			else
			{
				LOG_VULKAN_ERROR(result, "vkCreateRayTracingPipelinesKHR failed");
			}

			return false;
		}

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

	void RayTracingPipelineStateVK::SetName(const String& debugName)
	{
		m_pDevice->SetVulkanObjectName(debugName, reinterpret_cast<uint64>(m_Pipeline), VK_OBJECT_TYPE_PIPELINE);
		m_DebugName = debugName;
	}

	void RayTracingPipelineStateVK::CreateShaderStageInfo(const ShaderModuleDesc* pShaderModule, TArray<VkPipelineShaderStageCreateInfo>& shaderStagesInfos,
		TArray<VkSpecializationInfo>& shaderStagesSpecializationInfos, TArray<TArray<VkSpecializationMapEntry>>& shaderStagesSpecializationMaps)
	{
		const ShaderVK* pShader = reinterpret_cast<const ShaderVK*>(pShaderModule->pShader);

		// ShaderStageInfo
		VkPipelineShaderStageCreateInfo shaderCreateInfo = { };
		shaderCreateInfo.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderCreateInfo.pNext	= nullptr;
		shaderCreateInfo.flags	= 0;
		shaderCreateInfo.stage	= ConvertShaderStageFlag(pShader->GetDesc().Stage);
		shaderCreateInfo.module	= pShader->GetShaderModule();
		shaderCreateInfo.pName	= pShader->GetEntryPoint().c_str();

		// Shader Constants
		if (!pShaderModule->ShaderConstants.IsEmpty())
		{
			TArray<VkSpecializationMapEntry> specializationEntries(pShaderModule->ShaderConstants.GetSize());
			for (uint32 i = 0; i < pShaderModule->ShaderConstants.GetSize(); i++)
			{
				VkSpecializationMapEntry* pSpecializationEntry = &specializationEntries[i];
				pSpecializationEntry->constantID	= i;
				pSpecializationEntry->offset		= i * sizeof(ShaderConstant);
				pSpecializationEntry->size			= sizeof(ShaderConstant);
			}

			TArray<VkSpecializationMapEntry>& emplacedSpecializationEntries = shaderStagesSpecializationMaps.EmplaceBack(specializationEntries);

			VkSpecializationInfo specializationInfo = { };
			specializationInfo.mapEntryCount = static_cast<uint32>(emplacedSpecializationEntries.GetSize());
			specializationInfo.pMapEntries	= emplacedSpecializationEntries.GetData();
			specializationInfo.dataSize		= static_cast<uint32>(pShaderModule->ShaderConstants.GetSize()) * sizeof(ShaderConstant);
			specializationInfo.pData		= pShaderModule->ShaderConstants.GetData();
			shaderStagesSpecializationInfos.EmplaceBack(specializationInfo);

			shaderCreateInfo.pSpecializationInfo = &shaderStagesSpecializationInfos.GetBack();
		}
		else
		{
			shaderCreateInfo.pSpecializationInfo = nullptr;
		}

		shaderStagesInfos.EmplaceBack(shaderCreateInfo);
	}
}
