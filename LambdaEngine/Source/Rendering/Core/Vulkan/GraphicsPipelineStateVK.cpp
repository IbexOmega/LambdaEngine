#include "Rendering/Core/Vulkan/GraphicsPipelineStateVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	GraphicsPipelineStateVK::GraphicsPipelineStateVK(const GraphicsDeviceVK* pDevice) : 
		TDeviceChild(pDevice),
		m_Pipeline(VK_NULL_HANDLE)
	{
		//Default InputAssembly
		m_InputAssembly.sType                   = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		m_InputAssembly.flags                   = 0;
		m_InputAssembly.pNext                   = nullptr;
		m_InputAssembly.topology                = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		m_InputAssembly.primitiveRestartEnable  = VK_FALSE;

		//Default RasterizerState
		m_RasterizerState.sType                     = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		m_RasterizerState.flags                     = 0;
		m_RasterizerState.pNext                     = nullptr;
		m_RasterizerState.polygonMode               = VK_POLYGON_MODE_FILL;
		m_RasterizerState.lineWidth                 = 1.0f;
		m_RasterizerState.cullMode                  = VK_CULL_MODE_NONE;
		m_RasterizerState.frontFace                 = VK_FRONT_FACE_CLOCKWISE;
		m_RasterizerState.depthBiasEnable           = VK_FALSE;
		m_RasterizerState.depthClampEnable          = VK_FALSE;
		m_RasterizerState.rasterizerDiscardEnable   = VK_FALSE;
		m_RasterizerState.depthBiasClamp            = 0.0f;
		m_RasterizerState.depthBiasConstantFactor   = 0.0f;
		m_RasterizerState.depthBiasSlopeFactor      = 0.0f;

		//MultisamplingState
		m_MultisamplingState.sType                  = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		m_MultisamplingState.flags                  = 0;
		m_MultisamplingState.pNext                  = nullptr;
		m_MultisamplingState.alphaToCoverageEnable  = VK_FALSE;
		m_MultisamplingState.alphaToOneEnable       = VK_FALSE;
		m_MultisamplingState.minSampleShading       = 0.0f;
		m_MultisamplingState.sampleShadingEnable    = VK_FALSE;
		m_MultisamplingState.rasterizationSamples   = VK_SAMPLE_COUNT_1_BIT;
		m_MultisamplingState.pSampleMask            = nullptr;

		//BlendState
		m_BlendState.sType              = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		m_BlendState.flags              = 0;
		m_BlendState.pNext              = nullptr;
		m_BlendState.logicOpEnable      = VK_FALSE;
		m_BlendState.logicOp            = VK_LOGIC_OP_COPY;
		m_BlendState.pAttachments       = nullptr;
		m_BlendState.attachmentCount    = 0;
		m_BlendState.blendConstants[0]  = 0.0f;
		m_BlendState.blendConstants[1]  = 0.0f;
		m_BlendState.blendConstants[2]  = 0.0f;
		m_BlendState.blendConstants[3]  = 0.0f;

		//DepthstencilState
		m_DepthStencilState.sType                   = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		m_DepthStencilState.flags                   = 0;
		m_DepthStencilState.pNext                   = nullptr;
		m_DepthStencilState.depthTestEnable         = VK_FALSE;
		m_DepthStencilState.depthWriteEnable        = VK_TRUE;
		m_DepthStencilState.depthCompareOp          = VK_COMPARE_OP_LESS_OR_EQUAL;
		m_DepthStencilState.depthBoundsTestEnable   = VK_FALSE;
		m_DepthStencilState.stencilTestEnable       = VK_FALSE;
		m_DepthStencilState.minDepthBounds          = 0.0f;
		m_DepthStencilState.maxDepthBounds          = 1.0f; 
		m_DepthStencilState.front                   = {};
		m_DepthStencilState.back                    = {};
	}

	GraphicsPipelineStateVK::~GraphicsPipelineStateVK()
	{
		if (m_Pipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(m_pDevice->Device, m_Pipeline, nullptr);
			m_Pipeline = VK_NULL_HANDLE;
		}
	}

	bool GraphicsPipelineStateVK::Init(const GraphicsPipelineDesc& desc)
	{
		 // Define shader stage create infos
		std::vector<VkPipelineShaderStageCreateInfo> shaderStagesInfos;
		std::vector<VkSpecializationInfo> shaderStagesSpecializationInfos;
		std::vector<std::vector<VkSpecializationMapEntry>> shaderStagesSpecializationMaps;

		if (!CreateShaderData(shaderStagesInfos, shaderStagesSpecializationInfos, shaderStagesSpecializationMaps, desc))
			return false;

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.flags                           = 0;
		vertexInputInfo.pNext                           = nullptr;
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions    = nullptr;
		vertexInputInfo.vertexBindingDescriptionCount   = 0;
		vertexInputInfo.pVertexBindingDescriptions      = nullptr;

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.flags         = 0;
		viewportState.pNext         = nullptr;
		viewportState.viewportCount = 1;
		viewportState.pViewports    = nullptr;
		viewportState.scissorCount  = 1;
		viewportState.pScissors     = nullptr;

		VkDynamicState dynamicStates[] =
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType              = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.flags              = 0;
		dynamicState.pNext              = nullptr;
		dynamicState.pDynamicStates     = dynamicStates;
		dynamicState.dynamicStateCount  = 2;

		VkPipelineColorBlendAttachmentState blendAttachment = {};
		blendAttachment.blendEnable = VK_FALSE;
		blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		m_BlendState.pAttachments       = &blendAttachment;
		m_BlendState.attachmentCount    = 1;

		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType                  = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.pNext                  = nullptr;
		pipelineInfo.flags                  = 0;
		pipelineInfo.stageCount             = uint32_t(shaderStagesInfos.size());
		pipelineInfo.pStages                = shaderStagesInfos.data();
		pipelineInfo.pVertexInputState      = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState    = &m_InputAssembly;
		pipelineInfo.pViewportState         = &viewportState;
		pipelineInfo.pRasterizationState    = &m_RasterizerState;
		pipelineInfo.pMultisampleState      = &m_MultisamplingState;
		pipelineInfo.pDepthStencilState     = &m_DepthStencilState;
		pipelineInfo.pColorBlendState       = &m_BlendState;
		pipelineInfo.pDynamicState          = &dynamicState;
		pipelineInfo.renderPass             = VK_NULL_HANDLE;
		pipelineInfo.layout                 = VK_NULL_HANDLE;
		pipelineInfo.subpass                = 0;
		pipelineInfo.basePipelineHandle     = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex      = -1;

		if (vkCreateGraphicsPipelines(m_pDevice->Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS)
		{
			LOG_ERROR("--- GraphicsPipelineStateVK: vkCreateGraphicsPipelines failed for %s", desc.pName);
			return false;
		}

		SetName(desc.pName);

		return true;
	}

	void GraphicsPipelineStateVK::SetName(const char* pName)
	{
		m_pDevice->SetVulkanObjectName(pName, (uint64)m_Pipeline, VK_OBJECT_TYPE_PIPELINE);
	}

	bool GraphicsPipelineStateVK::CreateShaderData(
		std::vector<VkPipelineShaderStageCreateInfo>& shaderStagesInfos, 
		std::vector<VkSpecializationInfo>& shaderStagesSpecializationInfos, 
		std::vector<std::vector<VkSpecializationMapEntry>>& shaderStagesSpecializationMaps, 
		const GraphicsPipelineDesc& desc)
	{
		if (desc.MeshShader.pSource != nullptr)
		{
			//Mesh Shader
			{
				const ShaderDesc& shaderDesc = desc.MeshShader;

				VkPipelineShaderStageCreateInfo shaderCreateInfo;
				VkSpecializationInfo shaderSpecializationInfo;
				std::vector<VkSpecializationMapEntry> shaderSpecializationMaps;

				CreateSpecializationInfo(shaderSpecializationInfo, shaderSpecializationMaps, shaderDesc);

				if (!CreateShaderStageInfo(m_pDevice->Device, shaderCreateInfo, shaderDesc, shaderSpecializationInfo))
					return false;

				shaderStagesInfos.push_back(shaderCreateInfo);
				shaderStagesSpecializationInfos.push_back(shaderSpecializationInfo);
				shaderStagesSpecializationMaps.push_back(shaderSpecializationMaps);
			}
		}
		else
		{
			//Vertex Shader
			{
				const ShaderDesc& shaderDesc = desc.VertexShader;

				VkPipelineShaderStageCreateInfo shaderCreateInfo;
				VkSpecializationInfo shaderSpecializationInfo;
				std::vector<VkSpecializationMapEntry> shaderSpecializationMaps;

				CreateSpecializationInfo(shaderSpecializationInfo, shaderSpecializationMaps, shaderDesc);

				if (!CreateShaderStageInfo(m_pDevice->Device, shaderCreateInfo, shaderDesc, shaderSpecializationInfo))
					return false;

				shaderStagesInfos.push_back(shaderCreateInfo);
				shaderStagesSpecializationInfos.push_back(shaderSpecializationInfo);
				shaderStagesSpecializationMaps.push_back(shaderSpecializationMaps);
			}

			//Geometry Shader
			if (desc.GeometryShader.pSource != nullptr)
			{
				const ShaderDesc& shaderDesc = desc.GeometryShader;

				VkPipelineShaderStageCreateInfo shaderCreateInfo;
				VkSpecializationInfo shaderSpecializationInfo;
				std::vector<VkSpecializationMapEntry> shaderSpecializationMaps;

				CreateSpecializationInfo(shaderSpecializationInfo, shaderSpecializationMaps, shaderDesc);

				if (!CreateShaderStageInfo(m_pDevice->Device, shaderCreateInfo, shaderDesc, shaderSpecializationInfo))
					return false;

				shaderStagesInfos.push_back(shaderCreateInfo);
				shaderStagesSpecializationInfos.push_back(shaderSpecializationInfo);
				shaderStagesSpecializationMaps.push_back(shaderSpecializationMaps);
			}

			//Hull Shader
			if (desc.HullShader.pSource != nullptr)
			{
				const ShaderDesc& shaderDesc = desc.HullShader;

				VkPipelineShaderStageCreateInfo shaderCreateInfo;
				VkSpecializationInfo shaderSpecializationInfo;
				std::vector<VkSpecializationMapEntry> shaderSpecializationMaps;

				CreateSpecializationInfo(shaderSpecializationInfo, shaderSpecializationMaps, shaderDesc);

				if (!CreateShaderStageInfo(m_pDevice->Device, shaderCreateInfo, shaderDesc, shaderSpecializationInfo))
					return false;

				shaderStagesInfos.push_back(shaderCreateInfo);
				shaderStagesSpecializationInfos.push_back(shaderSpecializationInfo);
				shaderStagesSpecializationMaps.push_back(shaderSpecializationMaps);
			}

			//Domain Shader
			if (desc.DomainShader.pSource != nullptr)
			{
				const ShaderDesc& shaderDesc = desc.DomainShader;

				VkPipelineShaderStageCreateInfo shaderCreateInfo;
				VkSpecializationInfo shaderSpecializationInfo;
				std::vector<VkSpecializationMapEntry> shaderSpecializationMaps;

				CreateSpecializationInfo(shaderSpecializationInfo, shaderSpecializationMaps, shaderDesc);

				if (!CreateShaderStageInfo(m_pDevice->Device, shaderCreateInfo, shaderDesc, shaderSpecializationInfo))
					return false;

				shaderStagesInfos.push_back(shaderCreateInfo);
				shaderStagesSpecializationInfos.push_back(shaderSpecializationInfo);
				shaderStagesSpecializationMaps.push_back(shaderSpecializationMaps);
			}
		}

		//Pixel Shader
		if (desc.PixelShader.pSource != nullptr)
		{
			const ShaderDesc& shaderDesc = desc.PixelShader;

			VkPipelineShaderStageCreateInfo shaderCreateInfo;
			VkSpecializationInfo shaderSpecializationInfo;
			std::vector<VkSpecializationMapEntry> shaderSpecializationMaps;

			CreateSpecializationInfo(shaderSpecializationInfo, shaderSpecializationMaps, shaderDesc);

			if (!CreateShaderStageInfo(m_pDevice->Device, shaderCreateInfo, shaderDesc, shaderSpecializationInfo))
				return false;

			shaderStagesInfos.push_back(shaderCreateInfo);
			shaderStagesSpecializationInfos.push_back(shaderSpecializationInfo);
			shaderStagesSpecializationMaps.push_back(shaderSpecializationMaps);
		}

		return true;
	}
}