#include "Memory/API/StackAllocator.h"

namespace LambdaEngine
{
	/*
	* StackAllocator
	*/

	StackAllocator::StackAllocator(uint32 sizePerArena)
		: m_ArenaIndex(0)
		, m_SizePerArena(sizePerArena)
		, m_pCurrentArena(nullptr)
		, m_Arenas()
	{
		m_pCurrentArena = &m_Arenas.EmplaceBack(m_SizePerArena);
	}

	void* StackAllocator::Push(uint32 size)
	{
		VALIDATE(m_pCurrentArena != nullptr);
		
		// Do we need a new arena or can we take next one?
		if (!m_pCurrentArena->CanAllocate(size))
		{
			m_ArenaIndex++;
			if (m_Arenas.GetSize() <= m_ArenaIndex)
			{
				m_pCurrentArena = &m_Arenas.EmplaceBack(m_SizePerArena);
			}
			else
			{
				m_pCurrentArena = &m_Arenas[m_ArenaIndex];
				m_pCurrentArena->Reset();
			}

			VALIDATE(m_pCurrentArena != nullptr);
		}

		// Allocate
		return m_pCurrentArena->Allocate(size);
	}

	void StackAllocator::Pop(uint32 size)
	{
		VALIDATE(m_pCurrentArena != nullptr);

		if (!m_pCurrentArena->CanPop(size) && (m_ArenaIndex > 0))
		{
			m_ArenaIndex--;
			m_pCurrentArena = &m_Arenas[m_ArenaIndex];
				
			VALIDATE(m_pCurrentArena != nullptr);
		}
		
		m_pCurrentArena->Pop(size);
	}

	void StackAllocator::Reset()
	{
		for (MemoryArena& arena : m_Arenas)
		{
			arena.Reset();
		}

		m_ArenaIndex	= 0;
		m_pCurrentArena	= &m_Arenas[m_ArenaIndex];
	}
}