#include "Rendering/EntityMaskManager.h"

#include "Utilities/StringUtilities.h"
#include "ECS/ECSCore.h"

#include "Game/ECS/Components/Misc/MeshPaintComponent.h"

using namespace LambdaEngine;

THashTable<const ComponentType*, uint32>			EntityMaskManager::s_ComponentTypeToMaskMap;
THashTable<Entity, DrawArgExtensionGroupEntry>		EntityMaskManager::s_EntityToExtensionGroupEntryMap;
THashTable<uint32, DrawArgExtensionDesc>			EntityMaskManager::s_ExtensionMaskToExtensionDescMap;

bool LambdaEngine::EntityMaskManager::Init()
{
	BindTypeToExtensionDesc(MeshPaintComponent::Type(), {1});

	return true;
}

void LambdaEngine::EntityMaskManager::AddExtensionToEntity(Entity entity, const ComponentType*  type, const DrawArgExtensionData& DrawArgExtension)
{
	uint32 extensionMask = GetExtensionMask(type);

	// Bind entity to the extension data
	auto it = s_EntityToExtensionGroupEntryMap.find(entity);
	if (it != s_EntityToExtensionGroupEntryMap.end())
	{
		uint32 newIndex = it->second.extensionGroup.ExtensionCount++;
		CopyDrawArgExtensionData(it->second.extensionGroup.pExtensions[newIndex], DrawArgExtension);

		it->second.extensionGroup.pExtensionMasks[newIndex] = extensionMask;
		it->second.Mask |= extensionMask;
		return;
	}

	DrawArgExtensionGroupEntry& groupEntry = s_EntityToExtensionGroupEntryMap[entity];
	groupEntry.extensionGroup.ExtensionCount = 1;
	groupEntry.extensionGroup.pExtensionMasks[0] = extensionMask;
	CopyDrawArgExtensionData(groupEntry.extensionGroup.pExtensions[0], DrawArgExtension);
	groupEntry.Mask = 1 | extensionMask;
}

DrawArgExtensionGroup& LambdaEngine::EntityMaskManager::GetExtensionGroup(Entity entity)
{
	VALIDATE(s_EntityToExtensionGroupEntryMap.contains(entity));
	return s_EntityToExtensionGroupEntryMap[entity].extensionGroup;
}

uint32 LambdaEngine::EntityMaskManager::FetchEntityMask(Entity entity)
{
	auto it = s_EntityToExtensionGroupEntryMap.find(entity);
	if (it != s_EntityToExtensionGroupEntryMap.end())
		return it->second.Mask;
	return 1; // No extra extension is used.
}

TArray<uint32> LambdaEngine::EntityMaskManager::ExtractComponentMasksFromEntityMask(uint32 mask)
{
	TArray<uint32> componentMasks;
	for (uint32 bit = 1; bit < sizeof(mask) * 8; bit++)
	{
		// Check if bit is set, if it is. That bit is the mask for the component.
		uint32 componentMask = mask & (1 << bit);
		if (componentMask > 0)
			componentMasks.PushBack(componentMask);
	}
	return componentMasks;
}

uint32 LambdaEngine::EntityMaskManager::GetExtensionMask(const ComponentType* type)
{
	auto it = s_ComponentTypeToMaskMap.find(type);
	if (it != s_ComponentTypeToMaskMap.end())
		return it->second;

	// Generate a mask for this component type. Mask 0 is used as an error code.
	static uint32 s_MaskCounter = 0;
	uint32 mask = FLAG(++s_MaskCounter);
	s_ComponentTypeToMaskMap[type] = mask;
	return	mask;
}

const DrawArgExtensionDesc& LambdaEngine::EntityMaskManager::GetExtensionDescFromExtensionMask(uint32 mask)
{
	ASSERT(s_ExtensionMaskToExtensionDescMap.contains(mask));
	return s_ExtensionMaskToExtensionDescMap[mask];
}

void LambdaEngine::EntityMaskManager::BindTypeToExtensionDesc(const ComponentType* type, DrawArgExtensionDesc extensionDesc)
{
	uint32 extensionMask = GetExtensionMask(type);

	// Set extension description for later use
	auto eIt = s_ExtensionMaskToExtensionDescMap.find(extensionMask);
	if (eIt == s_ExtensionMaskToExtensionDescMap.end())
		s_ExtensionMaskToExtensionDescMap[extensionMask] = extensionDesc;
}

void LambdaEngine::EntityMaskManager::CopyDrawArgExtensionData(DrawArgExtensionData& dest, const DrawArgExtensionData& src)
{
	memcpy(&dest, &src, sizeof(DrawArgExtensionData));
}
