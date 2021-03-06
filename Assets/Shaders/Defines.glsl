#ifndef DEFINES_SHADER
#define DEFINES_SHADER

#if !defined(NO_BUFFERS) && !defined(NO_TEXTURES)
	#define BUFFER_SET_INDEX 			0
	#define TEXTURE_SET_INDEX 			1
	#define DRAW_SET_INDEX 				2
	#define DRAW_EXTENSIONS_SET_INDEX 	3
#elif defined(NO_BUFFERS)
	#define TEXTURE_SET_INDEX 			0
	#define DRAW_SET_INDEX 				1
	#define DRAW_EXTENSIONS_SET_INDEX 	2
#elif defined (NO_TEXTURES)
	#define BUFFER_SET_INDEX 			0
	#define DRAW_SET_INDEX 				1
	#define DRAW_EXTENSIONS_SET_INDEX 	2
#else
	#define DRAW_SET_INDEX 				0
	#define DRAW_EXTENSIONS_SET_INDEX 	1
#endif

#define UNWRAP_DRAW_SET_INDEX 3

#define MAX_UNIQUE_MATERIALS 32

#define UINT32_MAX              0xffffffff

const float INV_PI			= 1.0f / 3.14159265359f;
const float FOUR_PI			= 12.5663706144f;
const float TWO_PI			= 6.28318530718f;
const float PI 				= 3.14159265359f;
const float PI_OVER_TWO		= 1.57079632679f;
const float PI_OVER_FOUR	= 0.78539816330f;
const float EPSILON			= 0.001f;
const float GAMMA			= 2.2f;

#define MAX_NUM_AREA_LIGHTS 4
#define AREA_LIGHT_TYPE_QUAD 1

// Used in PointShadowDepthTest for PCF soft shadows
const vec3 sampleOffsetDirections[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1),
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);

struct SPositions
{
	vec3 WorldPos;
	vec3 ViewPos;
};

struct SVertex
{
	vec4 Position;
	vec4 Normal;
	vec4 Tangent;
	vec4 TexCoord;
};

struct SMeshlet
{
	uint VertCount;
	uint VertOffset;
	uint PrimCount;
	uint PrimOffset;
};

struct SInstance
{
	mat4 Transform;
	mat4 PrevTransform;
	uint MaterialSlot;
	uint ExtensionGroupIndex;
	uint TexturesPerExtensionGroup;
	uint MeshletCount;
	uint TeamIndex;
	uint PlayerIndex;
	uint Padding1;
	uint Padding2;
};

struct SIndirectArg
{
	uint	IndexCount;
	uint	InstanceCount;
	uint	FirstIndex;
	int		VertexOffset;
	uint	FirstInstance;

	uint	MaterialIndex;
};

struct SAreaLight
{
	uint	InstanceIndex;
	uint	Type;
	uvec2	Padding;
};

struct SPointLight
{
	vec4	ColorIntensity;
	vec3	Position;
	float	NearPlane;
	float	FarPlane;
	uint	TextureIndex;
	vec2	padding0;
	mat4	ProjView[6];
};

struct SLightsBuffer
{
	vec4	DirL_ColorIntensity;
	vec3	DirL_Direction;
	float	PointLightCount;
	mat4	DirL_ProjView;
};

struct SPerFrameBuffer
{
	mat4 Projection;
	mat4 View;
	mat4 PrevProjection;
	mat4 PrevView;
	mat4 ViewInv;
	mat4 ProjectionInv;
	vec4 CameraPosition;
	vec4 CameraRight;
	vec4 CameraUp;
	vec2 Jitter;
	vec2 ViewPortSize;

	uint FrameIndex;
	uint RandomSeed;
};

struct SMaterialParameters
{
	vec4	Albedo;
	float	AO;
	float	Metallic;
	float	Roughness;
	float	Unused;
};

struct SShapeSample
{
	vec3	Position;
	vec3	Normal;
	float	PDF;
};

struct SParticleVertex
{
	vec3 Position;
	float padding0;
};

struct SParticle
{
	mat4 Transform;
	vec3 Velocity;
	float CurrentLife;
	vec3 StartVelocity;
	float Padding0;
	vec3 Acceleration;
	uint TileIndex;
	vec3 StartPosition;
	bool WasCreated;
	vec3 StartAcceleration;
	float BeginRadius;
	float EndRadius;
	float FrictionFactor;
	float ShouldStop;
	float Padding1;
};

struct SEmitter
{
	vec4 Color;
	float LifeTime;
	float Radius;
	uint AtlasIndex;
	uint AnimationCount;
	uint FirstAnimationIndex;
	float Gravity;
	bool OneTime;
	uint Padding2;
};

struct SAtlasData
{
	float	TileFactorX;
	float	TileFactorY;
	uint	RowCount;
	uint	ColCount;
	uint	AtlasIndex;
};

struct SPaintData
{
	vec4 TargetPosition;
	vec4 TargetDirectionXYZAngleW;
	uint PaintMode;
	uint RemoteMode;
	uint TeamMode;
	uint ClearClient;
};

struct SAccelerationStructureInstance
{
	mat3x4	Transform;
	uint	HitMask_CustomProperties;
	uint	SBTRecordOffset_Flags;
	uint	AccelerationStructureHandleTop32;
	uint	AccelerationStructureHandleBottom32;
};

struct SParticleIndexData
{
	uint EmitterIndex;
	uint ASInstanceIndirectIndex;
	uint Padding0;
	uint Padding1;
};

#endif