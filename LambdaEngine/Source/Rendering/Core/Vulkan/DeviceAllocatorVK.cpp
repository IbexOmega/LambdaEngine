#include "Log/Log.h"

#include <mutex>
#include <string>

#include "Math/MathUtilities.h"

#include "Rendering/Core/Vulkan/DeviceAllocatorVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

namespace LambdaEngine
{
	/*
	 * DeviceMemoryBlockVK
	 */
	struct DeviceMemoryBlockVK
	{
		DeviceMemoryPageVK* pPage = nullptr;

		// Linked list of blocks
		DeviceMemoryBlockVK* pNext = nullptr;
		DeviceMemoryBlockVK* pPrevious = nullptr;

		// Size of the allocation
		uint64 SizeInBytes = 0;

		// Totoal size of the block (TotalSizeInBytes - SizeInBytes = AlignmentOffset)
		uint64 TotalSizeInBytes = 0;

		// Offset of the DeviceMemory
		uint64	Offset = 0;
		bool	IsFree = true;
	};

	/*
	 * DeviceMemoryPageVK
	 */
	class DeviceMemoryPageVK
	{
	public:
		DECL_UNIQUE_CLASS(DeviceMemoryPageVK);

		DeviceMemoryPageVK(const GraphicsDeviceVK* pDevice, DeviceAllocatorVK* pOwner, const uint32 id, const uint32 memoryIndex)
			: m_pDevice(pDevice)
			, m_pOwningAllocator(pOwner)
			, m_MemoryIndex(memoryIndex)
			, m_ID(id)
		{
		}

		~DeviceMemoryPageVK()
		{
			VALIDATE(ValidateBlock(m_pHead));
			VALIDATE(ValidateNoOverlap());

#ifdef LAMBDA_DEVELOPMENT
			if (m_pHead->pNext != nullptr)
			{
				LOG_WARNING("Memoryleak detected, m_pHead->pNext is not nullptr");
			}

			if (m_pHead->pPrevious != nullptr)
			{
				LOG_WARNING("Memoryleak detected, m_pHead->pPrevious is not nullptr");
			}
#endif
			DeviceMemoryBlockVK* pIterator = m_pHead;
			while (pIterator != nullptr)
			{
				DeviceMemoryBlockVK* pBlock = pIterator;
				pIterator = pBlock->pNext;

				SAFEDELETE(pBlock);
			}

			if (m_MappingCount > 0)
			{
				vkUnmapMemory(m_pDevice->Device, m_DeviceMemory);
				m_DeviceMemory = VK_NULL_HANDLE;

				m_pHostMemory = nullptr;
				m_MappingCount = 0;
			}

			vkFreeMemory(m_pDevice->Device, m_DeviceMemory, nullptr);
			m_DeviceMemory = VK_NULL_HANDLE;
		}

		bool Init(uint64 sizeInBytes)
		{
			std::scoped_lock<SpinLock> lock(m_Lock);

			VkResult result = m_pDevice->AllocateMemory(&m_DeviceMemory, sizeInBytes, m_MemoryIndex);
			if (result != VK_SUCCESS)
			{
				LOG_VULKAN_ERROR(result, "Failed to allocate memory");
				return false;
			}
			else
			{
				m_pHead = DBG_NEW DeviceMemoryBlockVK();
				m_pHead->pPage = this;
				m_pHead->TotalSizeInBytes = sizeInBytes;
				m_pHead->SizeInBytes = sizeInBytes;

				m_SizeInBytes = sizeInBytes;

#ifdef LAMBDA_DEBUG
				AllBlocks.EmplaceBack(m_pHead);
#endif
				return true;
			}
		}

		bool Allocate(AllocationVK* pAllocation, VkDeviceSize sizeInBytes, VkDeviceSize alignment, VkDeviceSize pageGranularity)
		{
			std::scoped_lock<SpinLock> lock(m_Lock);

			VALIDATE(pAllocation != nullptr);

			uint64 paddedOffset = 0;
			uint64 padding = 0;

			// Find a suitable block
			DeviceMemoryBlockVK* pBestFit = nullptr;
			VALIDATE(ValidateBlock(m_pHead));

			for (DeviceMemoryBlockVK* pIterator = m_pHead; pIterator != nullptr; pIterator = pIterator->pNext)
			{
				if (!pIterator->IsFree)
				{
					continue;
				}

				if (pIterator->SizeInBytes < sizeInBytes)
				{
					continue;
				}

				paddedOffset = AlignUp(pIterator->Offset, alignment);
				if (pageGranularity > 1)
				{
					DeviceMemoryBlockVK* pNext = pIterator->pNext;
					DeviceMemoryBlockVK* pPrevious = pIterator->pPrevious;

					if (pPrevious)
					{
						if (IsAliasing(pPrevious->Offset, pPrevious->TotalSizeInBytes, paddedOffset, pageGranularity))
						{
							paddedOffset = AlignUp(paddedOffset, pageGranularity);
						}
					}

					if (pNext)
					{
						if (IsAliasing(paddedOffset, sizeInBytes, pNext->Offset, pageGranularity))
						{
							continue;
						}
					}
				}

				padding = paddedOffset - pIterator->Offset;
				if (pIterator->SizeInBytes >= (sizeInBytes + padding))
				{
					pBestFit = pIterator;
					break;
				}
			}

			if (pBestFit == nullptr)
			{
				pAllocation->pBlock = nullptr;
				pAllocation->pAllocator = nullptr;
				pAllocation->Offset = 0;
				pAllocation->Memory = 0;
				return false;
			}

			// Divide block
			const uint64 paddedSizeInBytes = (padding + sizeInBytes);
			if (pBestFit->SizeInBytes > paddedSizeInBytes)
			{
				DeviceMemoryBlockVK* pNewBlock = DBG_NEW DeviceMemoryBlockVK();
				pNewBlock->Offset = pBestFit->Offset + paddedSizeInBytes;
				pNewBlock->pPage = this;
				pNewBlock->pNext = pBestFit->pNext;
				pNewBlock->pPrevious = pBestFit;
				pNewBlock->SizeInBytes = pBestFit->SizeInBytes - paddedSizeInBytes;
				pNewBlock->TotalSizeInBytes = pNewBlock->SizeInBytes;
				pNewBlock->IsFree = true;

				if (pBestFit->pNext)
				{
					pBestFit->pNext->pPrevious = pNewBlock;
				}

				pBestFit->pNext = pNewBlock;
				VALIDATE(ValidateChain());
				VALIDATE(ValidateBlock(pNewBlock));

#ifdef LAMBDA_DEBUG
				AllBlocks.EmplaceBack(pNewBlock);
#endif
			}

			// Set new attributes of block
			pBestFit->SizeInBytes = sizeInBytes;
			pBestFit->TotalSizeInBytes = paddedSizeInBytes;
			pBestFit->IsFree = false;

			VALIDATE(ValidateChain());
			VALIDATE(ValidateBlock(pBestFit));

			// Setup allocation
			pAllocation->Memory = m_DeviceMemory;
			pAllocation->Offset = paddedOffset;
			pAllocation->pBlock = pBestFit;
			pAllocation->pAllocator = m_pOwningAllocator;

			VALIDATE(ValidateNoOverlap());
			return true;
		}

		bool Free(AllocationVK* pAllocation)
		{
			std::scoped_lock<SpinLock> lock(m_Lock);
			VALIDATE(pAllocation != nullptr);

			DeviceMemoryBlockVK* pBlock = pAllocation->pBlock;
			VALIDATE(pBlock != nullptr);
			VALIDATE(ValidateChain());
			VALIDATE(ValidateBlock(pBlock));

			pBlock->IsFree = true;
			DeviceMemoryBlockVK* pPrevious = pBlock->pPrevious;
			if (pPrevious)
			{
				if (pPrevious->IsFree)
				{
					pPrevious->pNext = pBlock->pNext;
					if (pBlock->pNext)
					{
						pBlock->pNext->pPrevious = pPrevious;
					}

					pPrevious->SizeInBytes += pBlock->TotalSizeInBytes;
					pPrevious->TotalSizeInBytes += pBlock->TotalSizeInBytes;

					SAFEDELETE(pBlock);
					pBlock = pPrevious;
				}
			}

			VALIDATE(ValidateChain());

			DeviceMemoryBlockVK* pNext = pBlock->pNext;
			if (pNext)
			{
				if (pNext->IsFree)
				{
					if (pNext->pNext)
					{
						pNext->pNext->pPrevious = pBlock;
					}
					pBlock->pNext = pNext->pNext;

					pBlock->SizeInBytes += pNext->TotalSizeInBytes;
					pBlock->TotalSizeInBytes += pNext->TotalSizeInBytes;

					SAFEDELETE(pNext);
				}
			}

			VALIDATE(ValidateChain());
			VALIDATE(ValidateNoOverlap());

			pAllocation->Memory = VK_NULL_HANDLE;
			pAllocation->Offset = 0;
			pAllocation->pBlock = nullptr;
			pAllocation->pAllocator = nullptr;

			return true;
		}

		void* Map(const AllocationVK* pAllocation)
		{
			std::scoped_lock<SpinLock> lock(m_Lock);

			VALIDATE(pAllocation != nullptr);
			VALIDATE(pAllocation->pBlock != nullptr);
			VALIDATE(ValidateBlock(pAllocation->pBlock));

			if (m_MappingCount == 0)
			{
				vkMapMemory(m_pDevice->Device, m_DeviceMemory, 0, VK_WHOLE_SIZE, 0, (void**)&m_pHostMemory);
			}

			VALIDATE(m_pHostMemory != nullptr);

			m_MappingCount++;
			return m_pHostMemory + pAllocation->Offset;
		}

		void Unmap(const AllocationVK* pAllocation)
		{
			std::scoped_lock<SpinLock> lock(m_Lock);

			VALIDATE(pAllocation != nullptr);
			VALIDATE(pAllocation->pBlock != nullptr);
			VALIDATE(ValidateBlock(pAllocation->pBlock));

			UNREFERENCED_VARIABLE(pAllocation);

			m_MappingCount--;
			if (m_MappingCount == 0)
			{
				vkUnmapMemory(m_pDevice->Device, m_DeviceMemory);
				m_MappingCount = 0;
			}
		}

		void SetName(const String& debugName)
		{
			std::scoped_lock<SpinLock> lock(m_Lock);
			m_pDevice->SetVulkanObjectName(debugName, reinterpret_cast<uint64>(m_DeviceMemory), VK_OBJECT_TYPE_DEVICE_MEMORY);
		}

		FORCEINLINE bool IsEmpty() const
		{
			if (m_pHead)
			{
				return m_pHead->IsFree && (m_pHead->TotalSizeInBytes == m_SizeInBytes);
			}

			return true;
		}

		FORCEINLINE uint32 GetMemoryIndex() const
		{
			return m_MemoryIndex;
		}

		FORCEINLINE uint32 GetID() const
		{
			return m_ID;
		}

	private:
		bool IsAliasing(VkDeviceSize aOffset, VkDeviceSize aSize, VkDeviceSize bOffset, VkDeviceSize pageGranularity)
		{
			VALIDATE(aSize > 0);
			VALIDATE(pageGranularity > 0);

			VkDeviceSize aEnd = aOffset + (aSize - 1);
			VkDeviceSize aEndPage = aEnd & ~(pageGranularity - 1);
			VkDeviceSize bStart = bOffset;
			VkDeviceSize bStartPage = bStart & ~(pageGranularity - 1);
			return aEndPage >= bStartPage;
		}

		/*
		 * Debug tools
		 */
		bool ValidateBlock(DeviceMemoryBlockVK* pBlock) const
		{
			DeviceMemoryBlockVK* pIterator = m_pHead;
			while (pIterator != nullptr)
			{
				if (pIterator == pBlock)
				{
					return true;
				}
				pIterator = pIterator->pNext;
			}

#ifdef LAMBDA_DEBUG
			for (DeviceMemoryBlockVK* pBlockIt : AllBlocks)
			{
				if (pBlock == pBlockIt)
				{
					DEBUGBREAK();
				}
			}
#endif

			return false;
		}

		bool ValidateNoOverlap() const
		{
			DeviceMemoryBlockVK* pIterator = m_pHead;
			while (pIterator != nullptr)
			{
				if (pIterator->pNext)
				{
					if ((pIterator->Offset + pIterator->TotalSizeInBytes) > (pIterator->pNext->Offset))
					{
						LOG_DEBUG("Overlap found");
						return false;
					}
				}

				pIterator = pIterator->pNext;
			}

			return true;
		}

		bool ValidateChain() const
		{
			TArray<DeviceMemoryBlockVK*> traversedBlocks;

			// Traverse all the blocks and put them into an array in order
			DeviceMemoryBlockVK* pIterator = m_pHead;
			DeviceMemoryBlockVK* pTail = nullptr;
			while (pIterator != nullptr)
			{
				traversedBlocks.EmplaceBack(pIterator);
				pTail = pIterator;
				pIterator = pIterator->pNext;
			}

			/* When we have reached the tail we start going backwards and check so
			that the order in the array is the same as when we traversed forward*/
			while (pTail != nullptr)
			{
				// In case this fails our chain is not valid
				if (pTail != traversedBlocks.GetBack())
				{
					return false;
				}

				traversedBlocks.PopBack();
				pTail = pTail->pPrevious;
			}

			return true;
		}

	private:
		const GraphicsDeviceVK* const m_pDevice;
		DeviceAllocatorVK* const m_pOwningAllocator;
		const uint32 m_MemoryIndex;
		const uint32 m_ID;
		uint64 m_SizeInBytes = 0;

		DeviceMemoryBlockVK* m_pHead = nullptr;
		byte* m_pHostMemory = nullptr;
		VkDeviceMemory m_DeviceMemory = VK_NULL_HANDLE;
		uint32 m_MappingCount = 0;

		SpinLock m_Lock;
#ifdef LAMBDA_DEBUG
		TArray<DeviceMemoryBlockVK*> AllBlocks;
#endif
	};

	/*
	 * DeviceAllocatorVK
	 */

	DeviceAllocatorVK::DeviceAllocatorVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice)
		, m_Pages()
		, m_DeviceProperties()
		, m_PageSize()
		, m_DebugName()
		, m_Lock()
	{
	}

	DeviceAllocatorVK::~DeviceAllocatorVK()
	{
		for (DeviceMemoryPageVK* pMemoryPage : m_Pages)
		{
			SAFEDELETE(pMemoryPage);
		}

		m_Pages.Clear();
	}

	bool DeviceAllocatorVK::Init(const String& debugName, VkDeviceSize pageSize)
	{
		SetName(debugName);

		m_PageSize = pageSize;
		m_DeviceProperties = m_pDevice->GetPhysicalDeviceProperties();

		return true;
	}

	bool DeviceAllocatorVK::Allocate(AllocationVK* pAllocation, uint64 sizeInBytes, uint64 alignment, uint32 memoryIndex)
	{
		VALIDATE(pAllocation != nullptr);
		VALIDATE(sizeInBytes > 0);

		std::scoped_lock<SpinLock> lock(m_Lock);

		// Check if this size every will be possible with this allocator
		VkDeviceSize alignedSize = AlignUp(sizeInBytes, alignment);
		if (alignedSize >= m_PageSize)
		{
			pAllocation->pBlock = nullptr;
			pAllocation->pAllocator = nullptr;
			pAllocation->Offset = 0;
			pAllocation->Memory = 0;
			return false;
		}

		if (!m_Pages.IsEmpty())
		{
			for (DeviceMemoryPageVK* pMemoryPage : m_Pages)
			{
				VALIDATE(pMemoryPage != nullptr);

				if (pMemoryPage->GetMemoryIndex() == memoryIndex)
				{
					// Try and allocate otherwise we continue the search
					if (pMemoryPage->Allocate(pAllocation, sizeInBytes, alignment, m_DeviceProperties.limits.bufferImageGranularity))
					{
						return true;
					}
				}
			}
		}

		DeviceMemoryPageVK* pNewMemoryPage = DBG_NEW DeviceMemoryPageVK(m_pDevice, this, uint32(m_Pages.GetSize()), memoryIndex);
		if (!pNewMemoryPage->Init(m_PageSize))
		{
			pAllocation->pBlock = nullptr;
			pAllocation->pAllocator = nullptr;
			pAllocation->Offset = 0;
			pAllocation->Memory = 0;
			SAFEDELETE(pNewMemoryPage);
			return false;
		}
		else
		{
			SetPageName(pNewMemoryPage);
		}

		m_Pages.EmplaceBack(pNewMemoryPage);
		return pNewMemoryPage->Allocate(pAllocation, sizeInBytes, alignment, m_DeviceProperties.limits.bufferImageGranularity);
	}

	bool DeviceAllocatorVK::Free(AllocationVK* pAllocation)
	{
		std::scoped_lock<SpinLock> lock(m_Lock);

		VALIDATE(pAllocation != nullptr);
		DeviceMemoryBlockVK* pBlock = pAllocation->pBlock;

		VALIDATE(pBlock != nullptr);
		DeviceMemoryPageVK* pPage = pBlock->pPage;

		VALIDATE(pPage != nullptr);
		
		// Remove an empty page
		const bool result = pPage->Free(pAllocation);
		if (pPage->IsEmpty())
		{
			for (auto it = m_Pages.Begin(); it != m_Pages.End(); it++)
			{
				if (*it == pPage)
				{
					m_Pages.Erase(it);
					break;
				}
			}

			SAFEDELETE(pPage);
		}

		return result;
	}

	void* DeviceAllocatorVK::Map(const AllocationVK* pAllocation)
	{
		std::scoped_lock<SpinLock> lock(m_Lock);

		VALIDATE(pAllocation != nullptr);
		DeviceMemoryBlockVK* pBlock = pAllocation->pBlock;

		VALIDATE(pBlock != nullptr);
		DeviceMemoryPageVK* pPage = pBlock->pPage;

		VALIDATE(pPage != nullptr);
		return pPage->Map(pAllocation);
	}

	void DeviceAllocatorVK::Unmap(const AllocationVK* pAllocation)
	{
		std::scoped_lock<SpinLock> lock(m_Lock);

		VALIDATE(pAllocation != nullptr);
		DeviceMemoryBlockVK* pBlock = pAllocation->pBlock;

		VALIDATE(pBlock != nullptr);
		DeviceMemoryPageVK* pPage = pBlock->pPage;

		VALIDATE(pPage != nullptr);
		return pPage->Unmap(pAllocation);
	}

	void DeviceAllocatorVK::SetPageName(DeviceMemoryPageVK* pMemoryPage)
	{
		VALIDATE(pMemoryPage != nullptr);

		String name = m_DebugName + "[PageID=" + std::to_string(pMemoryPage->GetID()) + "]";
		pMemoryPage->SetName(name);
	}

	void DeviceAllocatorVK::SetName(const String& debugName)
	{
		if (!debugName.empty())
		{
			std::scoped_lock<SpinLock> lock(m_Lock);

			m_DebugName = debugName;
			for (DeviceMemoryPageVK* pMemoryPage : m_Pages)
			{
				SetPageName(pMemoryPage);
			}
		}
	}
}
