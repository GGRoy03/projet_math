#include "utility/allocators.h"
#include "math/matrix.hpp"

struct space
{
	f32   CellSize = 1.0f;
	vec_3 Origin;
	vec_3 Dimensions;

	u32              CellObjectResourceKey;
	u32              CellInstanceResourceKey;
	bump_allocator   CellInstanceData;
	mesh_info*       CellMeshInfo;
	render_pipeline* Pipeline;
};

static space Space;

struct cell_instance_data
{
	vec_3 Position;
};

static void Initialize3DSpace()
{
	Space.Origin           = vec_3(0.0f, 0.0f, 0.0f);
	Space.Dimensions       = vec_3(101.0f, 0.0f, 101.0f);
	Space.Pipeline         = CreateRenderPipeline(PipelineTable[PIPELINE_GRID]);
	Space.CellInstanceData = CreateBumpAllocator(Kilobytes(2), BUMP_RESIZABLE, "Cells");
	Space.CellMeshInfo     = LoadMesh(AssetTable[ENTITY_ASSET_GRID_CELL].Path);

	f32 InstanceCount      = Space.Dimensions.x * Space.Dimensions.z;
	size_t SizePerInstance = sizeof(cell_instance_data);

	vec_3 Origin = vec_3();
	f32 StartX   = Origin.x - (Space.Dimensions.x / 2);
	f32 StartZ   = Origin.z - (Space.Dimensions.z / 2);

	f32 PosX = StartX;
	for (u32 X = 0; X < Space.Dimensions.x; X++)
	{
		f32 PosZ = StartZ;
		for (u32 Z = 0; Z < Space.Dimensions.z; Z++)
		{
			cell_instance_data Instance = {};
			Instance.Position = vec_3(PosX, -0.05, PosZ);
			PushAndCopy(sizeof(Instance), &Instance, &Space.CellInstanceData);

			PosZ += Space.CellSize;
		}

		PosX += Space.CellSize; 
	}

	mat_4 GridTranslation         = TranslationMatrix(Space.Origin);
	Space.CellObjectResourceKey   = CreateObjectResource(&GridTranslation, sizeof(GridTranslation));
	Space.CellInstanceResourceKey = CreateInstancedResource(InstanceCount, Space.CellInstanceData.Memory, sizeof(cell_instance_data));

	FreeAllocator(&Space.CellInstanceData);
}

static void UpdateSpace()
{
	PushDrawCommand(Space.CellObjectResourceKey, Space.CellInstanceResourceKey, Space.CellMeshInfo,
		            Space.Pipeline);
}