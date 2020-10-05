#pragma once

#include "RenderGraphTypes.h"

#include "ICustomRenderer.h"

namespace LambdaEngine
{
	class LightRenderer : public ICustomRenderer
	{
	public:
		DECL_REMOVE_COPY(LightRenderer);
		DECL_REMOVE_MOVE(LightRenderer);

		LightRenderer();
		~LightRenderer();

		bool init(uint32 backBufferCount);

		virtual bool RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc) override final;

		virtual void PreBuffersDescriptorSetWrite()		override final;
		virtual void PreTexturesDescriptorSetWrite()	override final;

		virtual void UpdateTextureResource(const String& resourceName, const TextureView* const* ppTextureViews, uint32 count, bool backBufferBound) override final;
		virtual void UpdateBufferResource(const String& resourceName, const Buffer* const* ppBuffers, uint64* pOffsets, uint64* pSizesInBytes, uint32 count, bool backBufferBound) override final;
		virtual void UpdateAccelerationStructureResource(const String& resourceName, const AccelerationStructure* pAccelerationStructure) override final;
		virtual void UpdateDrawArgsResource(const String& resourceName, const DrawArg* pDrawArgs, uint32 count)  override final;

		virtual void Render(
			uint32 modFrameIndex,
			uint32 backBufferIndex,
			CommandList** ppFirstExecutionStage,
			CommandList** ppSecondaryExecutionStage,
			bool Sleeping)	override final;

		FORCEINLINE virtual FPipelineStageFlag GetFirstPipelineStage()	override final { return FPipelineStageFlag::PIPELINE_STAGE_FLAG_VERTEX_INPUT; }
		FORCEINLINE virtual FPipelineStageFlag GetLastPipelineStage()	override final { return FPipelineStageFlag::PIPELINE_STAGE_FLAG_PIXEL_SHADER; }

		virtual const String& GetName() const override final
		{
			static String name = RENDER_GRAPH_LIGHT_STAGE_NAME;
			return name;
		}
	
	private:
		bool CreatePipelineLayout();
		bool CreateDescriptorSets();
		bool CreateShaders();
		bool CreateCommandLists();
		bool CreateRenderPass(RenderPassAttachmentDesc* pBackBufferAttachmentDesc);
		bool CreatePipelineState();

		DescriptorSet* GetDrawArgsDescriptorSet(const String& debugname, uint32 descriptorLayoutIndex);

	private:
		TArray<TSharedRef<const TextureView>>	m_PointLFaceViews;

		const DrawArg*							m_pDrawArgs = nullptr;
		uint32									m_DrawCount	= 0;
		bool									m_UsingMeshShader = false;

		GUID_Lambda								m_VertexShaderPointGUID = 0;
		GUID_Lambda								m_PixelShaderPointGUID = 0;

		TSharedRef<CommandAllocator>			m_CopyCommandAllocator = nullptr;
		TSharedRef<CommandList>					m_CopyCommandList = nullptr;

		CommandAllocator**						m_ppGraphicCommandAllocators = nullptr;
		CommandList**							m_ppGraphicCommandLists = nullptr;

		TSharedRef<RenderPass>					m_RenderPass = nullptr;

		uint64									m_PipelineStateID = 0;
		TSharedRef<PipelineLayout>				m_PipelineLayout = nullptr;
		TSharedRef<DescriptorHeap>				m_DescriptorHeap = nullptr;
		TSharedRef<DescriptorSet>				m_LightDescriptorSet;
		TArray<TSharedRef<DescriptorSet>>		m_DrawArgsDescriptorSets;

		uint32									m_BackBufferCount = 0;
		TArray<TSharedRef<const TextureView>>	m_BackBuffers;
		TSharedRef<const TextureView>			m_DepthStencilBuffer;

	private:
		static LightRenderer* s_pInstance;

	};
}