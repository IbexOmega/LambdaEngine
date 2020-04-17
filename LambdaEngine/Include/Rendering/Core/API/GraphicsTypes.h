#pragma once
#include "LambdaEngine.h"

#include "Containers/TArray.h"

namespace LambdaEngine
{
	class ISampler;

	constexpr uint32 MAX_FRAMES_IN_FLIGHT = 3;

    enum class EMemoryType : uint8
    {
        NONE				= 0,
        MEMORY_CPU_VISIBLE  = 1,
        MEMORY_GPU			= 2,
    };

    enum class EFormat : uint8
    {
        NONE						= 0,
        FORMAT_R8G8B8A8_UNORM		= 1,
        FORMAT_B8G8R8A8_UNORM		= 2,
		FORMAT_D24_UNORM_S8_UINT	= 3
    };

	enum class ECommandQueueType : uint8
	{
		COMMAND_QUEUE_UNKNOWN	= 0,
		COMMAND_QUEUE_NONE		= 1,
		COMMAND_QUEUE_COMPUTE	= 2,
		COMMAND_QUEUE_GRAPHICS	= 3,
		COMMAND_QUEUE_COPY		= 4,
	};

	enum class ECommandListType : uint8
	{
		COMMAND_LIST_UNKNOWN		= 0,
		COMMAND_LIST_PRIMARY		= 1,
		COMMAND_LIST_SECONDARY		= 2
	};

	enum class EShaderLang : uint32
	{
		NONE	= 0,
		SPIRV	= BIT(0),
	};

	enum class ELoadOp : uint8
	{
		NONE		= 0,
		LOAD		= 1,
		CLEAR		= 2,
		DONT_CARE	= 3,
	};

	enum class EStoreOp : uint8
	{
		NONE		= 0,
		STORE		= 1,
		DONT_CARE	= 2,
	};

	enum class EFilter : uint8
	{
		NONE		= 0,
		NEAREST		= 1,
		LINEAR		= 2,
	};

	enum class EMipmapMode : uint8
	{
		NONE		= 0,
		NEAREST		= 1,
		LINEAR		= 2,
	};

	enum class EAddressMode : uint8
	{
		NONE					= 0,
		REPEAT					= 1,
		MIRRORED_REPEAT			= 2,
		CLAMP_TO_EDGE			= 3,
		CLAMP_TO_BORDER			= 4,
		MIRRORED_CLAMP_TO_EDGE	= 5,
	};

	enum class ETextureState : uint32
	{
		TEXTURE_STATE_UNKNOWN								= 0,
		TEXTURE_STATE_DONT_CARE								= 1,
		TEXTURE_STATE_GENERAL								= 2,
		TEXTURE_STATE_RENDER_TARGET							= 3,
		TEXTURE_STATE_DEPTH_STENCIL_ATTACHMENT				= 4,
		TEXTURE_STATE_DEPTH_STENCIL_READ_ONLY				= 5,
		TEXTURE_STATE_SHADER_READ_ONLY						= 6,
		TEXTURE_STATE_COPY_SRC								= 7,
		TEXTURE_STATE_COPY_DST								= 8,
		TEXTURE_STATE_PREINITIALIZED						= 9,
		TEXTURE_STATE_DEPTH_READ_ONLY_STENCIL_ATTACHMENT	= 10,
		TEXTURE_STATE_DEPTH_ATTACHMENT_STENCIL_READ_ONLY	= 11,
		TEXTURE_STATE_DEPTH_ATTACHMENT						= 12,
		TEXTURE_STATE_DEPTH_READ_ONLY						= 13,
		TEXTURE_STATE_STENCIL_ATTACHMENT					= 14,
		TEXTURE_STATE_STENCIL_READ_ONLY						= 15,
		TEXTURE_STATE_PRESENT								= 16,
		TEXTURE_STATE_SHADING_RATE							= 17,
	};

	enum class EDescriptorType : uint32
	{
		DESCRIPTOR_UNKNOWN							= 0,
		DESCRIPTOR_SHADER_RESOURCE_TEXTURE			= 1,
		DESCRIPTOR_SHADER_RESOURCE_COMBINED_SAMPLER	= 3,
		DESCRIPTOR_UNORDERED_ACCESS_TEXTURE			= 2,
		DESCRIPTOR_CONSTANT_BUFFER					= 4,
		DESCRIPTOR_UNORDERED_ACCESS_BUFFER			= 5,
		DESCRIPTOR_ACCELERATION_STRUCTURE			= 6,
		DESCRIPTOR_SAMPLER							= 7,
	};

	enum FShaderStageFlags : uint32
	{
		SHADER_STAGE_FLAG_NONE					= 0,
		SHADER_STAGE_FLAG_MESH_SHADER			= FLAG(0),
		SHADER_STAGE_FLAG_TASK_SHADER			= FLAG(1),
		SHADER_STAGE_FLAG_VERTEX_SHADER			= FLAG(2),
		SHADER_STAGE_FLAG_GEOMETRY_SHADER		= FLAG(3),
		SHADER_STAGE_FLAG_HULL_SHADER			= FLAG(4),
		SHADER_STAGE_FLAG_DOMAIN_SHADER			= FLAG(5),
		SHADER_STAGE_FLAG_PIXEL_SHADER			= FLAG(6),
		SHADER_STAGE_FLAG_COMPUTE_SHADER		= FLAG(7),
		SHADER_STAGE_FLAG_RAYGEN_SHADER			= FLAG(8),
		SHADER_STAGE_FLAG_INTERSECT_SHADER		= FLAG(9),
		SHADER_STAGE_FLAG_ANY_HIT_SHADER		= FLAG(10),
		SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER	= FLAG(11),
		SHADER_STAGE_FLAG_MISS_SHADER			= FLAG(12),
	};

	enum FTextureFlags : uint16
	{
		TEXTURE_FLAG_NONE				= 0,
		TEXTURE_FLAG_RENDER_TARGET		= FLAG(1),
		TEXTURE_FLAG_SHADER_RESOURCE	= FLAG(2),
		TEXTURE_FLAG_UNORDERED_ACCESS	= FLAG(3),
		TEXTURE_FLAG_DEPTH_STENCIL		= FLAG(4),
		TEXTURE_FLAG_COPY_SRC			= FLAG(5),
		TEXTURE_FLAG_COPY_DST			= FLAG(6),
	};

	enum FPipelineStageFlags : uint32
	{
		PIPELINE_STAGE_FLAG_UNKNOWN							= 0,
		PIPELINE_STAGE_FLAG_TOP								= FLAG(1),
		PIPELINE_STAGE_FLAG_BOTTOM							= FLAG(2),
		PIPELINE_STAGE_FLAG_DRAW_INDIRECT					= FLAG(3),
		PIPELINE_STAGE_FLAG_VERTEX_INPUT					= FLAG(4),
		PIPELINE_STAGE_FLAG_VERTEX_SHADER					= FLAG(5),
		PIPELINE_STAGE_FLAG_HULL_SHADER						= FLAG(6),
		PIPELINE_STAGE_FLAG_DOMAIN_SHADER					= FLAG(7),
		PIPELINE_STAGE_FLAG_GEOMETRY_SHADER					= FLAG(8),
		PIPELINE_STAGE_FLAG_PIXEL_SHADER					= FLAG(9),
		PIPELINE_STAGE_FLAG_EARLY_FRAGMENT_TESTS			= FLAG(10),
		PIPELINE_STAGE_FLAG_LATE_FRAGMENT_TESTS				= FLAG(11),
		PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT			= FLAG(12),
		PIPELINE_STAGE_FLAG_COMPUTE_SHADER					= FLAG(13),
		PIPELINE_STAGE_FLAG_COPY							= FLAG(14),
		PIPELINE_STAGE_FLAG_HOST							= FLAG(15),
		PIPELINE_STAGE_FLAG_STREAM_OUTPUT					= FLAG(16),
		PIPELINE_STAGE_FLAG_CONDITIONAL_RENDERING			= FLAG(17),
		PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER				= FLAG(18),
		PIPELINE_STAGE_FLAG_ACCELERATION_STRUCTURE_BUILD	= FLAG(19),
		PIPELINE_STAGE_FLAG_SHADING_RATE_TEXTURE			= FLAG(20),
		PIPELINE_STAGE_FLAG_TASK_SHADER						= FLAG(21),
		PIPELINE_STAGE_FLAG_MESH_SHADER						= FLAG(22),
	};

	enum FMemoryAccessFlags : uint32
	{
		MEMORY_ACCESS_FLAG_UNKNOWN								= 0,
		MEMORY_ACCESS_FLAG_INDIRECT_COMMAND_READ				= FLAG(1),
		MEMORY_ACCESS_FLAG_INDEX_READ							= FLAG(2),
		MEMORY_ACCESS_FLAG_VERTEX_ATTRIBUTE_READ				= FLAG(3),
		MEMORY_ACCESS_FLAG_CONSTANT_BUFFER_READ							= FLAG(4),
		MEMORY_ACCESS_FLAG_INPUT_ATTACHMENT_READ				= FLAG(5),
		MEMORY_ACCESS_FLAG_SHADER_READ							= FLAG(6),
		MEMORY_ACCESS_FLAG_SHADER_WRITE							= FLAG(7),
		MEMORY_ACCESS_FLAG_COLOR_ATTACHMENT_READ				= FLAG(8),
		MEMORY_ACCESS_FLAG_COLOR_ATTACHMENT_WRITE				= FLAG(9),
		MEMORY_ACCESS_FLAG_DEPTH_STENCIL_ATTACHMENT_READ		= FLAG(10),
		MEMORY_ACCESS_FLAG_DEPTH_STENCIL_ATTACHMENT_WRITE		= FLAG(11),
		MEMORY_ACCESS_FLAG_TRANSFER_READ						= FLAG(12),
		MEMORY_ACCESS_FLAG_TRANSFER_WRITE						= FLAG(13),
		MEMORY_ACCESS_FLAG_HOST_READ							= FLAG(14),
		MEMORY_ACCESS_FLAG_HOST_WRITE							= FLAG(15),
		MEMORY_ACCESS_FLAG_MEMORY_READ							= FLAG(16),
		MEMORY_ACCESS_FLAG_MEMORY_WRITE							= FLAG(17),
		MEMORY_ACCESS_FLAG_TRANSFORM_FEEDBACK_WRITE				= FLAG(18),
		MEMORY_ACCESS_FLAG_TRANSFORM_FEEDBACK_COUNTER_READ		= FLAG(19),
		MEMORY_ACCESS_FLAG_TRANSFORM_FEEDBACK_COUNTER_WRITE		= FLAG(20),
		MEMORY_ACCESS_FLAG_CONDITIONAL_RENDERING_READ			= FLAG(21),
		MEMORY_ACCESS_FLAG_COLOR_ATTACHMENT_READ_NONCOHERENT	= FLAG(22),
		MEMORY_ACCESS_FLAG_ACCELERATION_STRUCTURE_READ			= FLAG(23),
		MEMORY_ACCESS_FLAG_ACCELERATION_STRUCTURE_WRITE			= FLAG(24),
		MEMORY_ACCESS_FLAG_SHADING_RATE_IMAGE_READ				= FLAG(25),
		MEMORY_ACCESS_FLAG_FRAGMENT_DENSITY_MAP_READ			= FLAG(26),
		MEMORY_ACCESS_FLAG_COMMAND_PREPROCESS_READ				= FLAG(27),
		MEMORY_ACCESS_FLAG_COMMAND_PREPROCESS_WRITE				= FLAG(28),
	};

	enum FColorComponentFlags : uint8
	{
		COLOR_COMPONENT_FLAG_NONE	= 0,
		COLOR_COMPONENT_FLAG_R		= FLAG(1),
		COLOR_COMPONENT_FLAG_G		= FLAG(2),
		COLOR_COMPONENT_FLAG_B		= FLAG(3),
		COLOR_COMPONENT_FLAG_A		= FLAG(4),
	};

	enum FAccelerationStructureFlags : uint16
	{
		ACCELERATION_STRUCTURE_FLAG_NONE			= 0,
		ACCELERATION_STRUCTURE_FLAG_ALLOW_UPDATE	= FLAG(1),
	};

	struct DescriptorCountDesc
	{
		uint32 DescriptorSetCount						= 0;
		uint32 SamplerDescriptorCount					= 0;
		uint32 TextureDescriptorCount					= 0;
		uint32 TextureCombinedSamplerDescriptorCount	= 0;
		uint32 ConstantBufferDescriptorCount			= 0;
		uint32 UnorderedAccessBufferDescriptorCount		= 0;
		uint32 UnorderedAccessTextureDescriptorCount	= 0;
		uint32 AccelerationStructureDescriptorCount		= 0;
	};

	struct Viewport
	{
		float MinDepth	= 0.0f;
		float MaxDepth	= 0.0f;
		float Width		= 0.0f;
		float Height	= 0.0f;
		float x		    = 0.0f;
		float y		    = 0.0f;
	};

	struct ScissorRect
	{
		uint32 Width	= 0;
		uint32 Height	= 0;
		int32 x		    = 0;
		int32 y		    = 0;
	};
}
