#include "utility/allocators.h"
#include "utility/string.h"

enum SHADER_IN_DATA_TYPE
{
	SHADER_IN_NONE,

	SHADER_IN_POS_UV_NORM,
	SHADER_IN_POS_UV_NORM_INSTANCE_POS
};

enum ENTITY_ASSET_TAG
{
	ENTITY_ASSET_NONE,

	ENTITY_ASSET_VECTOR_GIZMO,
	ENTITY_ASSET_GRID_CELL,
	ENTITY_ASSET_CUBE,

	ENTITY_ASSET_COUNT,
};

enum SHADER_TYPE
{
	SHADER_TYPE_NONE,

	SHADER_TYPE_VERTEX,
	SHADER_TYPE_PIXEL,

	SHADER_TYPE_COUNT,
};

struct entity_asset_info
{
	const char* Path;
};

enum PIPELINE_TAG
{
	PIPELINE_NONE,

	PIPELINE_GIZMOS,
	PIPELINE_GRID,
	PIPELINE_CUBE,

	PIPELINE_COUNT
};

struct shader_info
{
	const char* Path;
	SHADER_TYPE Type;
};

struct pipeline_info
{
	SHADER_IN_DATA_TYPE ShaderInputType;
	shader_info         Shaders[SHADER_TYPE_COUNT];
};

struct tka_header
{
	u32  AllocationSize;
	u32  DataSize;
};

static entity_asset_info AssetTable[ENTITY_ASSET_COUNT] =
{
	// NONE
	{
		""
	},
	// VECTOR_GIZMO
	{
		"debug_vector_base.tka"
	},
	// GRID_CELL
	{
		"grid_cell.tka"
	},
	// CUBE
	{
		"cube.tka"
	}
};

static pipeline_info PipelineTable[PIPELINE_COUNT] =
{
	// NONE (Unused)
	{
		SHADER_IN_NONE,
		{
			{ nullptr, SHADER_TYPE_VERTEX },
			{ nullptr, SHADER_TYPE_PIXEL  },
		},
	},
	// GIZMOS_PIPELINE
	{
		SHADER_IN_POS_UV_NORM,
		{
			{ "gizmos_vs.cso", SHADER_TYPE_VERTEX},
			{ "gizmos_ps.cso", SHADER_TYPE_PIXEL},
		},
	},
	// GRID_PIPELINE
	{
		SHADER_IN_POS_UV_NORM_INSTANCE_POS,
		{
			{ "grid_cell_vs.cso", SHADER_TYPE_VERTEX},
			{ "grid_cell_ps.cso", SHADER_TYPE_PIXEL},
		},
	},
	// CUBE_PIPELINE
	{
		SHADER_IN_POS_UV_NORM,
		{
			{ "cube_vs.cso", SHADER_TYPE_VERTEX},
			{ "cube_ps.cso", SHADER_TYPE_PIXEL},
		},
	},
};