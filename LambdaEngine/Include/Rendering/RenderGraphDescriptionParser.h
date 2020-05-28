#pragma once

#include "LambdaEngine.h"

#include "RenderGraphTypes.h"
#include "RenderGraph.h"

#include <unordered_map>
#include <vector>
#include <set>

namespace LambdaEngine
{
	class LAMBDA_API RenderGraphDescriptionParser
	{
		struct InternalRenderStageAttachment;

		struct InternalRenderStage
		{
			const RenderStageDesc*							pRenderStage				= nullptr;
			std::vector<InternalRenderStageAttachment*>		InputAttachments;
			std::vector<InternalRenderStageAttachment*>		TemporalInputAttachments;
			std::vector<InternalRenderStageAttachment*>		ExternalInputAttachments;
			std::vector<InternalRenderStageAttachment*>		OutputAttachments;
			std::set<InternalRenderStage*>					ParentRenderStages;
			std::set<InternalRenderStage*>					ChildRenderStages;
			uint32											GlobalIndex					= 0;
			uint32											Weight						= 0;
		};

		struct InternalRenderStageAttachment
		{
			std::string														AttachmentName;
			std::unordered_map<std::string, std::pair<InternalRenderStage*, const RenderStageAttachment*>>	RenderStageNameToRenderStageAndAttachment;
			uint32															GlobalIndex = 0;

			const InternalRenderStageAttachment*							pConnectedAttachment = nullptr;
		};

	public:
		DECL_STATIC_CLASS(RenderGraphDescriptionParser);

		/*
		* Goes through the RenderGraphDesc and creates the underlying RenderGraphStructure, consisting of the vectors sortedRenderStages, sortedSynchronizationStages, sortedPipelineStages
		*	desc - The RenderGraph Description
		*	sortedRenderStageDescriptions - (Output) Sorted Render Stage Descriptions in order of execution
		*	sortedSynchronizationStageDescriptions - (Output) Sorted Resource Synchronization Stage Descriptions in order of execution
		*	sortedPipelineStageDescriptions - (Output) Sorted Pipeline Stage Descriptions in order of execution, each PipelineStage is either a RenderStage or a SynchronizationStage
		*	resourceDescriptions - (Output) All resources that are used internally by this Render Graph
		* return - true if the parsing succeeded, otherwise false
		*/
		static bool Parse(
			const RenderGraphDesc* pDesc,
			std::vector<RenderStageDesc>& sortedRenderStageDescriptions,
			std::vector<SynchronizationStageDesc>& sortedSynchronizationStageDescriptions,
			std::vector<PipelineStageDesc>& sortedPipelineStageDescriptions,
			std::vector<RenderStageResourceDesc>& resourceDescriptions);

	private:
		static bool CreateDebugStages(const RenderGraphDesc* pDesc, std::vector<RenderStageDesc*>& renderStages);

		/*
		* Goes through each Render Stage and sorts its attachments into three groups, either Input, External Input or Output
		*/
		static bool SortRenderStagesAttachments(
			const std::vector<RenderStageDesc*>& renderStages,
			std::unordered_map<std::string, std::vector<const RenderStageAttachment*>>&		renderStagesInputAttachments,
			std::unordered_map<std::string, std::vector<const RenderStageAttachment*>>&		renderStagesExternalInputAttachments,
			std::unordered_map<std::string, std::vector<const RenderStageAttachment*>>&		renderStagesOutputAttachments,
			std::vector<RenderStageResourceDesc>&											resourceDescriptions);

		/*
		* Parses everything to internal structures, creates bidirectional connections, separates temporal inputs from non-temporal inputs
		*/
		static bool ParseInitialStages(
			const std::vector<RenderStageDesc*>& renderStages,
			std::unordered_map<std::string, std::vector<const RenderStageAttachment*>>&		renderStagesInputAttachments,
			std::unordered_map<std::string, std::vector<const RenderStageAttachment*>>&		renderStagesExternalInputAttachments,
			std::unordered_map<std::string, std::vector<const RenderStageAttachment*>>&		renderStagesOutputAttachments,
			std::unordered_map<std::string, InternalRenderStage>&							parsedRenderStages,
			std::unordered_map<std::string, InternalRenderStageAttachment>&					parsedInputAttachments,
			std::unordered_map<std::string, InternalRenderStageAttachment>&					parsedTemporalInputAttachments,
			std::unordered_map<std::string, InternalRenderStageAttachment>&					parsedExternalInputAttachments,
			std::unordered_map<std::string, InternalRenderStageAttachment>&					parsedOutputAttachments);

		static bool ParseStage(
			const RenderStageDesc* pRenderStage,
			InternalRenderStage* pInternalRenderStage,
			uint32* pGlobalAttachmentIndex,
			std::unordered_map<std::string, std::vector<const RenderStageAttachment*>>&		renderStagesInputAttachments,
			std::unordered_map<std::string, std::vector<const RenderStageAttachment*>>&		renderStagesExternalInputAttachments,
			std::unordered_map<std::string, std::vector<const RenderStageAttachment*>>&		renderStagesOutputAttachments,
			std::unordered_map<std::string, InternalRenderStage>&							parsedRenderStages,
			std::unordered_map<std::string, InternalRenderStageAttachment>&					parsedInputAttachments,
			std::unordered_map<std::string, InternalRenderStageAttachment>&					parsedTemporalInputAttachments,
			std::unordered_map<std::string, InternalRenderStageAttachment>&					parsedExternalInputAttachments,
			std::unordered_map<std::string, InternalRenderStageAttachment>&					parsedOutputAttachments);

		/*
		* Connects output resources to input resources, thereby marking resource transitions, also discovers input resources that miss output resources
		*/
		static bool ConnectOutputsToInputs(
			std::unordered_map<std::string, InternalRenderStageAttachment>&	parsedInputAttachments,
			std::unordered_map<std::string, InternalRenderStageAttachment>&	parsedOutputAttachments);
		/*
		* For each renderpass, go through its ascendants and add 1 to their weights
		*/
		static bool WeightRenderStages(
			std::unordered_map<std::string, InternalRenderStage>& parsedRenderStages,
			std::unordered_map<std::string, InternalRenderStageAttachment>& parsedOutputAttachments);
		static void RecursivelyWeightAncestors(InternalRenderStage* pRenderStage);
		/*
		* Sorts Render Stages and Creates Synchronization Stages
		*/
		static bool SortPipelineStages(
			std::unordered_map<std::string, InternalRenderStage>&	parsedRenderStages,
			std::vector<const InternalRenderStage*>&				sortedInternalRenderStages,
			std::vector<RenderStageDesc>&							sortedRenderStages,
			std::vector<SynchronizationStageDesc>&					sortedSynchronizationStages,
			std::vector<PipelineStageDesc>&							sortedPipelineStages);
		/*
		* Removes unnecessary synchronizations
		*/
		static bool PruneUnnecessarySynchronizations(
			std::vector<SynchronizationStageDesc>&		sortedSynchronizationStages,
			std::vector<PipelineStageDesc>&				sortedPipelineStages);

		static bool IsInputTemporal(const std::vector<const RenderStageAttachment*>& renderStageOutputAttachments, const RenderStageAttachment* pInputAttachment);
		static bool AreRenderStagesRelated(const InternalRenderStage* pRenderStageAncestor, const InternalRenderStage* pRenderStageDescendant);

		static bool WriteGraphViz(
			const char* pName, 
			bool declareExternalInputs, 
			bool linkExternalInputs,
			std::unordered_map<std::string, InternalRenderStage>&					parsedRenderStages,
			std::unordered_map<std::string, InternalRenderStageAttachment>&			parsedInputAttachments,
			std::unordered_map<std::string, InternalRenderStageAttachment>&			parsedTemporalInputAttachments,
			std::unordered_map<std::string, InternalRenderStageAttachment>&			parsedExternalInputAttachments,
			std::unordered_map<std::string, InternalRenderStageAttachment>&			parsedOutputAttachments,
			std::vector<const InternalRenderStage*>&								sortedInternalRenderStages,
			std::vector<RenderStageDesc>&											sortedRenderStages,
			std::vector<SynchronizationStageDesc>&									sortedSynchronizationStages,
			std::vector<PipelineStageDesc>&											sortedPipelineStages);
		static void WriteGraphVizPipelineStages(
			FILE* pFile,
			std::unordered_map<std::string, InternalRenderStage>&					parsedRenderStages,
			std::unordered_map<std::string, InternalRenderStageAttachment>&			parsedInputAttachments,
			std::unordered_map<std::string, InternalRenderStageAttachment>&			parsedTemporalInputAttachments,
			std::unordered_map<std::string, InternalRenderStageAttachment>&			parsedExternalInputAttachments,
			std::unordered_map<std::string, InternalRenderStageAttachment>&			parsedOutputAttachments,
			std::vector<const InternalRenderStage*>&								sortedInternalRenderStages,
			std::vector<RenderStageDesc>&											sortedRenderStages,
			std::vector<SynchronizationStageDesc>&									sortedSynchronizationStages,
			std::vector<PipelineStageDesc>&											sortedPipelineStages);
		static void WriteGraphVizCompleteDeclarations(
			FILE* pFile, 
			bool declareExternalInputs,
			std::unordered_map<std::string, InternalRenderStage>&					parsedRenderStages,
			std::unordered_map<std::string, InternalRenderStageAttachment>&			parsedInputAttachments,
			std::unordered_map<std::string, InternalRenderStageAttachment>&			parsedTemporalInputAttachments,
			std::unordered_map<std::string, InternalRenderStageAttachment>&			parsedExternalInputAttachments,
			std::unordered_map<std::string, InternalRenderStageAttachment>&			parsedOutputAttachments,
			std::vector<const InternalRenderStage*>&								sortedInternalRenderStages,
			std::vector<RenderStageDesc>&											sortedRenderStages,
			std::vector<SynchronizationStageDesc>&									sortedSynchronizationStages,
			std::vector<PipelineStageDesc>&											sortedPipelineStages);
		static void WriteGraphVizCompleteDefinitions(
			FILE* pFile, 
			bool externalInputsDeclared, 
			bool linkExternalInputs,
			std::unordered_map<std::string, InternalRenderStage>&					parsedRenderStages,
			std::unordered_map<std::string, InternalRenderStageAttachment>&			parsedInputAttachments,
			std::unordered_map<std::string, InternalRenderStageAttachment>&			parsedTemporalInputAttachments,
			std::unordered_map<std::string, InternalRenderStageAttachment>&			parsedExternalInputAttachments,
			std::unordered_map<std::string, InternalRenderStageAttachment>&			parsedOutputAttachments,
			std::vector<const InternalRenderStage*>&								sortedInternalRenderStages,
			std::vector<RenderStageDesc>&											sortedRenderStages,
			std::vector<SynchronizationStageDesc>&									sortedSynchronizationStages,
			std::vector<PipelineStageDesc>&											sortedPipelineStages);

		static void SanitizeString(char* pString, uint32 numCharacters);
		static void ConcatPipelineStateToString(char* pStringBuffer, EPipelineStateType pipelineState);

	private:
		static RenderStageDesc						s_ImGuiRenderStageDesc;
		static std::vector<RenderStageAttachment>	s_ImGuiAttachments;
	};
}
