#pragma once

#include "LambdaEngine.h"

#include "RenderGraphTypes.h"

#include "Containers/String.h"
#include "Containers/TArray.h"
#include "Containers/THashTable.h"
#include "Containers/TSet.h"

#include "Application/API/ApplicationEventHandler.h"

#include "Application/API/Events/MouseEvents.h"
#include "Application/API/Events/KeyEvents.h"

#include "Utilities/IOUtilities.h"

namespace LambdaEngine
{
	class RenderGraph;

	class LAMBDA_API RenderGraphEditor
	{
	public:
		DECL_REMOVE_COPY(RenderGraphEditor);
		DECL_REMOVE_MOVE(RenderGraphEditor);

		RenderGraphEditor();
		~RenderGraphEditor();

		void InitGUI();
		void Update();
		void RenderGUI();

		bool OnButtonReleased(const MouseButtonReleasedEvent& event);
		bool OnKeyPressed(const KeyPressedEvent& event);
		bool OnKeyReleased(const KeyReleasedEvent& event);

	private:
		void InitDefaultResources();

		void RenderResourceView(float textWidth, float textHeight);
		void RenderNewRenderGraphConfirmationView();
		void RenderAddResourceView();
		void RenderEditResourceView();
		void InternalRenderEditResourceView(RenderGraphResourceDesc* pResource, char* pNameBuffer, int32 nameBufferLength);

		void RenderShaderView(float textWidth, float textHeight);
		void RenderShaderTreeView(const LambdaDirectory& dir, float textWidth, float textHeight, int64& selectedIndex);

		void RenderGraphView();
		void RenderAddRenderStageView();
		void RenderSaveRenderGraphView();
		void RenderLoadRenderGraphView();

		void RenderParsedRenderGraphView();

		void RenderShaderBoxes(EditorRenderStageDesc* pRenderStage);
		void RenderShaderBoxCommon(String* pTarget, bool* pAdded = nullptr, bool* pRemoved = nullptr);

		bool AddHitGroupShader(const String& label, String* pTarget, FShaderStageFlag shaderStageFlag, FShaderStageFlag& typeAdded, FShaderStageFlag& typeRemoved, const glm::vec4& color = glm::vec4(0.4f, 0.4f, 1.0f, 0.5f));

		TArray<RenderGraphResourceDesc>::Iterator FindResource(const String& name);
		EditorResourceStateIdent CreateResourceState(const String& resourceName, ERenderGraphResourceType resourceType, const String& renderStageName, bool removable, ERenderGraphResourceBindingType bindingType);
		bool CheckLinkValid(int32* pSrcAttributeIndex, int32* pDstAttributeIndex);

		void RemoveResourceStateFrom(const String& name, EditorResourceStateGroup* pResourceStateGroup);
		void RemoveResourceStateFrom(const String& name, EditorRenderStageDesc* pRenderStageDesc);
		void DestroyLink(int32 linkIndex);

		void PushPinColorIfNeeded(EEditorPinType pinType, EditorRenderGraphResourceState* pResourceState, int32 targetAttributeIndex);
		void PopPinColorIfNeeded(EEditorPinType pinType, EditorRenderGraphResourceState* pResourceState, int32 targetAttributeIndex);
		bool CustomPinColorNeeded(EEditorPinType pinType, EditorRenderGraphResourceState* pResourceState, int32 targetAttributeIndex);

		void CalculateResourceStateBindingTypes(const EditorRenderStageDesc* pRenderStage, const RenderGraphResourceDesc* pResource, const EditorRenderGraphResourceState* pResourceState, TArray<ERenderGraphResourceBindingType>& bindingTypes, TArray<const char*>& bindingTypeNames);

		bool LoadFromFile(const String& renderGraphFileName);
		void SetInitialNodePositions();
		void ResetState();

		// Helpers
		void RenderDimChangeOptions(EPipelineStateType pipelineStateType, int32 selectedXYZOptions[3], float32 valuesXYZ[3], float inputWidth);

	private:
		bool												m_GUIInitialized = false;

		TArray<EditorResourceStateGroup>	m_ResourceStateGroups;
		EditorFinalOutput					m_FinalOutput		= {};

		TArray<RenderGraphResourceDesc> m_Resources;

		THashTable<int32, String>							m_RenderStageNameByInputAttributeIndex;
		THashTable<String, EditorRenderStageDesc>			m_RenderStagesByName;
		THashTable<int32, EditorRenderGraphResourceState>	m_ResourceStatesByHalfAttributeIndex;
		THashTable<int32, EditorRenderGraphResourceLink>	m_ResourceStateLinksByLinkIndex;

		EPipelineStateType			m_CurrentlyAddingRenderStage	= EPipelineStateType::PIPELINE_STATE_TYPE_NONE;
		ERenderGraphResourceType	m_CurrentlyAddingResource		= ERenderGraphResourceType::NONE;
		ERenderGraphTextureType		m_CurrentlyAddingTextureType	= ERenderGraphTextureType::TEXTURE_2D;
		String						m_CurrentlyEditingResource		= "";

		EditorStartedLinkInfo m_StartedLinkInfo				= {};

		LambdaDirectory m_FilesInShaderMap;

		RenderGraphStructureDesc	m_ParsedRenderGraphStructure	= {};
		bool						m_ParsedGraphValid				= true;
		bool						m_ParsedGraphDirty				= true;
		bool						m_ParsedGraphRenderDirty		= true;
		bool						m_ApplyRenderGraph				= false;

		bool						m_GraphActive					= false;

	private:
		static int32 s_NextNodeID;
		static int32 s_NextAttributeID;
		static int32 s_NextLinkID;
	};
}