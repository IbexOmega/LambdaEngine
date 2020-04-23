#include "Log/Log.h"

#include "Containers/TArray.h"

#include "Rendering/Core/Vulkan/DescriptorSetVK.h"
#include "Rendering/Core/Vulkan/DescriptorHeapVK.h"
#include "Rendering/Core/Vulkan/PipelineLayoutVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/TextureViewVK.h"
#include "Rendering/Core/Vulkan/AccelerationStructureVK.h"
#include "Rendering/Core/Vulkan/SamplerVK.h"
#include "Rendering/Core/Vulkan/BufferVK.h"
#include "Rendering/Core/Vulkan/PipelineLayoutVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

namespace LambdaEngine
{
	DescriptorSetVK::DescriptorSetVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice),
		m_Bindings()
	{
	}

	DescriptorSetVK::~DescriptorSetVK()
	{
		if (m_pDescriptorHeap)
		{
			m_pDescriptorHeap->FreeDescriptorSet(m_DescriptorSet);
			RELEASE(m_pDescriptorHeap);
		}

		m_pDescriptorHeap = VK_NULL_HANDLE;
	}

	bool DescriptorSetVK::Init(const char* pName, const IPipelineLayout* pPipelineLayout, uint32 descriptorLayoutIndex, IDescriptorHeap* pDescriptorHeap)
	{
		DescriptorHeapVK* pVkDescriptorHeap = reinterpret_cast<DescriptorHeapVK*>(pDescriptorHeap);
		m_DescriptorSet = pVkDescriptorHeap->AllocateDescriptorSet(pPipelineLayout, descriptorLayoutIndex);
		if (m_DescriptorSet == VK_NULL_HANDLE)
		{
			return false;
		}
		else
		{
			SetName(pName);

			const PipelineLayoutVK*		pPipelineLayoutVk	= reinterpret_cast<const PipelineLayoutVK*>(pPipelineLayout);
			DescriptorSetBindingsDesc	bindings			= pPipelineLayoutVk->GetDescriptorBindings(descriptorLayoutIndex);
			
			m_BindingCount = bindings.BindingCount;
			memcpy(m_Bindings, bindings.Bindings, sizeof(DescriptorBindingDesc) * m_BindingCount);

			pVkDescriptorHeap->AddRef();
			m_pDescriptorHeap = pVkDescriptorHeap;
			return true;
		}
	}

	void DescriptorSetVK::SetName(const char* pName)
	{
		if (pName)
		{
			TDeviceChild::SetName(pName);
			m_pDevice->SetVulkanObjectName(pName, (uint64)m_DescriptorSet, VK_OBJECT_TYPE_DESCRIPTOR_SET);
		}
	}

	void DescriptorSetVK::WriteTextureDescriptors(const ITextureView* const* ppTextures, const ISampler* const* ppSamplers, ETextureState textureState, uint32 firstBinding, uint32 descriptorCount, EDescriptorType descriptorType)
	{
		VALIDATE(ppTextures != nullptr);

		const TextureViewVK* const* ppVkTextureViews	= reinterpret_cast<const TextureViewVK* const*>(ppTextures);
		const SamplerVK* const*		ppVkSamplers		= reinterpret_cast<const SamplerVK* const*>(ppSamplers);

		VkDescriptorType descriptorTypeVk = ConvertDescriptorType(descriptorType);
		
		VkImageLayout imageLayout = ConvertTextureState(textureState);

		TArray<VkDescriptorImageInfo> imageInfos(descriptorCount);
		for (uint32_t i = 0; i < descriptorCount; i++)
		{
			VkDescriptorImageInfo& imageInfo = imageInfos[i];
			imageInfo.imageLayout = imageLayout;

			VALIDATE(ppVkTextureViews[i] != nullptr);
			imageInfo.imageView	= ppVkTextureViews[i]->GetImageView();
			
			if (descriptorTypeVk == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			{
				VALIDATE(ppVkSamplers		!= nullptr);
				VALIDATE(ppVkSamplers[i]	!= nullptr);
				imageInfo.sampler = ppVkSamplers[i]->GetSampler();
			}
		}

		VkWriteDescriptorSet descriptorImageWrite = {};
		descriptorImageWrite.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorImageWrite.dstSet				= m_DescriptorSet;
		descriptorImageWrite.dstBinding			= firstBinding;
		descriptorImageWrite.dstArrayElement	= 0;
		descriptorImageWrite.descriptorType		= descriptorTypeVk;
		descriptorImageWrite.pBufferInfo		= nullptr;
		descriptorImageWrite.descriptorCount	= uint32_t(imageInfos.size());
		descriptorImageWrite.pImageInfo			= imageInfos.data();
		descriptorImageWrite.pTexelBufferView	= nullptr;

		vkUpdateDescriptorSets(m_pDevice->Device, 1, &descriptorImageWrite, 0, nullptr);
	}

	void DescriptorSetVK::WriteBufferDescriptors(const IBuffer* const* ppBuffers, const uint64* pOffsets, const uint64* pSizes, uint32 firstBinding, uint32 descriptorCount, EDescriptorType descriptorType)
	{
		VALIDATE(ppBuffers	!= nullptr);
		VALIDATE(pOffsets	!= nullptr);
		VALIDATE(pSizes		!= nullptr);

		const BufferVK* const*	ppVkBuffers			= reinterpret_cast<const BufferVK* const*>(ppBuffers);
		VkDescriptorType		descriptorTypeVk	= ConvertDescriptorType(descriptorType);

		TArray<VkDescriptorBufferInfo> bufferInfos(descriptorCount);
		for (uint32_t i = 0; i < descriptorCount; i++)
		{
			VkDescriptorBufferInfo& bufferInfo = bufferInfos[i];
			bufferInfo.offset	= pOffsets[i];
			bufferInfo.range		= pSizes[i];

			VALIDATE(ppVkBuffers[i] != nullptr);
			bufferInfo.buffer	= ppVkBuffers[i]->GetBuffer();
		}

		VkWriteDescriptorSet descriptorImageWrite = {};
		descriptorImageWrite.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorImageWrite.dstSet				= m_DescriptorSet;
		descriptorImageWrite.dstBinding			= firstBinding;
		descriptorImageWrite.dstArrayElement	= 0;
		descriptorImageWrite.descriptorType		= descriptorTypeVk;
		descriptorImageWrite.descriptorCount	= uint32_t(bufferInfos.size());
		descriptorImageWrite.pBufferInfo		= bufferInfos.data();
		descriptorImageWrite.pImageInfo			= nullptr;
		descriptorImageWrite.pTexelBufferView	= nullptr;

		vkUpdateDescriptorSets(m_pDevice->Device, 1, &descriptorImageWrite, 0, nullptr);
	}

	void DescriptorSetVK::WriteAccelerationStructureDescriptors(const IAccelerationStructure* const * ppAccelerationStructures, uint32 firstBinding, uint32 descriptorCount)
	{
        VALIDATE(ppAccelerationStructures != nullptr);
        
        TArray<VkAccelerationStructureKHR> accelerationStructures(descriptorCount);
        for (uint32_t i = 0; i < descriptorCount; i++)
        {
            const AccelerationStructureVK* pAccelerationStructureVk = reinterpret_cast<const AccelerationStructureVK*>(ppAccelerationStructures[i]);

            VALIDATE(pAccelerationStructureVk != nullptr);
            accelerationStructures[i] = pAccelerationStructureVk->GetAccelerationStructure();
        }
        
        VkWriteDescriptorSetAccelerationStructureNV descriptorAccelerationStructureInfo = {};
        descriptorAccelerationStructureInfo.sType                       = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
        descriptorAccelerationStructureInfo.accelerationStructureCount  = descriptorCount;
        descriptorAccelerationStructureInfo.pAccelerationStructures     = accelerationStructures.data();

        VkWriteDescriptorSet accelerationStructureWrite = {};
        accelerationStructureWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        accelerationStructureWrite.pNext            = &descriptorAccelerationStructureInfo;
        accelerationStructureWrite.dstSet           = m_DescriptorSet;
        accelerationStructureWrite.dstBinding       = firstBinding;
        accelerationStructureWrite.descriptorCount  = 1;
        accelerationStructureWrite.descriptorType   = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
        accelerationStructureWrite.pBufferInfo      = nullptr;
        accelerationStructureWrite.pImageInfo       = nullptr;
        accelerationStructureWrite.pTexelBufferView = nullptr;

        vkUpdateDescriptorSets(m_pDevice->Device, 1, &accelerationStructureWrite, 0, nullptr);
	}

	IDescriptorHeap* DescriptorSetVK::GetHeap()
	{
		m_pDescriptorHeap->AddRef();
		return m_pDescriptorHeap;
	}
}