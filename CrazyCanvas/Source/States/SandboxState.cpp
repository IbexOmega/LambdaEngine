#include "States/SandboxState.h"

#include "Resources/ResourceManager.h"

#include "Application/API/CommonApplication.h"
#include "Application/API/Events/EventQueue.h"

#include "Audio/AudioAPI.h"
#include "Audio/FMOD/SoundInstance3DFMOD.h"

#include "Debug/Profiler.h"

#include "ECS/Components/Player/Player.h"
#include "ECS/Components/Player/WeaponComponent.h"
#include "ECS/Components/Misc/DestructionComponent.h"
#include "ECS/ECSCore.h"
#include "ECS/Systems/Player/WeaponSystem.h"

#include "Engine/EngineConfig.h"

#include "Game/ECS/Components/Audio/AudibleComponent.h"
#include "Game/ECS/Components/Misc/Components.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Rendering/MeshComponent.h"
#include "Game/ECS/Components/Rendering/AnimationComponent.h"
#include "Game/ECS/Components/Rendering/DirectionalLightComponent.h"
#include "Game/ECS/Components/Rendering/PointLightComponent.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Rendering/MeshPaintComponent.h"
#include "Game/ECS/Components/Rendering/ParticleEmitter.h"
#include "Game/ECS/Components/Rendering/GlobalLightProbeComponent.h"
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Game/ECS/Systems/TrackSystem.h"
#include "Game/ECS/Components/Player/PlayerRelatedComponent.h"
#include "Game/Multiplayer/Client/ClientSystem.h"
#include "Game/GameConsole.h"

#include "Teams/TeamHelper.h"

#include "Input/API/Input.h"

#include "Math/Random.h"

#include "Rendering/Core/API/GraphicsTypes.h"
#include "Rendering/RenderAPI.h"
#include "Rendering/RenderGraph.h"
#include "Rendering/RenderGraphEditor.h"
#include "Rendering/Animation/AnimationGraph.h"
#include "Rendering/EntityMaskManager.h"

#include "Rendering/PipelineStateManager.h"

#include "Math/Random.h"

#include "GUI/Core/GUIApplication.h"

#include "NoesisPCH.h"

#include "World/LevelManager.h"
#include "World/LevelObjectCreator.h"

#include "Match/Match.h"

#include "Multiplayer/Packet/PacketType.h"
#include "Multiplayer/SingleplayerInitializer.h"

#include "Physics/CollisionGroups.h"
#include "Resources/ResourceCatalog.h"

#include <imgui.h>

using namespace LambdaEngine;


SandboxState::SandboxState()
{
	SingleplayerInitializer::Init();
}

SandboxState::~SandboxState()
{
	EventQueue::UnregisterEventHandler<KeyPressedEvent>(EventHandler(this, &SandboxState::OnKeyPressed));

	SAFEDELETE(m_pRenderGraphEditor);

	SingleplayerInitializer::Release();
}

void SandboxState::Init()
{
	ResourceManager::GetMusic(ResourceCatalog::MAIN_MENU_MUSIC_GUID)->Pause();

	// Initialize event handlers
	m_AudioEffectHandler.Init();
	m_MultiplayerClient.InitInternal();

	// Initialize Systems
	TrackSystem::GetInstance().Init();
	m_DestructionSystem.Init();

	EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &SandboxState::OnKeyPressed);

	m_RenderGraphWindow = EngineConfig::GetBoolProperty(EConfigOption::CONFIG_OPTION_SHOW_RENDER_GRAPH);
	m_ShowDemoWindow	= EngineConfig::GetBoolProperty(EConfigOption::CONFIG_OPTION_SHOW_DEMO);
	m_DebuggingWindow	= EngineConfig::GetBoolProperty(EConfigOption::CONFIG_OPTION_DEBUGGING);

	ECSCore* pECS = ECSCore::GetInstance();
	PhysicsSystem* pPhysicsSystem = PhysicsSystem::GetInstance();

	// Load Match
	{
		const LambdaEngine::TArray<LambdaEngine::SHA256Hash>& levelHashes = LevelManager::GetLevelHashes();

		MatchDescription matchDescription =
		{
			.LevelHash = levelHashes[0]
		};

		Match::CreateMatch(&matchDescription);
	}

	// Global Light Probe
	{
		Entity entity = pECS->CreateEntity();
		pECS->AddComponent<GlobalLightProbeComponent>(entity, GlobalLightProbeComponent());
	}

	// Set Team Colors
	{
		TeamHelper::SetTeamColor(1, 3);
		RenderSystem::GetInstance().SetPaintMaskColor(1, TeamHelper::GetTeamColor(1));
	}

	GUID_Lambda sphereMeshGUID;
	ResourceManager::LoadMeshFromFile("sphere.obj", sphereMeshGUID, false);
	const float32 sphereRadius = PhysicsSystem::CalculateSphereRadius(ResourceManager::GetMesh(sphereMeshGUID));

	uint32 gridRadius = 8;

	for (uint32 y = 0; y < gridRadius; y++)
	{
		const float32 roughness = y / float32(gridRadius - 1);

		for (uint32 x = 0; x < gridRadius; x++)
		{
			const float32 metallic = x / float32(gridRadius - 1);

			MaterialProperties materialProperties;
			materialProperties.Albedo		= glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
			materialProperties.Roughness	= roughness;
			materialProperties.Metallic		= metallic;

			MeshComponent sphereMeshComp = {};
			sphereMeshComp.MeshGUID		= sphereMeshGUID;
			sphereMeshComp.MaterialGUID	= ResourceManager::LoadMaterialFromMemory(
				"Default r: " + std::to_string(roughness) + " m: " + std::to_string(metallic),
				GUID_TEXTURE_DEFAULT_COLOR_MAP,
				GUID_TEXTURE_DEFAULT_NORMAL_MAP,
				GUID_TEXTURE_DEFAULT_COLOR_MAP,
				GUID_TEXTURE_DEFAULT_COLOR_MAP,
				GUID_TEXTURE_DEFAULT_COLOR_MAP,
				materialProperties);

			glm::vec3 position(-float32(gridRadius) * 0.5f + x - 5.0f, 4.0f + y, 15.0f);
			glm::vec3 scale(1.0f);

			Entity entity = pECS->CreateEntity();
			pECS->AddComponent<MeshComponent>(entity, sphereMeshComp);
			const CollisionCreateInfo collisionCreateInfo =
			{
				.Entity			= entity,
				.Position		= pECS->AddComponent<PositionComponent>(entity, { true, position }),
				.Scale			= pECS->AddComponent<ScaleComponent>(entity, { true, scale }),
				.Rotation		= pECS->AddComponent<RotationComponent>(entity, { true, glm::identity<glm::quat>() }),
				.Shapes =
				{
					{
						.ShapeType		= EShapeType::SIMULATION,
						.GeometryType	= EGeometryType::SPHERE,
						.GeometryParams	= { .Radius = sphereRadius },
						.CollisionGroup	= FCollisionGroup::COLLISION_GROUP_STATIC,
						.CollisionMask	= ~FCollisionGroup::COLLISION_GROUP_STATIC, // Collide with any non-static object
						.EntityID		= entity,
					},
				},
			};

			const StaticCollisionComponent staticCollisionComponent = pPhysicsSystem->CreateStaticActor(collisionCreateInfo);
			pECS->AddComponent<StaticCollisionComponent>(entity, staticCollisionComponent);

			glm::mat4 transform = glm::translate(glm::identity<glm::mat4>(), position);
			transform *= glm::toMat4(glm::identity<glm::quat>());
			transform = glm::scale(transform, scale);
		}
	}

	// Snow Emitter
	//{
	//	Entity entity = pECS->CreateEntity();
	//	pECS->AddComponent<PositionComponent>(entity, { true, {0.0f, 30.0f, 0.0f } });
	//	pECS->AddComponent<RotationComponent>(entity, { true, glm::rotate<float>(glm::identity<glm::quat>(), 0.f, g_DefaultUp) });
	//	pECS->AddComponent<ParticleEmitterComponent>(entity,
	//		ParticleEmitterComponent{
	//			.SpawnDelay = 0.0007f,
	//			.ParticleCount = 25000,
	//			.EmitterShape = EEmitterShape::PLANE,
	//			.PlaneDimension = 50.0f,
	//			.VelocityRandomness = 0.7f,
	//			.Velocity = 5.0f,
	//			.Acceleration = 0.0f,
	//			.LifeTime = 7.0f,
	//			.RadiusRandomness = 0.5f,
	//			.BeginRadius = 0.3f,
	//			.FrictionFactor = 1.0f,
	//			.AnimationCount = 1,
	//			.FirstAnimationIndex = 6,
	//			.Color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)
	//		}
	//	);
	//}



	// Create dirLight
	//{
	//	DirectionalLightComponent directionalLightComponent =
	//	{
	//		.ColorIntensity = glm::vec4(1.0f, 1.0f, 1.0f, 5.0f),
	//	};

	//	m_DirLight = pECS->CreateEntity();
	//	pECS->AddComponent<PositionComponent>(m_DirLight, { true, glm::vec3(0.f) });
	//	pECS->AddComponent<RotationComponent>(m_DirLight, { true, glm::quatLookAt(glm::normalize(glm::vec3(0.5f, -1.0f, 0.5f)), g_DefaultUp) });
	//	pECS->AddComponent<DirectionalLightComponent>(m_DirLight, directionalLightComponent);

	//	LOG_DEBUG("Created Directional Light");
	//}

	//Preload some resources
	{
		GUID_Lambda meshGUID;
		TArray<GUID_Lambda> animations;
		ResourceManager::LoadMeshFromFile("Robot/Standard Walk.fbx", meshGUID, animations, false);
	}

	if constexpr (IMGUI_ENABLED)
	{
		ImGui::SetCurrentContext(ImGuiRenderer::GetImguiContext());

		m_pRenderGraphEditor = DBG_NEW RenderGraphEditor();
		m_pRenderGraphEditor->InitGUI();	//Must Be called after Renderer is initialized
	}

	ConsoleCommand cmd1;
	cmd1.Init("render_graph", true);
	cmd1.AddArg(Arg::EType::BOOL);
	cmd1.AddDescription("Activate/Deactivate rendergraph window.\n\t'render_graph true'");
	GameConsole::Get().BindCommand(cmd1, [&, this](GameConsole::CallbackInput& input)->void {
		m_RenderGraphWindow = input.Arguments.GetFront().Value.Boolean;
		});

	ConsoleCommand cmd2;
	cmd2.Init("imgui_demo", true);
	cmd2.AddArg(Arg::EType::BOOL);
	cmd2.AddDescription("Activate/Deactivate demo window.\n\t'imgui_demo true'");
	GameConsole::Get().BindCommand(cmd2, [&, this](GameConsole::CallbackInput& input)->void {
		m_ShowDemoWindow = input.Arguments.GetFront().Value.Boolean;
		});

	ConsoleCommand cmd3;
	cmd3.Init("show_debug_window", false);
	cmd3.AddArg(Arg::EType::BOOL);
	cmd3.AddDescription("Activate/Deactivate debugging window.\n\t'show_debug_window true'");
	GameConsole::Get().BindCommand(cmd3, [&, this](GameConsole::CallbackInput& input)->void {
		m_DebuggingWindow = input.Arguments.GetFront().Value.Boolean;
		});

	ConsoleCommand showTextureCMD;
	showTextureCMD.Init("debug_texture", true);
	showTextureCMD.AddArg(Arg::EType::BOOL);
	showTextureCMD.AddFlag("t", Arg::EType::STRING, 6);
	showTextureCMD.AddFlag("ps", Arg::EType::STRING);
	showTextureCMD.AddDescription("Show a texture resource which is used in the RenderGraph.\n\t'Example: debug_texture 1 -t TEXTURE_NAME1 TEXTURE_NAME2 ...'", { {"t", "The textures you want to display. (separated by spaces)"}, {"ps", "Which pixel shader you want to use."} });
	GameConsole::Get().BindCommand(showTextureCMD, [&, this](GameConsole::CallbackInput& input)->void
		{
			m_ShowTextureDebuggingWindow = input.Arguments.GetFront().Value.Boolean;

			auto textureNameIt = input.Flags.find("t");
			auto shaderNameIt = input.Flags.find("ps");

			GUID_Lambda textureDebuggingShaderGUID = shaderNameIt != input.Flags.end() ? ResourceManager::GetShaderGUID(shaderNameIt->second.Arg.Value.String) : GUID_NONE;
			if (textureNameIt != input.Flags.end())
			{
				m_TextureDebuggingNames.Resize(textureNameIt->second.NumUsedArgs);
				for (uint32 i = 0; i < textureNameIt->second.NumUsedArgs; i++)
				{
					m_TextureDebuggingNames[i].ResourceName = textureNameIt->second.Args[i].Value.String;
					m_TextureDebuggingNames[i].PixelShaderGUID = textureDebuggingShaderGUID;
				}
			}
		});

	ConsoleCommand cmdDebugLights;
	cmdDebugLights.Init("debug_light", true);
	cmdDebugLights.AddArg(Arg::EType::BOOL);
	cmdDebugLights.AddDescription("Debugging Directional Light'");
	GameConsole::Get().BindCommand(cmdDebugLights, [&, this](GameConsole::CallbackInput& input)->void {
		m_DirLightDebug = input.Arguments.GetFront().Value.Boolean;
		});

	ConsoleCommand cmdWireframe;
	cmdWireframe.Init("wireframe", false);
	cmdWireframe.AddArg(Arg::EType::BOOL);
	cmdWireframe.AddDescription("Activate/Deactivate wireframe mode\n");
	GameConsole::Get().BindCommand(cmdWireframe, [&, this](GameConsole::CallbackInput& input)->void {
		THashTable<uint64, ManagedGraphicsPipelineStateDesc>& graphicsPipelinesDescs = PipelineStateManager::GetGraphicsPipelineStateDescriptions();

		for (auto& it : graphicsPipelinesDescs)
		{
			ManagedGraphicsPipelineStateDesc& pipelineStateDesc = it.second;
			if (pipelineStateDesc.DebugName == "DEFERRED_GEOMETRY_PASS" || pipelineStateDesc.DebugName == "DEFERRED_GEOMETRY_PASS_MESH_PAINT")
			{
				pipelineStateDesc.RasterizerState.PolygonMode = input.Arguments.GetFront().Value.Boolean ? EPolygonMode::POLYGON_MODE_LINE : EPolygonMode::POLYGON_MODE_FILL;
			}
		}

		PipelineStateRecompileEvent recompileEvent = {};
		EventQueue::SendEvent(recompileEvent);
		});

	SingleplayerInitializer::Setup();
}

void SandboxState::Resume()
{
	// Unpause System

	// Reload Page
}

void SandboxState::Pause()
{
	// Pause System

	// Unload Page
}

void SandboxState::Tick(LambdaEngine::Timestamp delta)
{
	// Update State specfic objects
	m_pRenderGraphEditor->Update();
	LambdaEngine::Profiler::Tick(delta);

	m_MultiplayerClient.TickMainThreadInternal(delta);

	if constexpr (IMGUI_ENABLED)
	{
		RenderImgui();
	}

	// Debugging Emitters
	ECSCore* pECSCore = ECSCore::GetInstance();
	static uint32 indexEmitters = 0;
	static LambdaEngine::Timestamp time;
	const uint32 emitterCount = 10U;
	if (m_DebugEmitters)
	{
		time += delta;
		if (time.AsSeconds() > 0.1f)
		{
			uint32 modIndex = indexEmitters % emitterCount;

			if (indexEmitters < emitterCount)
			{
				Entity e = pECSCore->CreateEntity();
				m_Emitters[modIndex] = e;

				pECSCore->AddComponent<PositionComponent>(e, { true, {0.0f, 2.0f + Random::Float32(-1.0f, 1.0f), -4.f + float(modIndex) } });
				pECSCore->AddComponent<RotationComponent>(e, { true,	GetRotationQuaternion(glm::normalize(glm::vec3(float(modIndex % 2U), 5.0f + float(modIndex % 3U), float(modIndex % 5U)))) });
				pECSCore->AddComponent<ParticleEmitterComponent>(e, ParticleEmitterComponent{
					.OneTime = true,
					.Explosive = 0.9f,
					.SpawnDelay = 0.01f,
					.ParticleCount = 256,
					.Velocity = 1.0f + Random::Float32(-3.f, 3.f),
					.Acceleration = 0.0f,
					.Gravity = Random::Float32(-5.0f, 5.0f),
					.LifeTime = Random::Float32(1.0f, 3.0f),
					.BeginRadius = 0.1f + Random::Float32(0.0f, 0.5f),
					.FirstAnimationIndex = 6,
					.Color = glm::vec4(modIndex % 2U, modIndex % 3U, modIndex % 5U, 1.0f),
				});
			}
			else
			{
				auto& emitterComp = pECSCore->GetComponent<ParticleEmitterComponent>(m_Emitters[modIndex]);

				emitterComp.Explosive = 0.9f + Random::Float32(0.f, 0.1f);
				emitterComp.BeginRadius = 0.1f + Random::Float32(0.0f, 0.5f);
				emitterComp.Velocity = 1.0f + Random::Float32(-3.f, 3.f);
				emitterComp.Gravity = Random::Float32(-5.0f, 5.0f);
				emitterComp.LifeTime = Random::Float32(1.0f, 3.0f);
				emitterComp.Color = glm::vec4(indexEmitters % 2U, indexEmitters % 3U, indexEmitters % 5U, 1.0f),
				emitterComp.Active = true;
			}

			indexEmitters++;
			time = 0;
		}
	}
}

void SandboxState::FixedTick(LambdaEngine::Timestamp delta)
{
	m_MultiplayerClient.FixedTickMainThreadInternal(delta);
}

void SandboxState::OnRenderGraphRecreate(LambdaEngine::RenderGraph* pRenderGraph)
{
	using namespace LambdaEngine;

	Sampler* pNearestSampler				= Sampler::GetNearestSampler();

	GUID_Lambda blueNoiseID = ResourceManager::GetTextureGUID("Blue Noise Texture");

	Texture* pBlueNoiseTexture				= ResourceManager::GetTexture(blueNoiseID);
	TextureView* pBlueNoiseTextureView		= ResourceManager::GetTextureView(blueNoiseID);

	ResourceUpdateDesc blueNoiseUpdateDesc = {};
	blueNoiseUpdateDesc.ResourceName								= "BLUE_NOISE_LUT";
	blueNoiseUpdateDesc.ExternalTextureUpdate.ppTextures			= &pBlueNoiseTexture;
	blueNoiseUpdateDesc.ExternalTextureUpdate.ppTextureViews		= &pBlueNoiseTextureView;
	blueNoiseUpdateDesc.ExternalTextureUpdate.ppSamplers			= &pNearestSampler;

	pRenderGraph->UpdateResource(&blueNoiseUpdateDesc);

	GUID_Lambda cubemapTexID = ResourceManager::GetTextureGUID("Cubemap Texture");

	Texture* pCubeTexture			= ResourceManager::GetTexture(cubemapTexID);
	TextureView* pCubeTextureView	= ResourceManager::GetTextureView(cubemapTexID);

	ResourceUpdateDesc cubeTextureUpdateDesc = {};
	cubeTextureUpdateDesc.ResourceName = "SKYBOX";
	cubeTextureUpdateDesc.ExternalTextureUpdate.ppTextures		= &pCubeTexture;
	cubeTextureUpdateDesc.ExternalTextureUpdate.ppTextureViews	= &pCubeTextureView;
	cubeTextureUpdateDesc.ExternalTextureUpdate.ppSamplers		= &pNearestSampler;

	pRenderGraph->UpdateResource(&cubeTextureUpdateDesc);
}

void SandboxState::RenderImgui()
{
	using namespace LambdaEngine;

	ImGuiRenderer::Get().DrawUI([&]()
	{
		if (m_RenderGraphWindow)
			m_pRenderGraphEditor->RenderGUI();

		if (m_ShowDemoWindow)
			ImGui::ShowDemoWindow();

		if (m_DebuggingWindow)
		{
			Profiler::Render();
		}

		if (m_ShowTextureDebuggingWindow)
		{
			if (ImGui::Begin("Texture Debugging"))
			{
				for (ImGuiTexture& imGuiTexture : m_TextureDebuggingNames)
				{
					ImGui::Image(&imGuiTexture, ImGui::GetWindowSize());
				}
			}

			ImGui::End();
		}

		if (m_DirLightDebug)
		{
			// If you want to use this debug you have to create a directional light entity and store entityID in m_DirLight
			ECSCore* pECSCore = ECSCore::GetInstance();

			auto* pDirLightComps = pECSCore->GetComponentArray<DirectionalLightComponent>();
			auto* pPosComps = pECSCore->GetComponentArray<PositionComponent>();
			auto* pRotComps = pECSCore->GetComponentArray<RotationComponent>();

			if (pDirLightComps->HasComponent(m_DirLight))
			{
				auto& dirLightComp = pDirLightComps->GetData(m_DirLight);
				auto& posComp = pPosComps->GetData(m_DirLight);
				auto& rotComp = pRotComps->GetData(m_DirLight);

				static glm::vec3 mRotation = glm::eulerAngles(rotComp.Quaternion);

				if (ImGui::Begin("DirLight Debugging"))
				{
					ImGui::Text("Position: ");
					ImGui::SameLine();
					ImGui::InputFloat("##X", &posComp.Position.x);
					ImGui::InputFloat("##Y", &posComp.Position.y);
					ImGui::InputFloat("##Z", &posComp.Position.z);

					ImGui::Text("Rotation: ");
					ImGui::SameLine();
					ImGui::InputFloat("##rotX", &mRotation.x);
					ImGui::InputFloat("##rotY", &mRotation.y);
					ImGui::InputFloat("##rotZ", &mRotation.z);

					ImGui::Text("Frustum Width: ");
					ImGui::SameLine();
					ImGui::InputFloat("##Frustum Width", &dirLightComp.FrustumWidth);
					ImGui::Text("Frustum Height: ");
					ImGui::SameLine();
					ImGui::InputFloat("##Frustum Height", &dirLightComp.FrustumHeight);
					ImGui::Text("Z-Near: ");
					ImGui::SameLine();
					ImGui::InputFloat("##Z-Near", &dirLightComp.FrustumZNear);
					ImGui::Text("Z-Far: ");
					ImGui::SameLine();
					ImGui::InputFloat("##Z-Far", &dirLightComp.FrustumZFar);
				}
				ImGui::End();

				rotComp.Quaternion = glm::quatLookAt(glm::normalize(mRotation), g_DefaultUp);
			}
		}
	});
}

bool SandboxState::OnKeyPressed(const LambdaEngine::KeyPressedEvent& event)
{
	using namespace LambdaEngine;

	if (!IsEventOfType<KeyPressedEvent>(event))
	{
		return false;
	}

	if (event.IsRepeat)
	{
		return false;
	}

	static bool geometryAudioActive = true;
	static bool reverbSphereActive = true;

	if (event.Key == EKey::KEY_5)
	{
		EventQueue::SendEvent(ShaderRecompileEvent());
		EventQueue::SendEvent(PipelineStateRecompileEvent());
	}

	if (event.Key == EKey::KEY_DELETE)
	{
		if (!m_Entities.IsEmpty())
		{
			const uint32 numEntities = m_Entities.GetSize();

			static uint32 removeCounter = 0;
			static uint32 removeIndices[] = {
				3,
				2,
				0,
				1,
				0
			};
			const uint32 index = removeIndices[removeCounter++];

			//const uint32 index = Random::UInt32(0, numEntities-1);
			Entity entity = m_Entities[index];
			m_Entities.Erase(m_Entities.Begin() + index);
			ECSCore::GetInstance()->RemoveEntity(entity);

			std::string info = "Removed entity[" + std::to_string(entity) + "] with index [" + std::to_string(index) + "/" + std::to_string(numEntities) + "]!";
			GameConsole::Get().PushInfo(info);
			LOG_INFO(info.c_str());
		}
	}

	// Debugging Lights
	static uint32 indexLights = 0;
	static bool removeLights = true;
	ECSCore* ecsCore = ECSCore::GetInstance();
	if (event.Key == EKey::KEY_9)
	{
		uint32 modIndex = indexLights % 10U;
		if (modIndex == 0U)
			removeLights = !removeLights;

		if (!removeLights)
		{
			Entity e = ecsCore->CreateEntity();
			m_PointLights[modIndex] = e;

			ecsCore->AddComponent<PositionComponent>(e, { true, {0.0f, 2.0f, -5.0f + modIndex} });
			ecsCore->AddComponent<PointLightComponent>(e, PointLightComponent{ .ColorIntensity = {modIndex % 2U, modIndex % 3U, modIndex % 5U, 25.0f} });
		}
		else
		{
			ecsCore->RemoveEntity(m_PointLights[modIndex]);
		}

		indexLights++;
	}

	// Debugging Emitters
	if (event.Key == EKey::KEY_8)
	{
		m_DebugEmitters = !m_DebugEmitters;
	}

	return true;
}
