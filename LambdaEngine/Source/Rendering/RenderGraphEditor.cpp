#include "Rendering/RenderGraphEditor.h"

#include "Application/API/CommonApplication.h"

#include "Rendering/Core/API/GraphicsHelpers.h"

#include "Log/Log.h"

#include "Utilities/IOUtilities.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <imnodes.h>

#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/filereadstream.h>

namespace LambdaEngine
{
	int32		RenderGraphEditor::s_NextNodeID				= 0;
	int32		RenderGraphEditor::s_NextAttributeID		= 0;
	int32		RenderGraphEditor::s_NextLinkID				= 0;

	constexpr const uint32 HOVERED_COLOR	= IM_COL32(232, 27, 86, 255);
	constexpr const uint32 SELECTED_COLOR	= IM_COL32(162, 19, 60, 255);

	constexpr const uint32 EXTERNAL_RESOURCE_STATE_GROUP_INDEX	= 0;
	constexpr const uint32 TEMPORAL_RESOURCE_STATE_GROUP_INDEX	= 1;
	constexpr const uint32 NUM_RESOURCE_STATE_GROUPS			= 2;

	RenderGraphEditor::RenderGraphEditor()
	{
		CommonApplication::Get()->AddEventHandler(this);

		imnodes::StyleColorsDark();

		imnodes::PushColorStyle(imnodes::ColorStyle_TitleBarHovered,	HOVERED_COLOR);
		imnodes::PushColorStyle(imnodes::ColorStyle_TitleBarSelected,	SELECTED_COLOR);

		imnodes::PushColorStyle(imnodes::ColorStyle_LinkHovered,		HOVERED_COLOR);
		imnodes::PushColorStyle(imnodes::ColorStyle_LinkSelected,		SELECTED_COLOR);

		InitDefaultResources();
	}

	RenderGraphEditor::~RenderGraphEditor()
	{
	}

	void RenderGraphEditor::RenderGUI()
	{
		if (ImGui::Begin("Render Graph Editor"))
		{
			ImVec2 contentRegionMin = ImGui::GetWindowContentRegionMin();
			ImVec2 contentRegionMax = ImGui::GetWindowContentRegionMax();

			float contentRegionWidth	= contentRegionMax.x - contentRegionMin.x;
			float contentRegionHeight	= contentRegionMax.y - contentRegionMin.y;

			float maxResourcesViewTextWidth = 0.0f;
			float textHeight = ImGui::CalcTextSize("I").y + 5.0f;

			for (auto resourceIt = m_ResourcesByName.begin(); resourceIt != m_ResourcesByName.end(); resourceIt++)
			{
				const EditorResource* pResource = &resourceIt->second;
				ImVec2 textSize = ImGui::CalcTextSize(pResource->Name.c_str());

				maxResourcesViewTextWidth = textSize.x > maxResourcesViewTextWidth ? textSize.x : maxResourcesViewTextWidth;
			}

			for (auto shaderIt = m_FilesInShaderDirectory.begin(); shaderIt != m_FilesInShaderDirectory.end(); shaderIt++)
			{
				const String& shaderName = *shaderIt;
				ImVec2 textSize = ImGui::CalcTextSize(shaderName.c_str());

				maxResourcesViewTextWidth = textSize.x > maxResourcesViewTextWidth ? textSize.x : maxResourcesViewTextWidth;
			}

			float editButtonWidth = 120.0f;
			float removeButtonWidth = 120.0f;
			ImVec2 resourceViewSize(2.0f * maxResourcesViewTextWidth + editButtonWidth + removeButtonWidth, contentRegionHeight);

			if (ImGui::BeginChild("##Graph Resources View", resourceViewSize))
			{
				ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Resources");
				ImGui::NewLine();

				if (ImGui::BeginChild("##Resource View", ImVec2(0.0f, contentRegionHeight * 0.5f - textHeight * 5.0f), true, ImGuiWindowFlags_MenuBar))
				{
					RenderResourceView(maxResourcesViewTextWidth, textHeight);
					RenderAddResourceView();
					RenderEditResourceView();
				}
				ImGui::EndChild();

				ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Shaders");
				ImGui::NewLine();

				if (ImGui::BeginChild("##Shader View", ImVec2(0.0f, contentRegionHeight * 0.5f - textHeight * 5.0f), true))
				{
					RenderShaderView(maxResourcesViewTextWidth, textHeight);
				}
				ImGui::EndChild();

				if (ImGui::Button("Refresh Shaders"))
				{
					m_FilesInShaderDirectory = EnumerateFilesInDirectory("../Assets/Shaders/", true);
				}
			}
			ImGui::EndChild();

			ImGui::SameLine();

			if (ImGui::BeginChild("##Graph Views", ImVec2(contentRegionWidth - resourceViewSize.x, 0.0f)))
			{
				if (ImGui::BeginTabBar("##Render Graph Editor Tabs"))
				{
					if (ImGui::BeginTabItem("Graph Editor"))
					{
						if (ImGui::BeginChild("##Graph Editor View", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_MenuBar))
						{
							RenderGraphView();
							RenderAddRenderStageView();
							RenderSaveRenderGraphView();
							RenderLoadRenderGraphView();
						}

						ImGui::EndChild();
						ImGui::EndTabItem();
					}

					if (ImGui::BeginTabItem("Parsed Graph"))
					{
						if (ImGui::BeginChild("##Parsed Graph View"))
						{
							RenderParsedRenderGraphView();
						}

						ImGui::EndChild();
						ImGui::EndTabItem();
					}

					ImGui::EndTabBar();
				}
			}

			ImGui::EndChild();
		}
		ImGui::End();

		if (m_ParsedGraphDirty)
		{
			if (!ParseStructure(true))
			{
				LOG_ERROR("Parsing Error: %s", m_ParsingError.c_str());
				m_ParsingError = "";
			}

			m_ParsedGraphDirty = false;
		}
	}

	void RenderGraphEditor::OnButtonReleased(EMouseButton button)
	{
		//imnodes seems to be bugged when releasing a link directly after starting creation, so we check this here
		if (EMouseButton::MOUSE_BUTTON_LEFT)
		{
			if (m_StartedLinkInfo.LinkStarted)
			{
				m_StartedLinkInfo = {};
			}
		}
	}

	void RenderGraphEditor::OnKeyPressed(EKey key, uint32 modifierMask, bool isRepeat)
	{
		if (key == EKey::KEY_LEFT_SHIFT && !isRepeat)
		{
			imnodes::PushAttributeFlag(imnodes::AttributeFlags_EnableLinkDetachWithDragClick);
		}
	}

	void RenderGraphEditor::OnKeyReleased(EKey key)
	{
		if (key == EKey::KEY_LEFT_SHIFT)
		{
			imnodes::PopAttributeFlag();
		}
	}

	void RenderGraphEditor::InitDefaultResources()
	{
		m_FilesInShaderDirectory = EnumerateFilesInDirectory("../Assets/Shaders/", true);

		m_FinalOutput.Name				= "FINAL_OUTPUT";
		m_FinalOutput.NodeIndex			= s_NextNodeID++;

		EditorResourceStateGroup externalResourcesGroup = {};
		externalResourcesGroup.Name			= "EXTERNAL_RESOURCES";
		externalResourcesGroup.NodeIndex	= s_NextNodeID++;

		EditorResourceStateGroup temporalResourcesGroup = {};
		temporalResourcesGroup.Name			= "TEMPORAL_RESOURCES";
		temporalResourcesGroup.NodeIndex	= s_NextNodeID++;

		//EditorRenderStage imguiRenderStage = {};
		//imguiRenderStage.Name					= RENDER_GRAPH_IMGUI_STAGE_NAME;
		//imguiRenderStage.NodeIndex				= s_NextNodeID++;
		//imguiRenderStage.InputAttributeIndex	= s_NextAttributeID;
		//imguiRenderStage.Type					= EPipelineStateType::GRAPHICS;
		//imguiRenderStage.CustomRenderer			= true;
		//imguiRenderStage.Enabled				= true;

		//m_RenderStageNameByInputAttributeIndex[imguiRenderStage.InputAttributeIndex] = imguiRenderStage.Name;
		//m_RenderStagesByName[imguiRenderStage.Name] = imguiRenderStage;

		s_NextAttributeID += 2;

		{
			EditorResource resource = {};
			resource.Name						= RENDER_GRAPH_BACK_BUFFER_ATTACHMENT;
			resource.Type						= EEditorResourceType::TEXTURE;
			resource.SubResourceType			= EEditorSubResourceType::PER_FRAME;
			resource.SubResourceArrayCount		= 1;
			resource.Editable					= false;
			resource.TextureFormat				= EFormat::FORMAT_B8G8R8A8_UNORM;
			m_ResourcesByName[resource.Name]	= resource;

			m_FinalOutput.BackBufferAttributeIndex					= CreateResourceState(resource.Name, m_FinalOutput.Name, false);
		}

		{
			EditorResource resource = {};
			resource.Name						= FULLSCREEN_QUAD_VERTEX_BUFFER;
			resource.Type						= EEditorResourceType::BUFFER;
			resource.SubResourceType			= EEditorSubResourceType::ARRAY;
			resource.SubResourceArrayCount		= 1;
			resource.Editable					= false;
			m_ResourcesByName[resource.Name]	= resource;

			externalResourcesGroup.ResourceStates[resource.Name] = CreateResourceState(resource.Name, externalResourcesGroup.Name, false);
		}

		{
			EditorResource resource = {};
			resource.Name						= PER_FRAME_BUFFER;
			resource.Type						= EEditorResourceType::BUFFER;
			resource.SubResourceType			= EEditorSubResourceType::ARRAY;
			resource.SubResourceArrayCount		= 1;
			resource.Editable					= false;
			m_ResourcesByName[resource.Name]	= resource;

			externalResourcesGroup.ResourceStates[resource.Name] = CreateResourceState(resource.Name, externalResourcesGroup.Name, false);
		}

		{
			EditorResource resource = {};
			resource.Name						= SCENE_LIGHTS_BUFFER;
			resource.Type						= EEditorResourceType::BUFFER;
			resource.SubResourceType			= EEditorSubResourceType::ARRAY;
			resource.SubResourceArrayCount		= 1;
			resource.Editable					= false;
			m_ResourcesByName[resource.Name]	= resource;

			externalResourcesGroup.ResourceStates[resource.Name] = CreateResourceState(resource.Name, externalResourcesGroup.Name, false);
		}

		{
			EditorResource resource = {};
			resource.Name						= SCENE_MAT_PARAM_BUFFER;
			resource.Type						= EEditorResourceType::BUFFER;
			resource.SubResourceType			= EEditorSubResourceType::ARRAY;
			resource.SubResourceArrayCount		= 1;
			resource.Editable					= false;
			m_ResourcesByName[resource.Name]	= resource;

			externalResourcesGroup.ResourceStates[resource.Name] = CreateResourceState(resource.Name, externalResourcesGroup.Name, false);
		}

		{
			EditorResource resource = {};
			resource.Name						= SCENE_VERTEX_BUFFER;
			resource.Type						= EEditorResourceType::BUFFER;
			resource.SubResourceType			= EEditorSubResourceType::ARRAY;
			resource.SubResourceArrayCount		= 1;
			resource.Editable					= false;
			m_ResourcesByName[resource.Name]	= resource;

			externalResourcesGroup.ResourceStates[resource.Name] = CreateResourceState(resource.Name, externalResourcesGroup.Name, false);
		}

		{
			EditorResource resource = {};
			resource.Name						= SCENE_INDEX_BUFFER;
			resource.Type						= EEditorResourceType::BUFFER;
			resource.SubResourceType			= EEditorSubResourceType::ARRAY;
			resource.SubResourceArrayCount		= 1;
			resource.Editable					= false;
			m_ResourcesByName[resource.Name]	= resource;

			externalResourcesGroup.ResourceStates[resource.Name] = CreateResourceState(resource.Name, externalResourcesGroup.Name, false);
		}

		{
			EditorResource resource = {};
			resource.Name						= SCENE_INSTANCE_BUFFER;
			resource.Type						= EEditorResourceType::BUFFER;
			resource.SubResourceType			= EEditorSubResourceType::ARRAY;
			resource.SubResourceArrayCount		= 1;
			resource.Editable					= false;
			m_ResourcesByName[resource.Name]	= resource;

			externalResourcesGroup.ResourceStates[resource.Name] = CreateResourceState(resource.Name, externalResourcesGroup.Name, false);
		}

		{
			EditorResource resource = {};
			resource.Name						= SCENE_MESH_INDEX_BUFFER;
			resource.Type						= EEditorResourceType::BUFFER;
			resource.SubResourceType			= EEditorSubResourceType::ARRAY;
			resource.SubResourceArrayCount		= 1;
			resource.Editable					= false;
			m_ResourcesByName[resource.Name]	= resource;

			externalResourcesGroup.ResourceStates[resource.Name] = CreateResourceState(resource.Name, externalResourcesGroup.Name, false);
		}

		{
			EditorResource resource = {};
			resource.Name						= SCENE_TLAS;
			resource.Type						= EEditorResourceType::ACCELERATION_STRUCTURE;
			resource.SubResourceType			= EEditorSubResourceType::ARRAY;
			resource.SubResourceArrayCount		= 1;
			resource.Editable					= false;
			m_ResourcesByName[resource.Name]	= resource;

			externalResourcesGroup.ResourceStates[resource.Name] = CreateResourceState(resource.Name, externalResourcesGroup.Name, false);
		}

		{
			EditorResource resource = {};
			resource.Name						= SCENE_ALBEDO_MAPS;
			resource.Type						= EEditorResourceType::TEXTURE;
			resource.SubResourceType			= EEditorSubResourceType::ARRAY;
			resource.SubResourceArrayCount		= MAX_UNIQUE_MATERIALS;
			resource.Editable					= false;
			resource.TextureFormat				= EFormat::FORMAT_R8G8B8A8_UNORM;
			m_ResourcesByName[resource.Name]	= resource;

			externalResourcesGroup.ResourceStates[resource.Name] = CreateResourceState(resource.Name, externalResourcesGroup.Name, false);
		}

		{
			EditorResource resource = {};
			resource.Name						= SCENE_NORMAL_MAPS;
			resource.Type						= EEditorResourceType::TEXTURE;
			resource.SubResourceType			= EEditorSubResourceType::ARRAY;
			resource.SubResourceArrayCount		= MAX_UNIQUE_MATERIALS;
			resource.Editable					= false;
			resource.TextureFormat				= EFormat::FORMAT_R8G8B8A8_UNORM;
			m_ResourcesByName[resource.Name]	= resource;

			externalResourcesGroup.ResourceStates[resource.Name] = CreateResourceState(resource.Name, externalResourcesGroup.Name, false);
		}

		{
			EditorResource resource = {};
			resource.Name						= SCENE_AO_MAPS;
			resource.Type						= EEditorResourceType::TEXTURE;
			resource.SubResourceType			= EEditorSubResourceType::ARRAY;
			resource.SubResourceArrayCount		= MAX_UNIQUE_MATERIALS;
			resource.Editable					= false;
			resource.TextureFormat				= EFormat::FORMAT_R8G8B8A8_UNORM;
			m_ResourcesByName[resource.Name]	= resource;

			externalResourcesGroup.ResourceStates[resource.Name] = CreateResourceState(resource.Name, externalResourcesGroup.Name, false);
		}

		{
			EditorResource resource = {};
			resource.Name						= SCENE_ROUGHNESS_MAPS;
			resource.Type						= EEditorResourceType::TEXTURE;
			resource.SubResourceType			= EEditorSubResourceType::ARRAY;
			resource.SubResourceArrayCount		= MAX_UNIQUE_MATERIALS;
			resource.Editable					= false;
			resource.TextureFormat				= EFormat::FORMAT_R8G8B8A8_UNORM;
			m_ResourcesByName[resource.Name]	= resource;

			externalResourcesGroup.ResourceStates[resource.Name] = CreateResourceState(resource.Name, externalResourcesGroup.Name, false);
		}

		{
			EditorResource resource = {};
			resource.Name						= SCENE_METALLIC_MAPS;
			resource.Type						= EEditorResourceType::TEXTURE;
			resource.SubResourceType			= EEditorSubResourceType::ARRAY;
			resource.SubResourceArrayCount		= MAX_UNIQUE_MATERIALS;
			resource.Editable					= false;
			resource.TextureFormat				= EFormat::FORMAT_R8G8B8A8_UNORM;
			m_ResourcesByName[resource.Name]	= resource;

			externalResourcesGroup.ResourceStates[resource.Name] = CreateResourceState(resource.Name, externalResourcesGroup.Name, false);
		}

		m_ResourceStateGroups.resize(NUM_RESOURCE_STATE_GROUPS);
		m_ResourceStateGroups[EXTERNAL_RESOURCE_STATE_GROUP_INDEX] = externalResourcesGroup;
		m_ResourceStateGroups[TEMPORAL_RESOURCE_STATE_GROUP_INDEX] = temporalResourcesGroup;
	}

	void RenderGraphEditor::RenderResourceView(float textWidth, float textHeight)
	{
		bool openAddResourcePopup = false;
		bool openEditResourcePopup = false;

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Menu"))
			{
				if (ImGui::MenuItem("Add Texture", NULL, nullptr))
				{
					openAddResourcePopup = true;
					m_CurrentlyAddingResource = EEditorResourceType::TEXTURE;
				}

				if (ImGui::MenuItem("Add Buffer", NULL, nullptr))
				{
					openAddResourcePopup = true;
					m_CurrentlyAddingResource = EEditorResourceType::BUFFER;
				}

				if (ImGui::MenuItem("Add Acceleration Structure", NULL, nullptr))
				{
					openAddResourcePopup = true;
					m_CurrentlyAddingResource = EEditorResourceType::ACCELERATION_STRUCTURE;
				}

				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		ImGui::Columns(2);

		static int32 selectedResourceIndex			= -1;
		static EditorResource* pSelectedResource	= nullptr;
		static EditorResource* pRemovedResource		= nullptr;

		for (auto resourceIt = m_ResourcesByName.begin(); resourceIt != m_ResourcesByName.end(); resourceIt++)
		{
			int32 index = std::distance(m_ResourcesByName.begin(), resourceIt);
			EditorResource* pResource = &(resourceIt.value());

			if (ImGui::Selectable(pResource->Name.c_str(), selectedResourceIndex == index, ImGuiSeparatorFlags_None, ImVec2(textWidth, textHeight)))
			{
				selectedResourceIndex	= index;
				pSelectedResource		= pResource;
			}

			if (ImGui::BeginDragDropSource())
			{
				ImGui::SetDragDropPayload("RESOURCE", &pResource, sizeof(EditorResource*));
				ImGui::EndDragDropSource();
			}

			if (pResource->Editable)
			{
				ImGui::SameLine();
				if (ImGui::Button(("Edit##" + pResource->Name).c_str()))
				{
					openEditResourcePopup = true;
					m_CurrentlyEditingResource = pResource->Name;
				}

				ImGui::SameLine();
				if (ImGui::Button(("-##" + pResource->Name).c_str()))
				{
					pRemovedResource = pResource;
				}
			}
		}

		if (pSelectedResource != nullptr)
		{
			ImGui::NextColumn();

			String resourceType = RenderGraphResourceTypeToString(pSelectedResource->Type);
			ImGui::Text("Type: %s", resourceType.c_str());

			String subResourceType = RenderGraphSubResourceTypeToString(pSelectedResource->SubResourceType);
			ImGui::Text("Sub Resource Type: %s", subResourceType.c_str());

			if (pSelectedResource->SubResourceType == EEditorSubResourceType::ARRAY)
			{
				String subResourceArrayCount = std::to_string(pSelectedResource->SubResourceArrayCount);
				ImGui::Text("Array Count: %s", subResourceArrayCount.c_str());
			}
		}

		if (pRemovedResource != nullptr)
		{
			//Update Resource State Groups and Resource States
			for (auto resourceStateGroupIt = m_ResourceStateGroups.begin(); resourceStateGroupIt != m_ResourceStateGroups.end(); resourceStateGroupIt++)
			{
				EditorResourceStateGroup* pResourceStateGroup = &(*resourceStateGroupIt);

				auto resourceStateIndexIt = pResourceStateGroup->ResourceStates.find(pRemovedResource->Name);

				if (resourceStateIndexIt != pResourceStateGroup->ResourceStates.end())
				{
					int32 attributeIndex = resourceStateIndexIt->second;
					auto resourceStateIt = m_ResourceStatesByHalfAttributeIndex.find(attributeIndex / 2);
					EditorRenderGraphResourceState* pResourceState = &resourceStateIt->second;

					DestroyLink(pResourceState->InputLinkIndex);

					//Copy so that DestroyLink wont delete from set we're iterating through
					TSet<int32> outputLinkIndices = pResourceState->OutputLinkIndices;
					for (auto outputLinkIt = outputLinkIndices.begin(); outputLinkIt != outputLinkIndices.end(); outputLinkIt++)
					{
						int32 linkToBeDestroyedIndex = *outputLinkIt;
						DestroyLink(linkToBeDestroyedIndex);
					}

					m_ResourceStatesByHalfAttributeIndex.erase(resourceStateIt);
					pResourceStateGroup->ResourceStates.erase(resourceStateIndexIt);
				}
			}

			//Update Render Stages and Resource States
			for (auto renderStageIt = m_RenderStagesByName.begin(); renderStageIt != m_RenderStagesByName.end(); renderStageIt++)
			{
				EditorRenderStage* pRenderStage = &renderStageIt->second;

				auto resourceStateIndexIt = pRenderStage->ResourceStates.find(pRemovedResource->Name);

				if (resourceStateIndexIt != pRenderStage->ResourceStates.end())
				{
					int32 attributeIndex = resourceStateIndexIt->second;
					auto resourceStateIt = m_ResourceStatesByHalfAttributeIndex.find(attributeIndex / 2);
					EditorRenderGraphResourceState* pResourceState = &resourceStateIt->second;

					DestroyLink(pResourceState->InputLinkIndex);

					//Copy so that DestroyLink wont delete from set we're iterating through
					TSet<int32> outputLinkIndices = pResourceState->OutputLinkIndices;
					for (auto outputLinkIt = outputLinkIndices.begin(); outputLinkIt != outputLinkIndices.end(); outputLinkIt++)
					{
						int32 linkToBeDestroyedIndex = *outputLinkIt;
						DestroyLink(linkToBeDestroyedIndex);
					}

					m_ResourceStatesByHalfAttributeIndex.erase(resourceStateIt);
					pRenderStage->ResourceStates.erase(resourceStateIndexIt);
				}
			}

			m_ResourcesByName.erase(pRemovedResource->Name);
		}

		if (openAddResourcePopup)
			ImGui::OpenPopup("Add Resource ##Popup");
		if (openEditResourcePopup)
			ImGui::OpenPopup("Edit Resource ##Popup");
	}

	void RenderGraphEditor::RenderAddResourceView()
	{
		constexpr const int32 RESOURCE_NAME_BUFFER_LENGTH = 256;
		static char resourceNameBuffer[RESOURCE_NAME_BUFFER_LENGTH];
		static int32 selectedSubResourceType	= 0;
		static int32 subResourceArrayCount		= 1;

		ImGui::SetNextWindowSize(ImVec2(360, 200));
		if (ImGui::BeginPopupModal("Add Resource ##Popup"))
		{
			if (m_CurrentlyAddingResource != EEditorResourceType::NONE)
			{
				ImGui::AlignTextToFramePadding();

				ImGui::Text("Resource Name:    ");
				ImGui::SameLine();
				ImGui::InputText("##Resource Name", resourceNameBuffer, RESOURCE_NAME_BUFFER_LENGTH, ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank);

				const char* subresourceTypes[2] = { "Array", "Per Frame" };

				ImGui::Text("Sub Resource Type:");
				ImGui::SameLine();
				ImGui::Combo("##Sub Resource Type", &selectedSubResourceType, subresourceTypes, ARR_SIZE(subresourceTypes));

				if (selectedSubResourceType == 0)
				{
					ImGui::Text("Array Count:      ");
					ImGui::SameLine();
					ImGui::SliderInt("##Sub Resource Array Count", &subResourceArrayCount, 1, 128);
				}

				bool done = false;
				bool resourceExists = m_ResourcesByName.find(resourceNameBuffer) != m_ResourcesByName.end();
				bool resourceNameEmpty = resourceNameBuffer[0] == 0;
				bool resourceInvalid = resourceExists || resourceNameEmpty;

				if (resourceExists)
				{
					ImGui::Text("A resource with that name already exists...");
				}
				else if (resourceNameEmpty)
				{
					ImGui::Text("Resource name empty...");
				}

				if (ImGui::Button("Close"))
				{
					done = true;
				}

				ImGui::SameLine();

				if (resourceInvalid)
				{
					ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
				}

				if (ImGui::Button("Create"))
				{
					EditorResource newResource = {};
					newResource.Name				= resourceNameBuffer;
					newResource.Type				= m_CurrentlyAddingResource;

					if (selectedSubResourceType == 0)
					{
						newResource.SubResourceType			= EEditorSubResourceType::ARRAY;
						newResource.SubResourceArrayCount	= subResourceArrayCount;
					}
					else if (selectedSubResourceType == 1)
					{
						newResource.SubResourceType			= EEditorSubResourceType::PER_FRAME;
						newResource.SubResourceArrayCount	= 1;
					}

					newResource.Editable			= true;
						
					m_ResourcesByName[newResource.Name] = newResource;

					done = true;
				}

				if (resourceInvalid)
				{
					ImGui::PopItemFlag();
					ImGui::PopStyleVar();
				}

				if (done)
				{
					ZERO_MEMORY(resourceNameBuffer, RESOURCE_NAME_BUFFER_LENGTH);
					selectedSubResourceType		= 0;
					subResourceArrayCount		= 1;
					m_CurrentlyAddingResource	= EEditorResourceType::NONE;
					ImGui::CloseCurrentPopup();
				}
			}
			else
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void RenderGraphEditor::RenderEditResourceView()
	{
		constexpr const int32 RESOURCE_NAME_BUFFER_LENGTH = 256;
		static char resourceNameBuffer[RESOURCE_NAME_BUFFER_LENGTH];
		static int32 selectedSubResourceType = -1;
		static int32 subResourceArrayCount = -1;

		ImGui::SetNextWindowSize(ImVec2(360, 200));
		if (ImGui::BeginPopupModal("Edit Resource ##Popup"))
		{
			if (m_CurrentlyEditingResource != "")
			{
				auto editedResourceIt = m_ResourcesByName.find(m_CurrentlyEditingResource);

				if (editedResourceIt == m_ResourcesByName.end())
				{
					ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
					LOG_ERROR("Editing non-existant resource!");
					return;
				}

				EditorResource* pEditedResource = &editedResourceIt.value();

				if (resourceNameBuffer[0] == 0)
				{
					memcpy(resourceNameBuffer, m_CurrentlyEditingResource.c_str(), m_CurrentlyEditingResource.size());
				}

				if (selectedSubResourceType == -1)
				{
					if (pEditedResource->SubResourceType == EEditorSubResourceType::ARRAY)
						selectedSubResourceType = 0;
					else if (pEditedResource->SubResourceType == EEditorSubResourceType::PER_FRAME)
						selectedSubResourceType = 1;
				}

				if (subResourceArrayCount == -1)
				{
					subResourceArrayCount = pEditedResource->SubResourceArrayCount;
				}

				ImGui::AlignTextToFramePadding();

				ImGui::Text("Resource Name:    ");
				ImGui::SameLine();
				ImGui::InputText("##Resource Name", resourceNameBuffer, RESOURCE_NAME_BUFFER_LENGTH, ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank);

				const char* subresourceTypes[2] = { "Array", "Per Frame" };

				ImGui::Text("Sub Resource Type:");
				ImGui::SameLine();
				ImGui::Combo("##Sub Resource Type", &selectedSubResourceType, subresourceTypes, ARR_SIZE(subresourceTypes));

				if (selectedSubResourceType == 0)
				{
					ImGui::Text("Array Count:      ");
					ImGui::SameLine();
					ImGui::SliderInt("##Sub Resource Array Count", &subResourceArrayCount, 1, 128);
				}

				bool done = false;
				auto existingResourceIt = m_ResourcesByName.find(resourceNameBuffer);
				bool resourceExists = existingResourceIt != m_ResourcesByName.end() && existingResourceIt != editedResourceIt;
				bool resourceNameEmpty = resourceNameBuffer[0] == 0;
				bool resourceInvalid = resourceExists || resourceNameEmpty;

				if (resourceExists)
				{
					ImGui::Text("Another resource with that name already exists...");
				}
				else if (resourceNameEmpty)
				{
					ImGui::Text("Resource name empty...");
				}

				if (ImGui::Button("Close"))
				{
					done = true;
				}

				ImGui::SameLine();

				if (resourceInvalid)
				{
					ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
				}

				if (ImGui::Button("Done"))
				{
					EditorResource editedResourceCopy = *pEditedResource;

					//Update Resource State Groups and Resource States
					for (auto resourceStateGroupIt = m_ResourceStateGroups.begin(); resourceStateGroupIt != m_ResourceStateGroups.end(); resourceStateGroupIt++)
					{
						EditorResourceStateGroup* pResourceStateGroup = &(*resourceStateGroupIt);

						auto resourceStateIndexIt = pResourceStateGroup->ResourceStates.find(editedResourceCopy.Name);

						if (resourceStateIndexIt != pResourceStateGroup->ResourceStates.end())
						{
							int32 attributeIndex = resourceStateIndexIt->second;
							EditorRenderGraphResourceState* pResourceState = &m_ResourceStatesByHalfAttributeIndex[attributeIndex / 2];
							pResourceState->ResourceName = resourceNameBuffer;

							pResourceStateGroup->ResourceStates.insert_at_position(pResourceStateGroup->ResourceStates.erase(resourceStateIndexIt), { pResourceState->ResourceName, attributeIndex });
						}
					}

					//Update Render Stages and Resource States
					for (auto renderStageIt = m_RenderStagesByName.begin(); renderStageIt != m_RenderStagesByName.end(); renderStageIt++)
					{
						EditorRenderStage* pRenderStage = &renderStageIt->second;

						auto resourceStateIndexIt = pRenderStage->ResourceStates.find(editedResourceCopy.Name);

						if (resourceStateIndexIt != pRenderStage->ResourceStates.end())
						{
							int32 attributeIndex = resourceStateIndexIt->second;
							EditorRenderGraphResourceState* pResourceState = &m_ResourceStatesByHalfAttributeIndex[attributeIndex / 2];
							pResourceState->ResourceName = resourceNameBuffer;

							pRenderStage->ResourceStates.insert_at_position(pRenderStage->ResourceStates.erase(resourceStateIndexIt), { pResourceState->ResourceName, attributeIndex });
						}
					}

					editedResourceCopy.Name				= resourceNameBuffer;

					if (selectedSubResourceType == 0)
					{
						editedResourceCopy.SubResourceType			= EEditorSubResourceType::ARRAY;
						editedResourceCopy.SubResourceArrayCount	= subResourceArrayCount;
					}
					else if (selectedSubResourceType == 1)
					{
						editedResourceCopy.SubResourceType			= EEditorSubResourceType::PER_FRAME;
						editedResourceCopy.SubResourceArrayCount	= 1;
					}

					m_ResourcesByName.insert_at_position(m_ResourcesByName.erase(editedResourceIt), { editedResourceCopy.Name, editedResourceCopy });

					done = true;
				}

				if (resourceInvalid)
				{
					ImGui::PopItemFlag();
					ImGui::PopStyleVar();
				}

				if (done)
				{
					ZERO_MEMORY(resourceNameBuffer, RESOURCE_NAME_BUFFER_LENGTH);
					selectedSubResourceType		= -1;
					subResourceArrayCount		= -1;
					m_CurrentlyEditingResource	= "";
					ImGui::CloseCurrentPopup();
				}
			}
			else
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void RenderGraphEditor::RenderShaderView(float textWidth, float textHeight)
	{
		UNREFERENCED_VARIABLE(textHeight);

		static int32 selectedResourceIndex = -1;
		
		for (auto fileIt = m_FilesInShaderDirectory.begin(); fileIt != m_FilesInShaderDirectory.end(); fileIt++)
		{
			int32 index = std::distance(m_FilesInShaderDirectory.begin(), fileIt);
			const String* pFilename = &(*fileIt);

			if (pFilename->find(".glsl") != String::npos)
			{
				if (ImGui::Selectable(pFilename->c_str(), selectedResourceIndex == index, ImGuiSeparatorFlags_None, ImVec2(textWidth, textHeight)))
				{
					selectedResourceIndex = index;
				}

				if (ImGui::BeginDragDropSource())
				{
					ImGui::SetDragDropPayload("SHADER", &pFilename, sizeof(const String*));
					ImGui::EndDragDropSource();
				}
			}
		}
	}

	void RenderGraphEditor::RenderGraphView()
	{
		bool openAddRenderStagePopup = false;
		bool openSaveRenderStagePopup = false;
		bool openLoadRenderStagePopup = false;

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Menu"))
			{
				if (ImGui::MenuItem("Save", NULL, nullptr))
				{
					openSaveRenderStagePopup = true;
				}

				if (ImGui::MenuItem("Load", NULL, nullptr))
				{
					openLoadRenderStagePopup = true;
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem("Add Graphics Render Stage", NULL, nullptr))
				{
					m_CurrentlyAddingRenderStage = EPipelineStateType::GRAPHICS;
					openAddRenderStagePopup = true;
				}

				if (ImGui::MenuItem("Add Compute Render Stage", NULL, nullptr))
				{
					m_CurrentlyAddingRenderStage = EPipelineStateType::COMPUTE;
					openAddRenderStagePopup = true;
				}

				if (ImGui::MenuItem("Add Ray Tracing Render Stage", NULL, nullptr))
				{
					m_CurrentlyAddingRenderStage = EPipelineStateType::RAY_TRACING;
					openAddRenderStagePopup = true;
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		imnodes::BeginNodeEditor();

		ImGui::GetWindowDrawList()->Flags &= ~ImDrawListFlags_AntiAliasedLines; //Disable this since otherwise link thickness does not work

		//Resource State Groups
		for (auto resourceStateGroupIt = m_ResourceStateGroups.begin(); resourceStateGroupIt != m_ResourceStateGroups.end(); resourceStateGroupIt++)
		{
			EditorResourceStateGroup* pResourceStateGroup = &(*resourceStateGroupIt);

			imnodes::BeginNode(pResourceStateGroup->NodeIndex);

			imnodes::BeginNodeTitleBar();
			ImGui::TextUnformatted(pResourceStateGroup->Name.c_str());
			imnodes::EndNodeTitleBar();

			String resourceStateToRemove = "";

			for (auto resourceIt = pResourceStateGroup->ResourceStates.begin(); resourceIt != pResourceStateGroup->ResourceStates.end(); resourceIt++)
			{
				uint32 primaryAttributeIndex = resourceIt->second / 2;
				uint32 inputAttributeIndex = resourceIt->second;
				uint32 outputAttributeIndex = inputAttributeIndex + 1;
				EditorRenderGraphResourceState* pResourceState = &m_ResourceStatesByHalfAttributeIndex[primaryAttributeIndex];

				PushPinColorIfNeeded(EEditorPinType::OUTPUT, nullptr, pResourceState, inputAttributeIndex);
				imnodes::BeginOutputAttribute(outputAttributeIndex);
				ImGui::Text(pResourceState->ResourceName.c_str());
				ImGui::SameLine();
				if (pResourceState->Removable)
				{
					if (ImGui::Button("-"))
					{
						resourceStateToRemove = pResourceState->ResourceName;
					}
				}
				imnodes::EndOutputAttribute();
				PopPinColorIfNeeded(EEditorPinType::OUTPUT, nullptr, pResourceState, inputAttributeIndex);
			}

			ImGui::Button("Drag Resource Here");

			if (ImGui::BeginDragDropTarget())
			{
				const ImGuiPayload* pPayload = ImGui::AcceptDragDropPayload("RESOURCE");

				if (pPayload != nullptr)
				{
					EditorResource* pResource = *reinterpret_cast<EditorResource**>(pPayload->Data);

					if (pResourceStateGroup->ResourceStates.find(pResource->Name) == pResourceStateGroup->ResourceStates.end())
					{
						pResourceStateGroup->ResourceStates[pResource->Name] = CreateResourceState(pResource->Name, pResourceStateGroup->Name, true);
						m_ParsedGraphDirty = true;
					}
				}

				ImGui::EndDragDropTarget();
			}

			imnodes::EndNode();

			//Remove resource if "-" button pressed
			if (resourceStateToRemove.size() > 0)
			{
				int32 resourceAttributeIndex = pResourceStateGroup->ResourceStates[resourceStateToRemove];
				int32 primaryAttributeIndex = resourceAttributeIndex / 2;
				int32 inputAttributeIndex = resourceAttributeIndex;
				int32 outputAttributeIndex = resourceAttributeIndex + 1;

				pResourceStateGroup->ResourceStates.erase(resourceStateToRemove);

				EditorRenderGraphResourceState* pResourceState = &m_ResourceStatesByHalfAttributeIndex[primaryAttributeIndex];

				DestroyLink(pResourceState->InputLinkIndex);

				//Copy so that DestroyLink wont delete from set we're iterating through
				TSet<int32> outputLinkIndices = pResourceState->OutputLinkIndices;
				for (auto outputLinkIt = outputLinkIndices.begin(); outputLinkIt != outputLinkIndices.end(); outputLinkIt++)
				{
					int32 linkToBeDestroyedIndex = *outputLinkIt;
					DestroyLink(linkToBeDestroyedIndex);
				}

				m_ResourceStatesByHalfAttributeIndex.erase(primaryAttributeIndex);
				m_ParsedGraphDirty = true;
			}
		}

		//Final Output
		{
			imnodes::BeginNode(m_FinalOutput.NodeIndex);

			imnodes::BeginNodeTitleBar();
			ImGui::TextUnformatted("FINAL_OUTPUT");
			imnodes::EndNodeTitleBar();

			uint32 primaryAttributeIndex	= m_FinalOutput.BackBufferAttributeIndex / 2;
			uint32 inputAttributeIndex		= m_FinalOutput.BackBufferAttributeIndex;
			uint32 outputAttributeIndex		= inputAttributeIndex + 1;
			EditorRenderGraphResourceState* pResource = &m_ResourceStatesByHalfAttributeIndex[primaryAttributeIndex];

			PushPinColorIfNeeded(EEditorPinType::INPUT, nullptr, pResource, inputAttributeIndex);
			imnodes::BeginInputAttribute(inputAttributeIndex);
			ImGui::Text(pResource->ResourceName.c_str());
			imnodes::EndInputAttribute();
			PopPinColorIfNeeded(EEditorPinType::INPUT, nullptr, pResource, inputAttributeIndex);

			imnodes::EndNode();
		}

		//Render Stages
		for (auto renderStageIt = m_RenderStagesByName.begin(); renderStageIt != m_RenderStagesByName.end(); renderStageIt++)
		{
			EditorRenderStage* pRenderStage = &renderStageIt->second;

			imnodes::BeginNode(pRenderStage->NodeIndex);

			String renderStageType = RenderStageTypeToString(pRenderStage->Type);
			
			imnodes::BeginNodeTitleBar();
			ImGui::Text("%s : [%s]", pRenderStage->Name.c_str(), renderStageType.c_str());
			ImGui::TextUnformatted("Enabled: ");
			ImGui::SameLine();
			if (ImGui::Checkbox("##Render Stage Enabled Checkbox", &pRenderStage->Enabled)) m_ParsedGraphDirty = true;
			ImGui::Text("Weight: %d", pRenderStage->Weight);
			imnodes::EndNodeTitleBar();


			String resourceStateToRemove = "";

			//Render Resources
			for (auto resourceIt = pRenderStage->ResourceStates.begin(); resourceIt != pRenderStage->ResourceStates.end(); resourceIt++)
			{
				int32 primaryAttributeIndex		= resourceIt->second / 2;
				int32 inputAttributeIndex		= resourceIt->second;
				int32 outputAttributeIndex		= inputAttributeIndex + 1;
				EditorRenderGraphResourceState* pResourceState = &m_ResourceStatesByHalfAttributeIndex[primaryAttributeIndex];

				PushPinColorIfNeeded(EEditorPinType::INPUT, pRenderStage, pResourceState, inputAttributeIndex);
				imnodes::BeginInputAttribute(inputAttributeIndex);
				ImGui::Text(pResourceState->ResourceName.c_str());
				imnodes::EndInputAttribute();
				PopPinColorIfNeeded(EEditorPinType::INPUT, pRenderStage, pResourceState, inputAttributeIndex);

				ImGui::SameLine();

				PushPinColorIfNeeded(EEditorPinType::OUTPUT, pRenderStage, pResourceState, outputAttributeIndex);
				imnodes::BeginOutputAttribute(outputAttributeIndex);
				if (pResourceState->Removable)
				{
					if (ImGui::Button("-"))
					{
						resourceStateToRemove = pResourceState->ResourceName;
					}
				}
				else
				{
					ImGui::InvisibleButton("##Resouce State Invisible Button", ImGui::CalcTextSize("-"));
				}
				imnodes::EndOutputAttribute();
				PopPinColorIfNeeded(EEditorPinType::OUTPUT, pRenderStage, pResourceState, outputAttributeIndex);
			}

			PushPinColorIfNeeded(EEditorPinType::RENDER_STAGE_INPUT, pRenderStage, nullptr , -1);
			imnodes::BeginInputAttribute(pRenderStage->InputAttributeIndex);
			ImGui::Text("New Input");
			imnodes::EndInputAttribute();
			PopPinColorIfNeeded(EEditorPinType::RENDER_STAGE_INPUT, pRenderStage, nullptr, -1);

			ImGui::Button("Drag Resource Here");

			if (ImGui::BeginDragDropTarget())
			{
				const ImGuiPayload* pPayload = ImGui::AcceptDragDropPayload("RESOURCE");

				if (pPayload != nullptr)
				{
					EditorResource* pResource = *reinterpret_cast<EditorResource**>(pPayload->Data);

					if (pRenderStage->ResourceStates.find(pResource->Name) == pRenderStage->ResourceStates.end())
					{
						pRenderStage->ResourceStates[pResource->Name] = CreateResourceState(pResource->Name, pRenderStage->Name, true);
						m_ParsedGraphDirty = true;
					}
				}

				ImGui::EndDragDropTarget();
			}

			//Render Shader Boxes
			if (!pRenderStage->CustomRenderer)
			{
				RenderShaderBoxes(pRenderStage);
			}

			imnodes::EndNode();

			//Remove resource if "-" button pressed
			if (resourceStateToRemove.size() > 0)
			{
				int32 resourceAttributeIndex	= pRenderStage->ResourceStates[resourceStateToRemove];
				int32 primaryAttributeIndex		= resourceAttributeIndex / 2;
				int32 inputAttributeIndex		= resourceAttributeIndex;
				int32 outputAttributeIndex		= resourceAttributeIndex + 1;

				pRenderStage->ResourceStates.erase(resourceStateToRemove);

				EditorRenderGraphResourceState* pResourceState = &m_ResourceStatesByHalfAttributeIndex[primaryAttributeIndex];

				DestroyLink(pResourceState->InputLinkIndex);

				//Copy so that DestroyLink wont delete from set we're iterating through
				TSet<int32> outputLinkIndices = pResourceState->OutputLinkIndices;
				for (auto outputLinkIt = outputLinkIndices.begin(); outputLinkIt != outputLinkIndices.end(); outputLinkIt++)
				{
					int32 linkToBeDestroyedIndex = *outputLinkIt;
					DestroyLink(linkToBeDestroyedIndex);
				}

				m_ResourceStatesByHalfAttributeIndex.erase(primaryAttributeIndex);
				m_ParsedGraphDirty = true;
			}
		}

		//Render Links
		for (auto linkIt = m_ResourceStateLinksByLinkIndex.begin(); linkIt != m_ResourceStateLinksByLinkIndex.end(); linkIt++)
		{
			EditorRenderGraphResourceLink* pLink = &linkIt->second;

			imnodes::Link(pLink->LinkIndex, pLink->SrcAttributeIndex, pLink->DstAttributeIndex);
		}

		imnodes::EndNodeEditor();

		int32 linkStartAttributeID		= -1;
		
		if (imnodes::IsLinkStarted(&linkStartAttributeID))
		{
			m_StartedLinkInfo.LinkStarted				= true;
			m_StartedLinkInfo.LinkStartAttributeID		= linkStartAttributeID;
			m_StartedLinkInfo.LinkStartedOnInputPin		= linkStartAttributeID % 2 == 0;
			m_StartedLinkInfo.LinkStartedOnResource		= m_ResourceStatesByHalfAttributeIndex[linkStartAttributeID / 2].ResourceName;
		}

		if (imnodes::IsLinkDropped())
		{
			m_StartedLinkInfo = {};
		}

		//Check for newly created Links
		int32 srcAttributeIndex = 0;
		int32 dstAttributeIndex = 0;
		if (imnodes::IsLinkCreated(&srcAttributeIndex, &dstAttributeIndex))
		{
			if (CheckLinkValid(&srcAttributeIndex, &dstAttributeIndex))
			{
				//Check if Render Stage Input Attribute
				auto renderStageNameIt = m_RenderStageNameByInputAttributeIndex.find(dstAttributeIndex);
				if (renderStageNameIt != m_RenderStageNameByInputAttributeIndex.end())
				{
					EditorRenderStage* pRenderStage = &m_RenderStagesByName[renderStageNameIt->second];
					EditorResource* pResource = &m_ResourcesByName[m_ResourceStatesByHalfAttributeIndex[srcAttributeIndex / 2].ResourceName];

					if (pRenderStage->ResourceStates.find(pResource->Name) == pRenderStage->ResourceStates.end())
						pRenderStage->ResourceStates[pResource->Name] = CreateResourceState(pResource->Name, pRenderStage->Name, true);

					dstAttributeIndex = pRenderStage->ResourceStates[pResource->Name];
				}

				EditorRenderGraphResourceState* pSrcResource = &m_ResourceStatesByHalfAttributeIndex[srcAttributeIndex / 2];
				EditorRenderGraphResourceState* pDstResource = &m_ResourceStatesByHalfAttributeIndex[dstAttributeIndex / 2];

				if (pSrcResource->ResourceName == pDstResource->ResourceName)
				{
					//Destroy old link
					if (pDstResource->InputLinkIndex != -1)
					{
						int32 linkToBeDestroyedIndex = pDstResource->InputLinkIndex;
						DestroyLink(linkToBeDestroyedIndex);
					}

					EditorRenderGraphResourceLink newLink = {};
					newLink.LinkIndex			= s_NextLinkID++;
					newLink.SrcAttributeIndex	= srcAttributeIndex;
					newLink.DstAttributeIndex	= dstAttributeIndex;
					m_ResourceStateLinksByLinkIndex[newLink.LinkIndex] = newLink;

					pDstResource->InputLinkIndex	= newLink.LinkIndex;
					pSrcResource->OutputLinkIndices.insert(newLink.LinkIndex);
					m_ParsedGraphDirty = true;
				}
			}

			m_StartedLinkInfo = {};
		}

		//Check for newly destroyed links
		int32 linkIndex = 0;
		if (imnodes::IsLinkDestroyed(&linkIndex))
		{
			DestroyLink(linkIndex);

			m_StartedLinkInfo = {};
		}

		if		(openAddRenderStagePopup)	ImGui::OpenPopup("Add Render Stage ##Popup");
		else if (openSaveRenderStagePopup)	ImGui::OpenPopup("Save Render Graph ##Popup");
		else if (openLoadRenderStagePopup)	ImGui::OpenPopup("Load Render Graph ##Popup");
	}

	void RenderGraphEditor::RenderAddRenderStageView()
	{
		constexpr const int32 RENDER_STAGE_NAME_BUFFER_LENGTH = 256;
		static char renderStageNameBuffer[RENDER_STAGE_NAME_BUFFER_LENGTH];
		static bool customRenderer = false;

		ImGui::SetNextWindowSize(ImVec2(360, 120));
		if (ImGui::BeginPopupModal("Add Render Stage ##Popup"))
		{
			if (m_CurrentlyAddingRenderStage != EPipelineStateType::NONE)
			{
				ImGui::AlignTextToFramePadding();

				ImGui::Text("Render Stage Name:");
				ImGui::SameLine();
				ImGui::InputText("##Render Stage Name", renderStageNameBuffer, RENDER_STAGE_NAME_BUFFER_LENGTH, ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank);

				ImGui::Text("Custom Renderer:  ");
				ImGui::SameLine();
				ImGui::Checkbox("##Render Stage Custom Renderer", &customRenderer);

				bool done = false;
				bool renderStageExists = m_RenderStagesByName.find(renderStageNameBuffer) != m_RenderStagesByName.end();
				bool renderStageNameEmpty = renderStageNameBuffer[0] == 0;
				bool renderStageInvalid = renderStageExists || renderStageNameEmpty;

				if (renderStageExists)
				{
					ImGui::Text("A render stage with that name already exists...");
				}
				else if (renderStageNameEmpty)
				{
					ImGui::Text("Render Stage name empty...");
				}

				if (ImGui::Button("Close"))
				{
					done = true;
				}

				ImGui::SameLine();

				if (renderStageInvalid)
				{
					ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
				}

				if (ImGui::Button("Create"))
				{
					EditorRenderStage newRenderStage = {};
					newRenderStage.Name					= renderStageNameBuffer;
					newRenderStage.NodeIndex			= s_NextNodeID++;
					newRenderStage.InputAttributeIndex	= s_NextAttributeID;
					newRenderStage.Type					= m_CurrentlyAddingRenderStage;
					newRenderStage.CustomRenderer		= customRenderer;
					newRenderStage.Enabled				= true;

					s_NextAttributeID += 2;

					m_RenderStageNameByInputAttributeIndex[newRenderStage.InputAttributeIndex] = newRenderStage.Name;
					m_RenderStagesByName[newRenderStage.Name] = newRenderStage;

					done = true;
				}

				if (renderStageInvalid)
				{
					ImGui::PopItemFlag();
					ImGui::PopStyleVar();
				}

				if (done)
				{
					ZERO_MEMORY(renderStageNameBuffer, RENDER_STAGE_NAME_BUFFER_LENGTH);
					customRenderer = false;
					m_CurrentlyAddingRenderStage = EPipelineStateType::NONE;
					ImGui::CloseCurrentPopup();
				}
			}
			else
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void RenderGraphEditor::RenderSaveRenderGraphView()
	{
		constexpr const int32 RENDER_GRAPH_NAME_BUFFER_LENGTH = 256;
		static char renderGraphNameBuffer[RENDER_GRAPH_NAME_BUFFER_LENGTH];

		ImGui::SetNextWindowSize(ImVec2(360, 120));
		if (ImGui::BeginPopupModal("Save Render Graph ##Popup"))
		{
			ImGui::Text("Render Graph Name:");
			ImGui::SameLine();
			ImGui::InputText("##Render Graph Name", renderGraphNameBuffer, RENDER_GRAPH_NAME_BUFFER_LENGTH, ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank);

			bool done = false;
			bool renderGraphNameEmpty = renderGraphNameBuffer[0] == 0;

			if (renderGraphNameEmpty)
			{
				ImGui::Text("Render Graph name empty...");
			}

			if (ImGui::Button("Close"))
			{
				done = true;
			}

			ImGui::SameLine();

			if (renderGraphNameEmpty)
			{
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
			}

			if (ImGui::Button("Save"))
			{
				SaveToFile(renderGraphNameBuffer);
				done = true;
			}

			if (renderGraphNameEmpty)
			{
				ImGui::PopItemFlag();
				ImGui::PopStyleVar();
			}

			if (done)
			{
				ZERO_MEMORY(renderGraphNameBuffer, RENDER_GRAPH_NAME_BUFFER_LENGTH);
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void RenderGraphEditor::RenderLoadRenderGraphView()
	{
		ImGui::SetNextWindowSize(ImVec2(360, 400));
		if (ImGui::BeginPopupModal("Load Render Graph ##Popup"))
		{
			TArray<String> filesInDirectory = EnumerateFilesInDirectory("../Assets/RenderGraphs/", true);
			TArray<const char*> renderGraphFilesInDirectory;

			for (auto fileIt = filesInDirectory.begin(); fileIt != filesInDirectory.end(); fileIt++)
			{
				const String& filename = *fileIt;
				
				if (filename.find(".lrg") != String::npos)
				{
					renderGraphFilesInDirectory.push_back(filename.c_str());
				}
			}

			static int32 selectedIndex = 0;
			static bool loadSucceded = true;

			if (selectedIndex >= renderGraphFilesInDirectory.size()) selectedIndex = renderGraphFilesInDirectory.size() - 1;
			ImGui::ListBox("##Render Graph Files", &selectedIndex, renderGraphFilesInDirectory.data(), (int32)renderGraphFilesInDirectory.size());

			bool done = false;

			if (!loadSucceded)
			{
				ImGui::Text("Loading Failed!");
			}

			if (ImGui::Button("Close"))
			{
				done = true;
			}

			ImGui::SameLine();

			if (ImGui::Button("Load"))
			{
				loadSucceded = LoadFromFile(("../Assets/RenderGraphs/" + filesInDirectory[selectedIndex]));
				done = loadSucceded;
				m_ParsedGraphDirty = loadSucceded;
			}

			if (done)
			{
				selectedIndex	= 0;
				loadSucceded	= true;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void RenderGraphEditor::RenderParsedRenderGraphView()
	{
		imnodes::BeginNodeEditor();

		ImGui::GetWindowDrawList()->Flags &= ~ImDrawListFlags_AntiAliasedLines; //Disable this since otherwise link thickness does not work

		static String textBuffer0;
		static String textBuffer1;
		textBuffer0.resize(1024);
		textBuffer1.resize(1024);

		int32 currentAttributeIndex = INT32_MAX;

		static TArray<std::tuple<int32, int32, int32>> links;
		links.clear();
		links.reserve(m_ParsedRenderGraphStructure.PipelineStages.size());

		//Resource State Groups
		for (auto pipelineStageIt = m_ParsedRenderGraphStructure.PipelineStages.begin(); pipelineStageIt != m_ParsedRenderGraphStructure.PipelineStages.end(); pipelineStageIt++)
		{
			int32 distance	= std::distance(m_ParsedRenderGraphStructure.PipelineStages.begin(), pipelineStageIt);
			int32 nodeIndex	= INT32_MAX - distance;
			const PipelineStageDesc* pPipelineStage = &(*pipelineStageIt);

			imnodes::BeginNode(nodeIndex);

			if (pPipelineStage->Type == EPipelineStageType::RENDER)
			{
				const EditorParsedRenderStage* pRenderStage = &m_ParsedRenderGraphStructure.RenderStages[pPipelineStage->StageIndex];

				String renderStageType = RenderStageTypeToString(pRenderStage->Type);

				imnodes::BeginNodeTitleBar();
				ImGui::Text("Render Stage");
				ImGui::Text("%s : [%s]", pRenderStage->Name.c_str(), renderStageType.c_str());
				ImGui::Text("RS: %d PS: %d", pPipelineStage->StageIndex, distance);
				ImGui::Text("Weight: %d", pRenderStage->Weight);
				imnodes::EndNodeTitleBar();

				if (ImGui::BeginChild(("##" + std::to_string(pPipelineStage->StageIndex) + " Child").c_str(), ImVec2(220.0f, 220.0f)))
				{
					for (auto resourceStateNameIt = pRenderStage->ResourceStateNames.begin(); resourceStateNameIt != pRenderStage->ResourceStateNames.end(); resourceStateNameIt++)
					{
						const EditorResource* pResource = &m_ResourcesByName[*resourceStateNameIt];
						textBuffer0 = "";
						textBuffer1 = "";

						textBuffer0 += pResource->Name.c_str();
						textBuffer1 += "Type: " + RenderGraphResourceTypeToString(pResource->Type);
						textBuffer1 += "\n";
						textBuffer1 += "Sub Resource Type: " + RenderGraphSubResourceTypeToString(pResource->SubResourceType);

						if (pResource->SubResourceType == EEditorSubResourceType::ARRAY)
						{
							textBuffer1 += "\n";
							textBuffer1 += "Sub Resource Array Count: " + pResource->SubResourceArrayCount;
						}

						if (pResource->Type == EEditorResourceType::TEXTURE)
						{
							textBuffer1 += "\n";
							textBuffer1 += "Texture Format: " + TextureFormatToString(pResource->TextureFormat);
						}
						ImVec2 textSize = ImGui::CalcTextSize((textBuffer0 + textBuffer1 + "\n\n\n\n").c_str());

						if (ImGui::BeginChild(("##" + std::to_string(pPipelineStage->StageIndex) + pResource->Name + " Child").c_str(), ImVec2(0.0f, textSize.y)))
						{
							ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), textBuffer0.c_str());
							ImGui::TextWrapped(textBuffer1.c_str());
						}

						ImGui::EndChild();
					}
				}
				ImGui::EndChild();

				imnodes::BeginInputAttribute(currentAttributeIndex--);
				ImGui::InvisibleButton("##Resouce State Invisible Input Button", ImGui::CalcTextSize("-"));
				imnodes::EndInputAttribute();

				ImGui::SameLine();

				imnodes::BeginOutputAttribute(currentAttributeIndex--);
				ImGui::InvisibleButton("##Resouce State Invisible Output Button", ImGui::CalcTextSize("-"));
				imnodes::EndOutputAttribute();
			}
			else if (pPipelineStage->Type == EPipelineStageType::SYNCHRONIZATION)
			{
				const EditorSynchronizationStage* pSynchronizationStage = &m_ParsedRenderGraphStructure.SynchronizationStages[pPipelineStage->StageIndex];

				imnodes::BeginNodeTitleBar();
				ImGui::Text("Synchronization Stage");
				ImGui::Text("RS: %d PS: %d", pPipelineStage->StageIndex, distance);
				imnodes::EndNodeTitleBar();

				if (ImGui::BeginChild(("##" + std::to_string(pPipelineStage->StageIndex) + " Child").c_str(), ImVec2(160.0f, 220.0f)))
				{
					for (auto synchronizationIt = pSynchronizationStage->Synchronizations.begin(); synchronizationIt != pSynchronizationStage->Synchronizations.end(); synchronizationIt++)
					{
						const EditorResourceSynchronization* pSynchronization = &(*synchronizationIt);
						const EditorResource* pResource = &pSynchronization->Resource;
						textBuffer0 = "";
						textBuffer1 = "";

						textBuffer0 += pResource->Name.c_str();
						textBuffer1 += "\n";
						textBuffer1 += CommandQueueToString(pSynchronization->FromQueue) + " -> " + CommandQueueToString(pSynchronization->ToQueue);
						textBuffer1 += "\n";
						textBuffer1 += ResourceAccessStateToString(pSynchronization->FromState) + " -> " + ResourceAccessStateToString(pSynchronization->ToState);
						ImVec2 textSize = ImGui::CalcTextSize((textBuffer0 + textBuffer1 + "\n\n\n\n").c_str());

						if (ImGui::BeginChild(("##" + std::to_string(pPipelineStage->StageIndex) + pResource->Name + " Child").c_str(), ImVec2(0.0f, textSize.y)))
						{
							ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), textBuffer0.c_str());
							ImGui::TextWrapped(textBuffer1.c_str());
						}

						ImGui::EndChild();
					}
				}

				ImGui::EndChild();

				imnodes::BeginInputAttribute(currentAttributeIndex--);
				ImGui::InvisibleButton("##Resouce State Invisible Input Button", ImGui::CalcTextSize("-"));
				imnodes::EndInputAttribute();

				ImGui::SameLine();

				imnodes::BeginOutputAttribute(currentAttributeIndex--);
				ImGui::InvisibleButton("##Resouce State Invisible Output Button", ImGui::CalcTextSize("-"));
				imnodes::EndOutputAttribute();
			}

			if (pipelineStageIt != m_ParsedRenderGraphStructure.PipelineStages.begin())
			{
				links.push_back(std::make_tuple(nodeIndex, currentAttributeIndex + 3, currentAttributeIndex + 2));
			}

			imnodes::EndNode();
		}

		for (auto linkIt = links.begin(); linkIt != links.end(); linkIt++)
		{
			imnodes::Link(std::get<0>(*linkIt), std::get<1>(*linkIt), std::get<2>(*linkIt));
		}

		imnodes::EndNodeEditor();
	}

	void RenderGraphEditor::RenderShaderBoxes(EditorRenderStage* pRenderStage)
	{
		if (pRenderStage->Type == EPipelineStateType::GRAPHICS)
		{
			if (pRenderStage->GraphicsShaders.VertexShader.size()	== 0 &&
				pRenderStage->GraphicsShaders.GeometryShader.size() == 0 &&
				pRenderStage->GraphicsShaders.HullShader.size()		== 0 &&
				pRenderStage->GraphicsShaders.DomainShader.size()	== 0)
			{
				ImGui::PushID("##Task Shader ID");
				ImGui::Button(pRenderStage->GraphicsShaders.TaskShader.size() == 0 ? "Task Shader" : pRenderStage->GraphicsShaders.TaskShader.c_str());
				RenderShaderBoxCommon(&pRenderStage->GraphicsShaders.TaskShader);
				ImGui::PopID();
				
				ImGui::PushID("##Mesh Shader ID");
				ImGui::Button(pRenderStage->GraphicsShaders.MeshShader.size() == 0 ? "Mesh Shader" : pRenderStage->GraphicsShaders.MeshShader.c_str());
				RenderShaderBoxCommon(&pRenderStage->GraphicsShaders.MeshShader);
				ImGui::PopID();
			}

			if (pRenderStage->GraphicsShaders.TaskShader.size()	== 0 &&
				pRenderStage->GraphicsShaders.MeshShader.size() == 0)
			{
				ImGui::PushID("##Vertex Shader ID");
				ImGui::Button(pRenderStage->GraphicsShaders.VertexShader.size() == 0 ? "Vertex Shader" : pRenderStage->GraphicsShaders.VertexShader.c_str());
				RenderShaderBoxCommon(&pRenderStage->GraphicsShaders.VertexShader);
				ImGui::PopID();
				
				ImGui::PushID("##Geometry Shader ID");
				ImGui::Button(pRenderStage->GraphicsShaders.GeometryShader.size() == 0 ? "Geometry Shader" : pRenderStage->GraphicsShaders.GeometryShader.c_str());
				RenderShaderBoxCommon(&pRenderStage->GraphicsShaders.GeometryShader);
				ImGui::PopID();

				ImGui::PushID("##Hull Shader ID");
				ImGui::Button(pRenderStage->GraphicsShaders.HullShader.size() == 0 ? "Hull Shader" : pRenderStage->GraphicsShaders.HullShader.c_str());
				RenderShaderBoxCommon(&pRenderStage->GraphicsShaders.HullShader);
				ImGui::PopID();

				ImGui::PushID("##Domain Shader ID");
				ImGui::Button(pRenderStage->GraphicsShaders.DomainShader.size() == 0 ? "Domain Shader" : pRenderStage->GraphicsShaders.DomainShader.c_str());
				RenderShaderBoxCommon(&pRenderStage->GraphicsShaders.DomainShader);
				ImGui::PopID();
			}

			ImGui::PushID("##Pixel Shader ID");
			ImGui::Button(pRenderStage->GraphicsShaders.PixelShader.size() == 0 ? "Pixel Shader" : pRenderStage->GraphicsShaders.PixelShader.c_str());
			RenderShaderBoxCommon(&pRenderStage->GraphicsShaders.PixelShader);
			ImGui::PopID();
		}
		else if (pRenderStage->Type == EPipelineStateType::COMPUTE)
		{
			ImGui::PushID("##Compute Shader ID");
			ImGui::Button(pRenderStage->ComputeShaders.Shader.size() == 0 ? "Shader" : pRenderStage->ComputeShaders.Shader.c_str());
			RenderShaderBoxCommon(&pRenderStage->ComputeShaders.Shader);
			ImGui::PopID();
		}
		else if (pRenderStage->Type == EPipelineStateType::RAY_TRACING)
		{
			ImGui::PushID("##Raygen Shader ID");
			ImGui::Button(pRenderStage->RayTracingShaders.RaygenShader.size() == 0 ? "Raygen Shader" : pRenderStage->RayTracingShaders.RaygenShader.c_str());
			RenderShaderBoxCommon(&pRenderStage->RayTracingShaders.RaygenShader);
			ImGui::PopID();

			uint32 missBoxesCount = glm::min(pRenderStage->RayTracingShaders.MissShaderCount + 1, MAX_MISS_SHADER_COUNT);
			for (uint32 m = 0; m < missBoxesCount; m++)
			{
				bool added = false;
				bool removed = false;

				ImGui::PushID(m);
				ImGui::Button(pRenderStage->RayTracingShaders.pMissShaders[m].size() == 0 ? "Miss Shader" : pRenderStage->RayTracingShaders.pMissShaders[m].c_str());
				RenderShaderBoxCommon(&(pRenderStage->RayTracingShaders.pMissShaders[m]), &added, &removed);
				ImGui::PopID();

				if (added) 
					pRenderStage->RayTracingShaders.MissShaderCount++;

				if (removed)
				{
					for (uint32 m2 = m; m2 < missBoxesCount - 1; m2++)
					{
						pRenderStage->RayTracingShaders.pMissShaders[m2] = pRenderStage->RayTracingShaders.pMissShaders[m2 + 1];
					}

					pRenderStage->RayTracingShaders.MissShaderCount--;
					missBoxesCount--;
				}
			}

			uint32 closestHitBoxesCount = glm::min(pRenderStage->RayTracingShaders.ClosestHitShaderCount + 1, MAX_CLOSEST_HIT_SHADER_COUNT);
			for (uint32 ch = 0; ch < closestHitBoxesCount; ch++)
			{
				bool added = false;
				bool removed = false;

				ImGui::PushID(ch);
				ImGui::Button(pRenderStage->RayTracingShaders.pClosestHitShaders[ch].size() == 0 ? "Closest Hit Shader" : pRenderStage->RayTracingShaders.pClosestHitShaders[ch].c_str());
				RenderShaderBoxCommon(&pRenderStage->RayTracingShaders.pClosestHitShaders[ch], &added, &removed);
				ImGui::PopID();

				if (added)
					pRenderStage->RayTracingShaders.ClosestHitShaderCount++;

				if (removed)
				{
					for (uint32 ch2 = ch; ch2 < missBoxesCount - 1; ch2++)
					{
						pRenderStage->RayTracingShaders.pClosestHitShaders[ch2] = pRenderStage->RayTracingShaders.pClosestHitShaders[ch2 + 1];
					}

					pRenderStage->RayTracingShaders.ClosestHitShaderCount--;
					closestHitBoxesCount--;
				}
			}
		}
	}

	void RenderGraphEditor::RenderShaderBoxCommon(String* pTarget, bool* pAdded, bool* pRemoved)
	{
		if (ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* pPayload = ImGui::AcceptDragDropPayload("SHADER");

			if (pPayload != nullptr)
			{
				String* pShaderName = *reinterpret_cast<String**>(pPayload->Data);
				if (pAdded != nullptr && pTarget->size() == 0) (*pAdded) = true;
				(*pTarget) = *pShaderName;
			}

			ImGui::EndDragDropTarget();
		}

		if (pTarget->size() > 0)
		{
			ImGui::SameLine();

			if (ImGui::Button("-"))
			{
				(*pTarget) = "";
				if (pRemoved != nullptr) (*pRemoved) = true;
			}
		}
	}

	int32 RenderGraphEditor::CreateResourceState(const String& resourceName, const String& renderStageName, bool removable)
	{
		EditorRenderGraphResourceState renderGraphInputResource = {};
		renderGraphInputResource.ResourceName		= resourceName;
		renderGraphInputResource.RenderStageName	= renderStageName;
		renderGraphInputResource.Removable			= removable;

		uint32 attributeIndex = s_NextAttributeID;
		s_NextAttributeID += 2;

		m_ResourceStatesByHalfAttributeIndex[attributeIndex / 2]			= renderGraphInputResource;
		return attributeIndex;
	}

	bool RenderGraphEditor::CheckLinkValid(int32* pSrcAttributeIndex, int32* pDstAttributeIndex)
	{
		int32 src = *pSrcAttributeIndex;
		int32 dst = *pDstAttributeIndex;
		
		if (src % 2 == 1 && dst % 2 == 0)
		{
			if (dst + 1 == src)
				return false;
			
			return true;
		}
		else if (src % 2 == 0 && dst % 2 == 1)
		{
			if (src + 1 == dst)
				return false;

			(*pSrcAttributeIndex) = dst;
			(*pDstAttributeIndex) = src;
			return true;
		}

		return false;
	}

	String RenderGraphEditor::RenderStageTypeToString(EPipelineStateType type)
	{
		switch (type)
		{
		case EPipelineStateType::GRAPHICS:						return "GRAPHICS";
		case EPipelineStateType::COMPUTE:						return "COMPUTE";
		case EPipelineStateType::RAY_TRACING:					return "RAY_TRACING";
		default:												return "NONE";
		}
	}

	String RenderGraphEditor::RenderGraphResourceTypeToString(EEditorResourceType type)
	{
		switch (type)
		{
		case EEditorResourceType::TEXTURE:						return "TEXTURE";
		case EEditorResourceType::BUFFER:						return "BUFFER";
		case EEditorResourceType::ACCELERATION_STRUCTURE:		return "ACCELERATION_STRUCTURE";
		default:												return "NONE";
		}
	}

	String RenderGraphEditor::RenderGraphSubResourceTypeToString(EEditorSubResourceType type)
	{
		switch (type)
		{
		case EEditorSubResourceType::ARRAY:						return "ARRAY";
		case EEditorSubResourceType::PER_FRAME:					return "PER_FRAME";
		default:												return "NONE";
		}
	}

	EPipelineStateType RenderGraphEditor::RenderStageTypeFromString(const String& string)
	{
		if		(string == "GRAPHICS")		return EPipelineStateType::GRAPHICS;
		else if (string == "COMPUTE")		return EPipelineStateType::COMPUTE;
		else if (string == "RAY_TRACING")	return EPipelineStateType::RAY_TRACING;

		return EPipelineStateType::NONE;
	}

	EEditorResourceType RenderGraphEditor::RenderGraphResourceTypeFromString(const String& string)
	{
		if		(string == "TEXTURE")					return EEditorResourceType::TEXTURE;
		else if (string == "BUFFER")					return EEditorResourceType::BUFFER;
		else if (string == "ACCELERATION_STRUCTURE")	return EEditorResourceType::ACCELERATION_STRUCTURE;

		return EEditorResourceType::NONE;
	}

	EEditorSubResourceType RenderGraphEditor::RenderGraphSubResourceTypeFromString(const String& string)
	{
		if		(string == "BUFFER")	return EEditorSubResourceType::ARRAY;
		else if (string == "PER_FRAME")	return EEditorSubResourceType::PER_FRAME;

		return EEditorSubResourceType::NONE;
	}

	void RenderGraphEditor::DestroyLink(int32 linkIndex)
	{
		if (linkIndex >= 0)
		{
			EditorRenderGraphResourceLink* pLinkToBeDestroyed = &m_ResourceStateLinksByLinkIndex[linkIndex];

			EditorRenderGraphResourceState* pSrcResource = &m_ResourceStatesByHalfAttributeIndex[pLinkToBeDestroyed->SrcAttributeIndex / 2];
			EditorRenderGraphResourceState* pDstResource = &m_ResourceStatesByHalfAttributeIndex[pLinkToBeDestroyed->SrcAttributeIndex / 2];

			m_ResourceStateLinksByLinkIndex.erase(linkIndex);

			pDstResource->InputLinkIndex = -1;
			pSrcResource->OutputLinkIndices.erase(linkIndex);
			m_ParsedGraphDirty = true;
		}
	}

	void RenderGraphEditor::PushPinColorIfNeeded(EEditorPinType pinType, EditorRenderStage* pRenderStage, EditorRenderGraphResourceState* pResourceState, int32 targetAttributeIndex)
	{
		if (CustomPinColorNeeded(pinType, pRenderStage, pResourceState, targetAttributeIndex))
		{
			imnodes::PushColorStyle(imnodes::ColorStyle_Pin,		HOVERED_COLOR);
			imnodes::PushColorStyle(imnodes::ColorStyle_PinHovered, HOVERED_COLOR);
		}
	}

	void RenderGraphEditor::PopPinColorIfNeeded(EEditorPinType pinType, EditorRenderStage* pRenderStage, EditorRenderGraphResourceState* pResourceState, int32 targetAttributeIndex)
	{
		if (CustomPinColorNeeded(pinType, pRenderStage, pResourceState, targetAttributeIndex))
		{
			imnodes::PopColorStyle();
			imnodes::PopColorStyle();
		}
	}

	bool RenderGraphEditor::CustomPinColorNeeded(EEditorPinType pinType, EditorRenderStage* pRenderStage, EditorRenderGraphResourceState* pResourceState, int32 targetAttributeIndex)
	{
		if (m_StartedLinkInfo.LinkStarted)
		{
			if (pResourceState == nullptr)
			{
				//Started on Output Pin and target is RENDER_STAGE_INPUT Pin
				if (!m_StartedLinkInfo.LinkStartedOnInputPin && pinType == EEditorPinType::RENDER_STAGE_INPUT)
				{
					return true;
				}
			}
			else if (m_StartedLinkInfo.LinkStartedOnResource == pResourceState->ResourceName || m_StartedLinkInfo.LinkStartedOnResource.size() == 0)
			{
				if (!m_StartedLinkInfo.LinkStartedOnInputPin)
				{
					if (pinType == EEditorPinType::INPUT && (targetAttributeIndex + 1) != m_StartedLinkInfo.LinkStartAttributeID)
					{
						return true;
					}
				}
				else
				{
					if (pinType == EEditorPinType::OUTPUT && (m_StartedLinkInfo.LinkStartAttributeID + 1) != targetAttributeIndex)
					{
						return true;
					}
				}
			}
		}

		return false;
	}

	bool RenderGraphEditor::SaveToFile(const String& renderGraphName)
	{
		using namespace rapidjson;

		StringBuffer jsonStringBuffer;
		PrettyWriter<StringBuffer> writer(jsonStringBuffer);

		writer.StartObject();

		//Resources
		{
			writer.String("resources");
			writer.StartArray();
			{
				for (auto resourceIt = m_ResourcesByName.begin(); resourceIt != m_ResourcesByName.end(); resourceIt++)
				{
					const EditorResource* pResource = &resourceIt->second;

					writer.StartObject();
					{
						writer.String("name");
						writer.String(pResource->Name.c_str());

						writer.String("type");
						writer.String(RenderGraphResourceTypeToString(pResource->Type).c_str());

						writer.String("sub_resource_type");
						writer.String(RenderGraphSubResourceTypeToString(pResource->SubResourceType).c_str());

						writer.String("sub_resource_array_count");
						writer.Uint(pResource->SubResourceArrayCount);

						writer.String("editable");
						writer.Bool(pResource->Editable);

						writer.String("texture_format");
						writer.String(TextureFormatToString(pResource->TextureFormat).c_str());
					}
					writer.EndObject();
				}
			}
			writer.EndArray();
		}

		//Resource State Group
		{
			writer.String("resource_state_groups");
			writer.StartArray();
			{
				for (auto resourceStateGroupIt = m_ResourceStateGroups.begin(); resourceStateGroupIt != m_ResourceStateGroups.end(); resourceStateGroupIt++)
				{
					EditorResourceStateGroup* pResourceStateGroup = &(*resourceStateGroupIt);

					writer.StartObject();
					{
						writer.String("name");
						writer.String(pResourceStateGroup->Name.c_str());

						writer.String("node_index");
						writer.Int(pResourceStateGroup->NodeIndex);

						writer.String("resource_states");
						writer.StartArray();
						{
							for (auto resourceStateIt = pResourceStateGroup->ResourceStates.begin(); resourceStateIt != pResourceStateGroup->ResourceStates.end(); resourceStateIt++)
							{
								int32 attributeIndex = resourceStateIt->second;
								EditorRenderGraphResourceState* pResourceState = &m_ResourceStatesByHalfAttributeIndex[attributeIndex / 2];

								writer.StartObject();
								{
									writer.String("name");
									writer.String(pResourceState->ResourceName.c_str());

									writer.String("render_stage_name");
									writer.String(pResourceState->RenderStageName.c_str());

									writer.String("removable");
									writer.Bool(pResourceState->Removable);

									writer.String("attribute_index");
									writer.Int(attributeIndex);

									writer.String("input_link_index");
									writer.Int(pResourceState->InputLinkIndex);

									writer.String("output_link_indices");
									writer.StartArray();
									{
										for (auto outputLinkIt = pResourceState->OutputLinkIndices.begin(); outputLinkIt != pResourceState->OutputLinkIndices.end(); outputLinkIt++)
										{
											int32 outputLinkIndex = *outputLinkIt;

											writer.Int(outputLinkIndex);
										}
									}
									writer.EndArray();
								}
								writer.EndObject();
							}
						}
						writer.EndArray();
					}
					writer.EndObject();
				}
			}
			writer.EndArray();
		}

		//Final Output Stage
		{
			writer.String("final_output_stage");
			writer.StartObject();
			{
				writer.String("name");
				writer.String(m_FinalOutput.Name.c_str());

				writer.String("node_index");
				writer.Int(m_FinalOutput.NodeIndex);

				writer.String("back_buffer_state");
				writer.StartObject();
				{
					int32 attributeIndex = m_FinalOutput.BackBufferAttributeIndex;
					EditorRenderGraphResourceState* pResourceState = &m_ResourceStatesByHalfAttributeIndex[attributeIndex / 2];

					writer.String("name");
					writer.String(pResourceState->ResourceName.c_str());

					writer.String("render_stage_name");
					writer.String(pResourceState->RenderStageName.c_str());

					writer.String("removable");
					writer.Bool(pResourceState->Removable);

					writer.String("attribute_index");
					writer.Int(attributeIndex);

					writer.String("input_link_index");
					writer.Int(pResourceState->InputLinkIndex);

					writer.String("output_link_indices");
					writer.StartArray();
					{
						for (auto outputLinkIt = pResourceState->OutputLinkIndices.begin(); outputLinkIt != pResourceState->OutputLinkIndices.end(); outputLinkIt++)
						{
							int32 outputLinkIndex = *outputLinkIt;

							writer.Int(outputLinkIndex);
						}
					}
					writer.EndArray();
				}
				writer.EndObject();
			}
			writer.EndObject();
		}

		//Render Stages
		{
			writer.String("render_stages");
			writer.StartArray();
			{
				for (auto renderStageIt = m_RenderStagesByName.begin(); renderStageIt != m_RenderStagesByName.end(); renderStageIt++)
				{
					EditorRenderStage* pRenderStage = &renderStageIt->second;

					writer.StartObject();
					{
						writer.String("name");
						writer.String(pRenderStage->Name.c_str());

						writer.String("node_index");
						writer.Int(pRenderStage->NodeIndex);

						writer.String("input_attribute_index");
						writer.Int(pRenderStage->InputAttributeIndex);

						writer.String("type");
						writer.String(RenderStageTypeToString(pRenderStage->Type).c_str());

						writer.String("custom_renderer");
						writer.Bool(pRenderStage->CustomRenderer);

						writer.String("shaders");
						writer.StartObject();
						{
							if (pRenderStage->Type == EPipelineStateType::GRAPHICS)
							{
								writer.String("task_shader");
								writer.String(pRenderStage->GraphicsShaders.TaskShader.c_str());

								writer.String("mesh_shader");
								writer.String(pRenderStage->GraphicsShaders.MeshShader.c_str());

								writer.String("vertex_shader");
								writer.String(pRenderStage->GraphicsShaders.VertexShader.c_str());

								writer.String("geometry_shader");
								writer.String(pRenderStage->GraphicsShaders.GeometryShader.c_str());

								writer.String("hull_shader");
								writer.String(pRenderStage->GraphicsShaders.HullShader.c_str());

								writer.String("domain_shader");
								writer.String(pRenderStage->GraphicsShaders.DomainShader.c_str());

								writer.String("pixel_shader");
								writer.String(pRenderStage->GraphicsShaders.PixelShader.c_str());
							}
							else if (pRenderStage->Type == EPipelineStateType::COMPUTE)
							{
								writer.String("shader");
								writer.String(pRenderStage->ComputeShaders.Shader.c_str());
							}
							else if (pRenderStage->Type == EPipelineStateType::RAY_TRACING)
							{
								writer.String("raygen_shader");
								writer.String(pRenderStage->RayTracingShaders.RaygenShader.c_str());

								writer.String("miss_shaders");
								writer.StartArray();
								for (uint32 m = 0; m < pRenderStage->RayTracingShaders.MissShaderCount; m++)
								{
									writer.String(pRenderStage->RayTracingShaders.pMissShaders[m].c_str());
								}
								writer.EndArray();

								writer.String("closest_hit_shaders");
								writer.StartArray();
								for (uint32 ch = 0; ch < pRenderStage->RayTracingShaders.ClosestHitShaderCount; ch++)
								{
									writer.String(pRenderStage->RayTracingShaders.pClosestHitShaders[ch].c_str());
								}
								writer.EndArray();
							}
						}
						writer.EndObject();

						writer.String("resource_states");
						writer.StartArray();
						{
							for (auto resourceStateIt = pRenderStage->ResourceStates.begin(); resourceStateIt != pRenderStage->ResourceStates.end(); resourceStateIt++)
							{
								int32 attributeIndex = resourceStateIt->second;
								EditorRenderGraphResourceState* pResourceState = &m_ResourceStatesByHalfAttributeIndex[attributeIndex / 2];

								writer.StartObject();
								{
									writer.String("name");
									writer.String(pResourceState->ResourceName.c_str());

									writer.String("render_stage_name");
									writer.String(pResourceState->RenderStageName.c_str());

									writer.String("removable");
									writer.Bool(pResourceState->Removable);

									writer.String("attribute_index");
									writer.Int(attributeIndex);

									writer.String("input_link_index");
									writer.Int(pResourceState->InputLinkIndex);

									writer.String("output_link_indices");
									writer.StartArray();
									{
										for (auto outputLinkIt = pResourceState->OutputLinkIndices.begin(); outputLinkIt != pResourceState->OutputLinkIndices.end(); outputLinkIt++)
										{
											int32 outputLinkIndex = *outputLinkIt;

											writer.Int(outputLinkIndex);
										}
									}
									writer.EndArray();
								}
								writer.EndObject();
							}
						}
						writer.EndArray();
					}
					writer.EndObject();
				}
			}
			writer.EndArray();
		}


		//Links
		{
			writer.String("links");
			writer.StartArray();
			{
				for (auto linkIt = m_ResourceStateLinksByLinkIndex.begin(); linkIt != m_ResourceStateLinksByLinkIndex.end(); linkIt++)
				{
					EditorRenderGraphResourceLink* pLink = &linkIt->second;

					writer.StartObject();
					{
						writer.String("link_index");
						writer.Int(pLink->LinkIndex);

						writer.String("src_attribute_index");
						writer.Int(pLink->SrcAttributeIndex);

						writer.String("dst_attribute_index");
						writer.Int(pLink->DstAttributeIndex);
					}
					writer.EndObject();
				}
			}
			writer.EndArray();
		}

		writer.EndObject();

		FILE* pFile = fopen(("../Assets/RenderGraphs/" + renderGraphName + ".lrg").c_str(), "w");

		if (pFile != nullptr)
		{
			fputs(jsonStringBuffer.GetString(), pFile);
			fclose(pFile);
			return true;
		}
		
		return false;
	}

	bool RenderGraphEditor::LoadFromFile(const String& filepath)
	{
		using namespace rapidjson;

		FILE* pFile = fopen(filepath.c_str(), "r");

		if (pFile == nullptr)
			return false;

		char readBuffer[65536];
		FileReadStream inputStream(pFile, readBuffer, sizeof(readBuffer));

		Document d;
		d.ParseStream(inputStream);

		int32 largestNodeID			= 0;
		int32 largestAttributeID	= 0;
		int32 largestLinkID			= 0;

		std::vector<EditorResourceStateGroup>				loadedResourceStateGroups;
		EditorFinalOutput									loadedFinalOutput		= {};

		tsl::ordered_map<String, EditorResource>			loadedResourcesByName;
		THashTable<int32, String>							loadedRenderStageNameByInputAttributeIndex;
		THashTable<String, EditorRenderStage>				loadedRenderStagesByName;
		THashTable<int32, EditorRenderGraphResourceState>	loadedResourceStatesByHalfAttributeIndex;
		THashTable<int32, EditorRenderGraphResourceLink>	loadedResourceStateLinks;

		//Load Resources
		if (d.HasMember("resources"))
		{
			if (d["resources"].IsArray())
			{
				GenericArray resourceArray = d["resources"].GetArray();

				for (uint32 r = 0; r < resourceArray.Size(); r++)
				{
					GenericObject resourceObject = resourceArray[r].GetObject();
					EditorResource resource = {};
					resource.Name					= resourceObject["name"].GetString();
					resource.Type					= RenderGraphResourceTypeFromString(resourceObject["type"].GetString());
					resource.SubResourceType		= RenderGraphSubResourceTypeFromString(resourceObject["sub_resource_type"].GetString());
					resource.SubResourceArrayCount	= resourceObject["sub_resource_array_count"].GetUint();
					resource.TextureFormat			= TextureFormatFromString(resourceObject["texture_format"].GetString());
					resource.Editable				= resourceObject["editable"].GetBool();

					loadedResourcesByName[resource.Name] = resource;
				}
			}
			else
			{
				LOG_ERROR("[RenderGraphEditor]: \"resources\" member wrong type!");
				return false;
			}
		}
		else
		{
			LOG_ERROR("[RenderGraphEditor]: \"resources\" member could not be found!");
			return false;
		}

		//Load Resource State Groups
		if (d.HasMember("resource_state_groups"))
		{
			if (d["resource_state_groups"].IsArray())
			{
				GenericArray resourceStateGroupsArray = d["resource_state_groups"].GetArray();

				for (uint32 rsg = 0; rsg < resourceStateGroupsArray.Size(); rsg++)
				{
					GenericObject resourceStateGroupObject = resourceStateGroupsArray[rsg].GetObject();
					EditorResourceStateGroup resourceStateGroup = {};
					resourceStateGroup.Name			= resourceStateGroupObject["name"].GetString();
					resourceStateGroup.NodeIndex	= resourceStateGroupObject["node_index"].GetInt();

					GenericArray resourceStateArray = resourceStateGroupObject["resource_states"].GetArray();

					for (uint32 r = 0; r < resourceStateArray.Size(); r++)
					{
						GenericObject resourceStateObject = resourceStateArray[r].GetObject();

						String resourceName		= resourceStateObject["name"].GetString();
						int32 attributeIndex	= resourceStateObject["attribute_index"].GetInt();

						EditorRenderGraphResourceState resourceState = {};
						resourceState.ResourceName		= resourceName;
						resourceState.RenderStageName	= resourceStateObject["render_stage_name"].GetString();
						resourceState.Removable			= resourceStateObject["removable"].GetBool();
						resourceState.InputLinkIndex	= resourceStateObject["input_link_index"].GetInt();

						GenericArray outputLinkIndicesArray = resourceStateObject["output_link_indices"].GetArray();

						for (uint32 ol = 0; ol < outputLinkIndicesArray.Size(); ol++)
						{
							resourceState.OutputLinkIndices.insert(outputLinkIndicesArray[ol].GetInt());
						}

						loadedResourceStatesByHalfAttributeIndex[attributeIndex / 2] = resourceState;
						resourceStateGroup.ResourceStates[resourceName] = attributeIndex;

						if (attributeIndex + 1 > largestAttributeID) largestAttributeID = attributeIndex + 1;
					}

					loadedResourceStateGroups.push_back(resourceStateGroup);

					if (loadedFinalOutput.NodeIndex > largestNodeID) largestNodeID = loadedFinalOutput.NodeIndex;
				}
			}
			else
			{
				LOG_ERROR("[RenderGraphEditor]: \"external_resources_stage\" member wrong type!");
				return false;
			}
		}
		else
		{
			LOG_ERROR("[RenderGraphEditor]: \"external_resources_stage\" member could not be found!");
			return false;
		}

		//Load Final Output Stage
		if (d.HasMember("final_output_stage"))
		{
			if (d["final_output_stage"].IsObject())
			{
				GenericObject finalOutputStageObject = d["final_output_stage"].GetObject();

				loadedFinalOutput.Name		= finalOutputStageObject["name"].GetString();
				loadedFinalOutput.NodeIndex = finalOutputStageObject["node_index"].GetInt();

				GenericObject resourceStateObject = finalOutputStageObject["back_buffer_state"].GetObject();

				String resourceName		= resourceStateObject["name"].GetString();
				int32 attributeIndex	= resourceStateObject["attribute_index"].GetInt();

				EditorRenderGraphResourceState resourceState = {};
				resourceState.ResourceName		= resourceName;
				resourceState.RenderStageName	= resourceStateObject["render_stage_name"].GetString();
				resourceState.Removable			= resourceStateObject["removable"].GetBool();
				resourceState.InputLinkIndex	= resourceStateObject["input_link_index"].GetInt();

				GenericArray outputLinkIndicesArray = resourceStateObject["output_link_indices"].GetArray();

				for (uint32 ol = 0; ol < outputLinkIndicesArray.Size(); ol++)
				{
					resourceState.OutputLinkIndices.insert(outputLinkIndicesArray[ol].GetInt());
				}

				loadedResourceStatesByHalfAttributeIndex[attributeIndex / 2] = resourceState;
				loadedFinalOutput.BackBufferAttributeIndex = attributeIndex;

				if (attributeIndex + 1 > largestAttributeID) largestAttributeID = attributeIndex + 1;

				if (loadedFinalOutput.NodeIndex > largestNodeID) largestNodeID = loadedFinalOutput.NodeIndex;
			}
			else
			{
				LOG_ERROR("[RenderGraphEditor]: \"external_resources_stage\" member wrong type!");
				return false;
			}
		}
		else
		{
			LOG_ERROR("[RenderGraphEditor]: \"external_resources_stage\" member could not be found!");
			return false;
		}

		//Load Render Stages and Render Stage Resource States
		if (d.HasMember("render_stages"))
		{
			if (d["render_stages"].IsArray())
			{
				GenericArray renderStageArray = d["render_stages"].GetArray();

				for (uint32 rs = 0; rs < renderStageArray.Size(); rs++)
				{
					GenericObject renderStageObject = renderStageArray[rs].GetObject();
					EditorRenderStage renderStage = {};
					renderStage.Name				= renderStageObject["name"].GetString();
					renderStage.NodeIndex			= renderStageObject["node_index"].GetInt();
					renderStage.InputAttributeIndex = renderStageObject["input_attribute_index"].GetInt();
					renderStage.Type				= RenderStageTypeFromString(renderStageObject["type"].GetString());
					renderStage.CustomRenderer		= renderStageObject["custom_renderer"].GetBool();

					GenericObject shadersObject		= renderStageObject["shaders"].GetObject();
					GenericArray resourceStateArray = renderStageObject["resource_states"].GetArray();

					if (renderStage.Type == EPipelineStateType::GRAPHICS)
					{
						renderStage.GraphicsShaders.TaskShader		= shadersObject["task_shader"].GetString();
						renderStage.GraphicsShaders.MeshShader		= shadersObject["mesh_shader"].GetString();
						renderStage.GraphicsShaders.VertexShader	= shadersObject["vertex_shader"].GetString();
						renderStage.GraphicsShaders.GeometryShader	= shadersObject["geometry_shader"].GetString();
						renderStage.GraphicsShaders.HullShader		= shadersObject["hull_shader"].GetString();
						renderStage.GraphicsShaders.DomainShader	= shadersObject["domain_shader"].GetString();
						renderStage.GraphicsShaders.PixelShader		= shadersObject["pixel_shader"].GetString();
					}
					else if (renderStage.Type == EPipelineStateType::COMPUTE)
					{
						renderStage.ComputeShaders.Shader			= shadersObject["shader"].GetString();
					}
					else if (renderStage.Type == EPipelineStateType::RAY_TRACING)
					{
						renderStage.RayTracingShaders.RaygenShader	= shadersObject["raygen_shader"].GetString();

						GenericArray missShadersArray = shadersObject["miss_shaders"].GetArray();

						for (uint32 m = 0; m < missShadersArray.Size(); m++)
						{
							renderStage.RayTracingShaders.pMissShaders[m] = missShadersArray[m].GetString();
						}

						renderStage.RayTracingShaders.MissShaderCount = missShadersArray.Size();

						GenericArray closestHitShadersArray = shadersObject["closest_hit_shaders"].GetArray();

						for (uint32 ch = 0; ch < closestHitShadersArray.Size(); ch++)
						{
							renderStage.RayTracingShaders.pClosestHitShaders[ch] = closestHitShadersArray[ch].GetString();
						}

						renderStage.RayTracingShaders.ClosestHitShaderCount = closestHitShadersArray.Size();
					}

					for (uint32 r = 0; r < resourceStateArray.Size(); r++)
					{
						GenericObject resourceStateObject = resourceStateArray[r].GetObject();

						String resourceName		= resourceStateObject["name"].GetString();
						int32 attributeIndex	= resourceStateObject["attribute_index"].GetInt();

						EditorRenderGraphResourceState resourceState = {};
						resourceState.ResourceName		= resourceName;
						resourceState.RenderStageName	= resourceStateObject["render_stage_name"].GetString();
						resourceState.Removable			= resourceStateObject["removable"].GetBool();
						resourceState.InputLinkIndex	= resourceStateObject["input_link_index"].GetInt();
						
						GenericArray outputLinkIndicesArray = resourceStateObject["output_link_indices"].GetArray();

						for (uint32 ol = 0; ol < outputLinkIndicesArray.Size(); ol++)
						{
							resourceState.OutputLinkIndices.insert(outputLinkIndicesArray[ol].GetInt());
						}

						loadedResourceStatesByHalfAttributeIndex[attributeIndex / 2] = resourceState;
						renderStage.ResourceStates[resourceName] = attributeIndex;

						if (attributeIndex + 1 > largestAttributeID) largestAttributeID = attributeIndex + 1;
					}

					loadedRenderStagesByName[renderStage.Name] = renderStage;
					loadedRenderStageNameByInputAttributeIndex[renderStage.InputAttributeIndex] = renderStage.Name;

					if (renderStage.InputAttributeIndex > largestNodeID) largestNodeID = renderStage.InputAttributeIndex;
				}
			}
			else
			{
				LOG_ERROR("[RenderGraphEditor]: \"render_stages\" member wrong type!");
				return false;
			}
		}
		else
		{
			LOG_ERROR("[RenderGraphEditor]: \"render_stages\" member could not be found!");
			return false;
		}

		//Load Resouce State Links
		if (d.HasMember("links"))
		{
			if (d["links"].IsArray())
			{
				GenericArray linkArray = d["links"].GetArray();

				for (uint32 l = 0; l < linkArray.Size(); l++)
				{
					GenericObject linkObject = linkArray[l].GetObject();
					EditorRenderGraphResourceLink link = {};

					link.LinkIndex			= linkObject["link_index"].GetInt();
					link.SrcAttributeIndex	= linkObject["src_attribute_index"].GetInt();
					link.DstAttributeIndex	= linkObject["dst_attribute_index"].GetInt();

					loadedResourceStateLinks[link.LinkIndex] = link;

					if (link.LinkIndex > largestLinkID) largestLinkID = link.LinkIndex;
				}
			}
			else
			{
				LOG_ERROR("[RenderGraphEditor]: \"links\" member wrong type!");
				return false;
			}
		}
		else
		{
			LOG_ERROR("[RenderGraphEditor]: \"links\" member could not be found!");
			return false;
		}

		fclose(pFile);

		//Reset to clear state
		{
			m_ResourceStateGroups.clear();
			m_FinalOutput		= {};

			m_ResourcesByName.clear();
			m_RenderStageNameByInputAttributeIndex.clear();
			m_RenderStagesByName.clear();
			m_ResourceStatesByHalfAttributeIndex.clear();
			m_ResourceStateLinksByLinkIndex.clear();

			m_CurrentlyAddingRenderStage	= EPipelineStateType::NONE;
			m_CurrentlyAddingResource		= EEditorResourceType::NONE;

			m_StartedLinkInfo				= {};
		}

		//Set Loaded State
		{
			s_NextNodeID		= largestNodeID + 1;
			s_NextAttributeID	= largestAttributeID + 1;
			s_NextLinkID		= largestLinkID + 1;

			m_ResourceStateGroups	= loadedResourceStateGroups;
			m_FinalOutput			= loadedFinalOutput;

			m_ResourcesByName						= loadedResourcesByName;
			m_RenderStageNameByInputAttributeIndex	= loadedRenderStageNameByInputAttributeIndex;
			m_RenderStagesByName					= loadedRenderStagesByName;
			m_ResourceStatesByHalfAttributeIndex	= loadedResourceStatesByHalfAttributeIndex;
			m_ResourceStateLinksByLinkIndex			= loadedResourceStateLinks;
		}

		return true;
	}

	bool RenderGraphEditor::ParseStructure(bool generateImGuiStage)
	{
		const EditorRenderGraphResourceState* pBackBufferFinalState = &m_ResourceStatesByHalfAttributeIndex[m_FinalOutput.BackBufferAttributeIndex / 2];

		if (pBackBufferFinalState->InputLinkIndex == -1)
		{
			m_ParsingError = "No link connected to Final Output";
			return false;
		}

		//Get the Render Stage connected to the Final Output Stage
		const String& lastRenderStageName = m_ResourceStatesByHalfAttributeIndex[m_ResourceStateLinksByLinkIndex[pBackBufferFinalState->InputLinkIndex].SrcAttributeIndex / 2].RenderStageName;

		//Check if the final Render Stage actually is a Render Stage and not a Resource State Group
		if (!IsRenderStage(lastRenderStageName))
		{
			m_ParsingError = "A valid render stage must be linked to " + m_FinalOutput.Name;
			return false;
		}

		//Reset Render Stage Weight
		for (auto renderStageIt = m_RenderStagesByName.begin(); renderStageIt != m_RenderStagesByName.end(); renderStageIt++)
		{
			EditorRenderStage* pCurrentRenderStage = &renderStageIt->second;
			pCurrentRenderStage->Weight = 0;
		}

		//Weight Render Stages
		for (auto renderStageIt = m_RenderStagesByName.begin(); renderStageIt != m_RenderStagesByName.end(); renderStageIt++)
		{
			EditorRenderStage* pCurrentRenderStage = &renderStageIt->second;

			if (pCurrentRenderStage->Enabled)
			{
				if (!RecursivelyWeightParentRenderStages(pCurrentRenderStage))
				{
					return false;
				}
			}
		}

		//Created sorted Map of Render Stages
		std::multimap<uint32, EditorRenderStage*> orderedMappedRenderStages;
		for (auto renderStageIt = m_RenderStagesByName.begin(); renderStageIt != m_RenderStagesByName.end(); renderStageIt++)
		{
			EditorRenderStage* pCurrentRenderStage = &renderStageIt->second;

			if (pCurrentRenderStage->Enabled)
			{
				orderedMappedRenderStages.insert({ pCurrentRenderStage->Weight, pCurrentRenderStage });
			}
		}

		////Push External Resources Initial State to Hash Table
		//THashTable<String, TArray<EditorRenderGraphResourceState>> externalResourceStates;
		//const EditorResourceStateGroup* pExternalResourceStateGroup = &m_ResourceStateGroups[EXTERNAL_RESOURCE_STATE_GROUP_INDEX];
		//for (auto resourceStateIt = pExternalResourceStateGroup->ResourceStates.begin(); resourceStateIt != pExternalResourceStateGroup->ResourceStates.end(); resourceStateIt++)
		//{
		//	const EditorRenderGraphResourceState* pResourceState = &m_ResourceStatesByHalfAttributeIndex[resourceStateIt->second / 2];
		//	externalResourceStates[pResourceState->ResourceName].push_back(*pResourceState);
		//}

		////Push Internal Resources Initial State to Hash Table
		//THashTable<String, TArray<EditorRenderGraphResourceState>> internalResourceStates;
		//const EditorResourceStateGroup* pTemporalResourceStateGroup = &m_ResourceStateGroups[TEMPORAL_RESOURCE_STATE_GROUP_INDEX];
		//for (auto resourceStateIt = pTemporalResourceStateGroup->ResourceStates.begin(); resourceStateIt != pTemporalResourceStateGroup->ResourceStates.end(); resourceStateIt++)
		//{
		//	const EditorRenderGraphResourceState* pResourceState = &m_ResourceStatesByHalfAttributeIndex[resourceStateIt->second / 2];
		//	internalResourceStates[pResourceState->ResourceName].push_back(*pResourceState);
		//}

		//------------------------------------------------------------------------------------------Create Initial Synchronization Stage for RenderPass attachments?

		//Loop Through each Render Stage in Order and create synchronization stages
		TArray<EditorParsedRenderStage>		orderedRenderStages;
		TArray<EditorSynchronizationStage>	orderedSynchronizationStages;
		TArray<PipelineStageDesc>			orderedPipelineStages;

		orderedRenderStages.reserve(orderedMappedRenderStages.size());
		orderedSynchronizationStages.reserve(orderedMappedRenderStages.size());
		orderedPipelineStages.reserve(2 * orderedMappedRenderStages.size());

		THashTable<String, const EditorRenderGraphResourceState*> finalStateOfResources;

		EditorParsedRenderStage imguiRenderStage = {};

		if (generateImGuiStage)
		{
			imguiRenderStage.Name				= RENDER_GRAPH_IMGUI_STAGE_NAME;
			imguiRenderStage.Type				= EPipelineStateType::GRAPHICS;
			imguiRenderStage.CustomRenderer		= true;
			imguiRenderStage.Enabled			= true;
		}

		for (auto orderedRenderStageIt = orderedMappedRenderStages.rbegin(); orderedRenderStageIt != orderedMappedRenderStages.rend(); orderedRenderStageIt++)
		{
			EditorRenderStage* pCurrentRenderStage = orderedRenderStageIt->second;

			if (pCurrentRenderStage->Enabled)
			{
				EditorSynchronizationStage synchronizationStage = {};

				//Loop through each Resource State in the Render Stage
				for (auto resourceStateIndexIt = pCurrentRenderStage->ResourceStates.begin(); resourceStateIndexIt != pCurrentRenderStage->ResourceStates.end(); resourceStateIndexIt++)
				{
					const EditorRenderGraphResourceState* pCurrentResourceState = &m_ResourceStatesByHalfAttributeIndex[resourceStateIndexIt->second / 2];
					const EditorRenderGraphResourceState* pNextResourceState = nullptr;
					const EditorRenderStage* pNextRenderStage = nullptr;

					//Loop through the following Render Stages and find the first one that uses this Resource'
					auto nextOrderedRenderStageIt = orderedRenderStageIt;
					nextOrderedRenderStageIt++;
					for (; nextOrderedRenderStageIt != orderedMappedRenderStages.rend(); nextOrderedRenderStageIt++)
					{
						const EditorRenderStage* pPotentialNextRenderStage = nextOrderedRenderStageIt->second;
						auto potentialNextResourceStateIt = pPotentialNextRenderStage->ResourceStates.find(pCurrentResourceState->ResourceName);

						//See if this Render Stage uses Resource we are looking for
						if (pPotentialNextRenderStage->Enabled)
						{
							if (potentialNextResourceStateIt != pPotentialNextRenderStage->ResourceStates.end())
							{
								pNextResourceState = &m_ResourceStatesByHalfAttributeIndex[potentialNextResourceStateIt->second / 2];
								pNextRenderStage = pPotentialNextRenderStage;
								break;
							}
						}
					}

					//If there is a Next State for the Resource, pNextResourceState will not be nullptr 
					EditorResourceSynchronization resourceSynchronization = {};
					bool isBackBuffer = pCurrentResourceState->ResourceName == RENDER_GRAPH_BACK_BUFFER_ATTACHMENT;

					if (pNextResourceState != nullptr)
					{
						//Check if pNextResourceState belongs to a Render Stage, otherwise we need to check if it belongs to Final Output
						if (pNextRenderStage != nullptr)
						{
							resourceSynchronization.FromState	= FindAccessStateFromResourceState(pCurrentResourceState);
							resourceSynchronization.ToState		= FindAccessStateFromResourceState(pNextResourceState);
							resourceSynchronization.FromQueue	= ConvertPipelineStateTypeToQueue(pCurrentRenderStage->Type);
							resourceSynchronization.ToQueue		= ConvertPipelineStateTypeToQueue(pNextRenderStage->Type);
							resourceSynchronization.Resource	= m_ResourcesByName[pCurrentResourceState->ResourceName];
						}

						synchronizationStage.Synchronizations.push_back(resourceSynchronization);
					}
					else if (generateImGuiStage)
					{
						const EditorResource* pResource = &m_ResourcesByName[pCurrentResourceState->ResourceName];

						if (pResource->Type == EEditorResourceType::TEXTURE && pResource->SubResourceType == EEditorSubResourceType::PER_FRAME)
						{
							resourceSynchronization.FromState	= FindAccessStateFromResourceState(pCurrentResourceState);
							resourceSynchronization.ToState		= isBackBuffer ? EResourceAccessState::WRITE : EResourceAccessState::READ;
							resourceSynchronization.FromQueue	= ConvertPipelineStateTypeToQueue(pCurrentRenderStage->Type);
							resourceSynchronization.ToQueue		= ECommandQueueType::COMMAND_QUEUE_GRAPHICS;
							resourceSynchronization.Resource	= *pResource;

							synchronizationStage.Synchronizations.push_back(resourceSynchronization);
						}
					}
					else if (isBackBuffer && pNextResourceState->RenderStageName == m_FinalOutput.Name)
					{
						resourceSynchronization.FromState		= FindAccessStateFromResourceState(pCurrentResourceState);
						resourceSynchronization.ToState			= EResourceAccessState::PRESENT;
						resourceSynchronization.FromQueue		= ConvertPipelineStateTypeToQueue(pCurrentRenderStage->Type);
						resourceSynchronization.ToQueue			= ECommandQueueType::COMMAND_QUEUE_GRAPHICS;
						resourceSynchronization.Resource		= m_ResourcesByName[pCurrentResourceState->ResourceName];

						synchronizationStage.Synchronizations.push_back(resourceSynchronization);
					}

					finalStateOfResources[pCurrentResourceState->ResourceName] = pCurrentResourceState;
				}

				EditorParsedRenderStage parsedRenderStage = {};
				CreateParsedRenderStage(&parsedRenderStage, pCurrentRenderStage);

				orderedRenderStages.push_back(parsedRenderStage);
				orderedPipelineStages.push_back({ EPipelineStageType::RENDER, uint32(orderedRenderStages.size()) - 1 });

				if (synchronizationStage.Synchronizations.size() > 0)
				{
					orderedSynchronizationStages.push_back(synchronizationStage);
					orderedPipelineStages.push_back({ EPipelineStageType::SYNCHRONIZATION, uint32(orderedSynchronizationStages.size()) - 1 });
				}
			}
		}

		if (generateImGuiStage)
		{
			EditorSynchronizationStage imguiSynchronizationStage = {};

			for (auto resourceStateIt = finalStateOfResources.begin(); resourceStateIt != finalStateOfResources.end(); resourceStateIt++)
			{
				const EditorRenderGraphResourceState* pFinalResourceState = resourceStateIt->second;

				const EditorResource* pResource = &m_ResourcesByName[pFinalResourceState->ResourceName];

				if (pResource->Type == EEditorResourceType::TEXTURE && pResource->SubResourceType == EEditorSubResourceType::PER_FRAME)
				{
					imguiRenderStage.ResourceStateNames.push_back(pFinalResourceState->ResourceName);

					if (pFinalResourceState->ResourceName == RENDER_GRAPH_BACK_BUFFER_ATTACHMENT)
					{
						EditorResourceSynchronization resourceSynchronization = {};
						resourceSynchronization.FromState = FindAccessStateFromResourceState(pFinalResourceState);
						resourceSynchronization.ToState = EResourceAccessState::PRESENT;
						resourceSynchronization.FromQueue = ECommandQueueType::COMMAND_QUEUE_GRAPHICS;
						resourceSynchronization.ToQueue = ECommandQueueType::COMMAND_QUEUE_GRAPHICS;
						resourceSynchronization.Resource = m_ResourcesByName[pFinalResourceState->ResourceName];

						imguiSynchronizationStage.Synchronizations.push_back(resourceSynchronization);
					}
				}				
			}

			orderedRenderStages.push_back(imguiRenderStage);
			orderedPipelineStages.push_back({ EPipelineStageType::RENDER, uint32(orderedRenderStages.size()) - 1 });

			orderedSynchronizationStages.push_back(imguiSynchronizationStage);
			orderedPipelineStages.push_back({ EPipelineStageType::SYNCHRONIZATION, uint32(orderedSynchronizationStages.size()) - 1 });
		}

		m_ParsedRenderGraphStructure.RenderStages			= orderedRenderStages;
		m_ParsedRenderGraphStructure.SynchronizationStages	= orderedSynchronizationStages;
		m_ParsedRenderGraphStructure.PipelineStages			= orderedPipelineStages;

		return true;
	}

	bool RenderGraphEditor::RecursivelyWeightParentRenderStages(EditorRenderStage* pChildRenderStage)
	{
		std::set<String> parentRenderStageNames;

		//Iterate through all resource states in the current Render Stages
		for (auto resourceStateIt = pChildRenderStage->ResourceStates.begin(); resourceStateIt != pChildRenderStage->ResourceStates.end(); resourceStateIt++)
		{
			const EditorRenderGraphResourceState* pResourceState = &m_ResourceStatesByHalfAttributeIndex[resourceStateIt->second / 2];

			//Check if resource state has input link
			if (pResourceState->InputLinkIndex != -1)
			{
				const String& parentRenderStageName = m_ResourceStatesByHalfAttributeIndex[m_ResourceStateLinksByLinkIndex[pResourceState->InputLinkIndex].SrcAttributeIndex / 2].RenderStageName;

				//Make sure parent is a Render Stage and check if it has already been visited
				if (IsRenderStage(parentRenderStageName) && parentRenderStageNames.count(parentRenderStageName) == 0)
				{
					EditorRenderStage* pParentRenderStage = &m_RenderStagesByName[parentRenderStageName];
					RecursivelyWeightParentRenderStages(pParentRenderStage);

					parentRenderStageNames.insert(parentRenderStageName);
					pParentRenderStage->Weight++;
				}
			}
		}

		return true;
	}

	bool RenderGraphEditor::IsRenderStage(const String& name)
	{
		return m_RenderStagesByName.count(name) > 0;
	}

	EResourceAccessState RenderGraphEditor::FindAccessStateFromResourceState(const EditorRenderGraphResourceState* pResourceState)
	{
		return pResourceState->OutputLinkIndices.size() > 0 ? EResourceAccessState::WRITE : EResourceAccessState::READ;
	}

	void RenderGraphEditor::CreateParsedRenderStage(EditorParsedRenderStage* pDstRenderStage, const EditorRenderStage* pSrcRenderStage)
	{
		pDstRenderStage->Name					= pSrcRenderStage->Name;
		pDstRenderStage->Type					= pSrcRenderStage->Type;
		pDstRenderStage->CustomRenderer			= pSrcRenderStage->CustomRenderer;
		pDstRenderStage->Enabled				= pSrcRenderStage->Enabled;
		pDstRenderStage->GraphicsShaders		= pSrcRenderStage->GraphicsShaders;
		pDstRenderStage->ComputeShaders			= pSrcRenderStage->ComputeShaders;
		pDstRenderStage->RayTracingShaders		= pSrcRenderStage->RayTracingShaders;
		pDstRenderStage->Weight					= pSrcRenderStage->Weight;
		pDstRenderStage->ResourceStateNames.reserve(pSrcRenderStage->ResourceStates.size());

		for (auto resourceStateIndexIt = pSrcRenderStage->ResourceStates.begin(); resourceStateIndexIt != pSrcRenderStage->ResourceStates.end(); resourceStateIndexIt++)
		{
			const EditorRenderGraphResourceState* pResourceState = &m_ResourceStatesByHalfAttributeIndex[resourceStateIndexIt->second / 2];
			pDstRenderStage->ResourceStateNames.push_back(pResourceState->ResourceName);
		}
	}

}