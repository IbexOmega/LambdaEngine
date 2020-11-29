#pragma once
#include "Audio/API/IAudioDevice.h"

#include "Containers/TArray.h"
#include "Containers/THashTable.h"

#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/Texture.h"
#include "Rendering/Core/API/Shader.h"

#include "Audio/API/ISoundEffect3D.h"
#include "Audio/API/ISoundEffect2D.h"
#include "Audio/API/IMusic.h"

#include "Game/ECS/Components/Rendering/MeshComponent.h"

#include "Material.h"
#include "Mesh.h"

namespace glslang
{
	class TIntermediate;
}

struct aiMesh;
struct aiNode;
struct aiScene;
struct aiAnimation;

namespace LambdaEngine
{
	class GLSLShaderSource;

	struct LevelObjectOnLoadDesc
	{
		String	Prefix		= "";
	};

	struct LevelObjectOnLoad
	{
		String		Prefix		= "";
		String		Name		= "";
		TArray<BoundingBox>		BoundingBoxes;
		TArray<MeshComponent>	MeshComponents;
		glm::vec3				DefaultPosition;
		glm::quat				DefaultRotation;
		glm::vec3				DefaultScale;
	};

	struct LoadedDirectionalLight
	{
		String		Name;
		glm::vec4	ColorIntensity	= glm::vec4(0.0f);
		glm::vec3	Direction		= glm::vec3(0.0f);
	};

	struct LoadedPointLight
	{
		String		Name;
		glm::vec4	ColorIntensity	= glm::vec4(0.0f);
		glm::vec3	Position		= glm::vec3(0.0f);
		glm::vec3	Attenuation		= glm::vec3(1.0f, 0.09f, 0.032f);
	};

	/*	SceneLoadRequest contains information needed to begin loading a scene. It is also used to specify whether to
		skip loading optional resources by setting fields to nullptr. */
	struct SceneLoadRequest
	{
		String									Filepath;
		int32									AssimpFlags;
		const TArray<LevelObjectOnLoadDesc>&	LevelObjectDescriptions;
		TArray<LoadedDirectionalLight>&			DirectionalLights;
		TArray<LoadedPointLight>&				PointLights;
		TArray<LevelObjectOnLoad>&				LevelObjects;
		TArray<Mesh*>&							Meshes;
		TArray<Animation*>*						pAnimations;
		TArray<MeshComponent>&					MeshComponents;
		// Either both materials and textures are nullptr, or they are both non-null pointers
		TArray<LoadedMaterial*>*	pMaterials;
		TArray<LoadedTexture*>*		pTextures;
		bool						AnimationsOnly;
		bool						ShouldTessellate;
	};

	// SceneLoadingContext is internally created from a SceneLoadRequest.
	struct SceneLoadingContext
	{
		String									Filename;
		String									DirectoryPath;
		const TArray<LevelObjectOnLoadDesc>&	LevelObjectDescriptions;
		TArray<LoadedDirectionalLight>&			DirectionalLights;
		TArray<LoadedPointLight>&				PointLights;
		TArray<LevelObjectOnLoad>&				LevelObjects;
		TArray<Mesh*>&							Meshes;
		TArray<MeshComponent>&					MeshComponents;
		TArray<Animation*>*						pAnimations;
		TArray<LoadedMaterial*>*				pMaterials;
		TArray<LoadedTexture*>*					pTextures;
		THashTable<String, LoadedTexture*>		LoadedTextures;
		THashTable<uint32, uint32>				MaterialIndices;
		bool									ShouldTessellate;
	};

	class LAMBDA_API ResourceLoader
	{
	public:
		static bool Init();
		static bool Release();

		/*
		* Load a Scene from file, (experimental, only tested with Sponza Scene)
		*	filepath				- Path to the file
		*	loadedMeshComponents	- A vector where all loaded MeshComponent(s) will be stored, the
										GUIDs of each MeshComponent is an index to the loadedMeshes and loadedMaterials vectors
		*	loadedMeshes			- A vector where all loaded Mesh(es) will be stored
		*	loadedMaterials			- A vector where all loaded Material(s) will be stored
		*	loadedTextures			- A vector where all loaded Texture(s) will be stored
		* return - true if the scene was loaded, false otherwise
		*/
		static bool LoadSceneFromFile(
			const String& filepath,
			const TArray<LevelObjectOnLoadDesc>& levelObjectDescriptions,
			TArray<MeshComponent>& meshComponents,
			TArray<LoadedDirectionalLight>& directionalLights,
			TArray<LoadedPointLight>& pointLights,
			TArray<LevelObjectOnLoad>& levelObjects,
			TArray<Mesh*>& meshes,
			TArray<Animation*>& animations,
			TArray<LoadedMaterial*>& materials,
			TArray<LoadedTexture*>& textures);

		/*
		* Load a mesh from file
		*	filepath	- Path to the file
		*	animations	- The animations in this file
		* return - a Mesh* if the mesh was loaded, otherwise nullptr will be returned
		*/
		static Mesh* LoadMeshFromFile(
			const String& filepath, 
			TArray<LoadedMaterial*>* pMaterials, 
			TArray<LoadedTexture*>* pTextures, 
			TArray<Animation*>* pAnimations, 
			int32 assimpFlags, 
			bool shouldTessellate);

		/*
		* Load a mesh from file
		*	filepath	- Path to the file
		* return - a TArray filled with Animation* if the file was loaded, otherwise an empty TArray will be returned
		*/
		static TArray<Animation*> LoadAnimationsFromFile(const String& filepath);

		/*
		* Load a mesh from memory
		*	pVertices	- An array of vertices
		*	numVertices	- The vertexcount
		*	pIndices	- An array of indices
		*	numIndices	- The Indexcount
		* return - a Mesh* if the mesh was loaded, otherwise nullptr will be returned
		*/
		static Mesh* LoadMeshFromMemory(
			const Vertex* pVertices, 
			uint32 numVertices, 
			const uint32* pIndices, 
			uint32 numIndices);

		/*
		* Load a texture from file
		*	name			- Name of the texture
		*	dir				- A root directory to be appended to entries of pFilepaths
		*	pFilenames		- Names of the texture files
		*	count			- Number of elements in pFilenames
		*	format			- The format of the pixeldata
		*	generateMips	- If mipmaps should be generated on load
		* return - an Texture* if the texture was loaded, otherwise nullptr will be returned
		*/
		static Texture* LoadTextureArrayFromFile(
			const String& name, 
			const String& dir, 
			const String* pFilenames, 
			uint32 count, 
			EFormat format, 
			bool generateMips, 
			bool linearFilteringMips);

		/*
		* Load a texturecube from a equirectangular panorama file
		*	name			- Name of the texture
		*	dir				- A root directory to be appended to entries of pFilepaths
		*	pFilenames		- Names of the texture files
		*	format			- The format of the pixeldata
		*	generateMips	- If mipmaps should be generated on load
		* return - an Texture* if the texture was loaded, otherwise nullptr will be returned
		*/
		static Texture* LoadTextureCubeFromPanoramaFile(
			const String& name,
			const String& dir,
			const String& filenames,
			uint32 size,
			EFormat format,
			bool generateMips);

		/*
		* Load multiple Cube textures from file and combine into Texture Arrays along with TextureViews and CubeTextureViews
		*	name - A Name of given to the cubeTexture
		*	pFilenames - Names of the texture files
		*	count - number of cubeTextures to load
		*	format - The format of the pixeldata
		*	generateMips - If mipmaps should be generated on load
		* return - a valid GUID if the texture was loaded, otherwise returns GUID_NONE
		*/
		static Texture* LoadCubeTexturesArrayFromFile(
			const String& name, 
			const String& dir, 
			const String* pFilenames, 
			uint32 count, 
			EFormat format, 
			bool generateMips, 
			bool linearFilteringMips);

		/*
		* Load a texture from memory
		*	name			- Name of the texture
		*	ppData			- An array of pixeldata
		*	arrayCount		- Number of elements in ppData
		*	width			- The pixel width of the texture
		*	height			- The pixel height of the texture
		*	format			- The format of the pixeldata
		*	usageFlags		- Usage flags
		*	generateMips	- If mipmaps should be generated on load
		* return - an Texture* if the texture was loaded, otherwise nullptr will be returned
		*/
		static Texture* LoadTextureArrayFromMemory(
			const String& name, 
			const void* const * ppData, 
			uint32 arrayCount, 
			uint32 width, 
			uint32 height, 
			EFormat format, 
			uint32 usageFlags, 
			bool generateMips, 
			bool linearFilteringMips);

		/*
		* Load sound from file
		*	filepath	- Path to the shader file
		*	stage		- Which stage the shader belongs to
		*	lang		- The language of the shader file
		*	EntryPoint	- The name of the shader entrypoint
		* return - an Shader* if the shader was loaded, otherwise nullptr will be returned
		*/
		static Shader* LoadShaderFromFile(
			const String& filepath, 
			FShaderStageFlag stage, 
			EShaderLang lang, 
			const String& entryPoint = "main");

		static bool CreateShaderReflection(
			const String& filepath, 
			FShaderStageFlag stage, 
			EShaderLang lang, 
			ShaderReflection* pReflection);

		/*
		* Load sound from a source string
		*	source		- Shader source
		*	name		- Optional Shadername
		*	stage		- Which stage the shader belongs to
		*	lang		- The language of the shader file
		*	EntryPoint	- The name of the shader entrypoint
		* return - an Shader* if the shader was loaded, otherwise nullptr will be returned
		*/
		static Shader* LoadShaderFromMemory(
			const String& source, 
			const String& name, 
			FShaderStageFlag stage, 
			EShaderLang lang, 
			const String& entryPoint = "main");

		static GLSLShaderSource LoadShaderSourceFromFile(
			const String& filepath, 
			FShaderStageFlag stage,
			const String& entryPoint = "main");

		/*
		* Load 3D sound from file
		*	filepath - Path to the audio file
		* return - an ISoundEffect3D* if the sound was loaded, otherwise nullptr will be returned
		*/
		static ISoundEffect3D* LoadSoundEffect3DFromFile(const String& filepath);

		/*
		* Load 2D sound from file
		*	filepath - Path to the audio file
		* return - an ISoundEffect3D* if the sound was loaded, otherwise nullptr will be returned
		*/
		static ISoundEffect2D* LoadSoundEffect2DFromFile(const String& filepath);

		/*
		* Load Music from file
		*	filepath - Path to the audio file
		* return - an ISoundEffect3D* if the sound was loaded, otherwise nullptr will be returned
		*/
		static IMusic* LoadMusicFromFile(const String& filepath, float32 defaultVolume = 1.0f, float32 defaultPitch = 1.0f);

		static bool ReadDataFromFile(const String& filepath, const char* pMode, byte** ppData, uint32* pDataSize);

	private:
		static bool InitCubemapGen();
		static void ReleaseCubemapGen();

		static void LoadBoundingBox(BoundingBox& boundingBox, const aiMesh* pMeshAI);
		static void LoadVertices(Mesh* pMesh, const aiMesh* pMeshAI);
		static void LoadIndices(Mesh* pMesh, const aiMesh* pMeshAI);
		static void LoadSkeleton(Mesh* pMesh, const aiMesh* pMeshAI, const aiScene* pSceneAI);
		static void LoadMaterial(SceneLoadingContext& context, const aiScene* pSceneAI, const aiMesh* pMeshAI);
		static void LoadAnimation(SceneLoadingContext& context, const aiAnimation* pAnimationAI);
		static bool LoadSceneWithAssimp(SceneLoadRequest& sceneLoadRequest);
		
		static void ProcessAssimpNode(
			SceneLoadingContext& context, 
			const aiNode* pNode, 
			const aiScene* pScene, 
			const void* pParentTransform);

		static bool CompileGLSLToSPIRV(
			const String& filepath,
			const char* pSource, 
			FShaderStageFlags stage, 
			TArray<uint32>* pSourceSPIRV, 
			ShaderReflection* pReflection);
		
		static bool CreateShaderReflection(
			glslang::TIntermediate* pIntermediate, 
			FShaderStageFlags stage, 
			ShaderReflection* pReflection);

	private:
		// Cubemap gen
		static DescriptorHeap*	s_pCubeMapGenDescriptorHeap;
		static DescriptorSet*	s_pCubeMapGenDescriptorSet;
		static PipelineLayout*	s_pCubeMapGenPipelineLayout;
		static PipelineState*	s_pCubeMapGenPipelineState;
		static Shader*			s_pCubeMapGenShader;

		// All resources
		static CommandAllocator*	s_pComputeCommandAllocator;
		static CommandList*			s_pComputeCommandList;
		static Fence*				s_pComputeFence;
		static uint64				s_ComputeSignalValue;
		static CommandAllocator*	s_pCopyCommandAllocator;
		static CommandList*			s_pCopyCommandList;
		static Fence*				s_pCopyFence;
		static uint64				s_CopySignalValue;
	};
}
