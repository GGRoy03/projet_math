#include "math/matrix.hpp"
#include "utility/allocators.h"
#include <chrono>  // For the cube's shader

constexpr auto MAX_CUBE_COUNT = 1;
constexpr auto CONSTANT_DELTA_TIME = 0.030f;

struct simulation_cube
{
    u32   InstanceIndex;
    f32   Mass;
    f32   ForceMagnitude;
    vec_3 Position;
    vec_3 Velocity;
    vec_3 ForceApplied;
    bool  AffectedByGravity;
    bool  IsBeingSimulated;
};

struct simulation_vector
{
    vec_3 Origin;
    vec_3 Direction;
    vec_3 Rotation;
    vec_4 Color;
    u32   InstanceIndex;
};

struct vector_instance_data
{
    mat_4 Transform;
    vec_4 Color;
};

struct cube_instance_data
{
    mat_4 Transform;
    f32   Time;
};

struct cube_object_data
{
    mat_4 Transform;
    f32   Time;
    char  Padding[12];
};

struct Entity_manager
{
    render_pipeline* VectorPipeline;
    mesh_info* VectorMesh;
    u32              VectorEntitysCount;
    u32              VectorInstanceResourceKey;
    bump_allocator   VectorInstanceData;
    u16              VectorUpdateTypes;

    render_pipeline* CubePipeline;
    mesh_info* CubeMesh;
    u32              CubeResourceKey;
    u32              CubesCount;
    simulation_cube  Cube;

    simulation_vector Vectors[MAX_VECTORS];
};

static Entity_manager EntityManager;
auto StartTime = std::chrono::steady_clock::now();

// -----------------
// Vector Functions
// -----------------
static simulation_vector* CreateSimulationVector(vec_3 Origin, vec_3 Direction, vec_4 Color)
{
    u32 Index = EntityManager.VectorEntitysCount;
    if (Index < MAX_VECTORS)
    {
        EntityManager.VectorEntitysCount += 1;
        simulation_vector* Vector = &EntityManager.Vectors[Index];
        Vector->Origin            = Origin;
        Vector->Direction         = Direction;
        Vector->Color             = Color;
        Vector->InstanceIndex     = Index;
        return Vector;
    }
    return &EntityManager.Vectors[MAX_VECTORS - 1];
}

static void CreateVectorEntity(simulation_vector* Vector)
{
    vec_3 OriginToPoint      = Vector->Direction - Vector->Origin;
    vec_3 Direction          = Normalize(OriginToPoint);
    vec_3 DefaultOrientation = vec_3(0.0f, 0.0f, 1.0f);

    f32 ScaleFactor   = VectorLength(OriginToPoint);
    vec_3 ScaleVector = vec_3(1.0f, 1.0f, ScaleFactor);
    mat_4 Scale       = ScalingMatrix(ScaleVector);

    if (AreEqual(Direction, DefaultOrientation))
    {
        DefaultOrientation = vec_3(0.0f, 1.0f, 0.0f);
    }

    vec_3 Right = Normalize(VectorProduct(DefaultOrientation, Direction));
    vec_3 Up = Normalize(VectorProduct(Direction, Right));

    mat_4 Rotation = mat_4(
        vec_4(Right.x, Up.x, Direction.x, 0),
        vec_4(Right.y, Up.y, Direction.y, 0),
        vec_4(Right.z, Up.z, Direction.z, 0),
        vec_4(0, 0, 0, 1)
    );

    vec_3 OffsetVector = vec_3((OriginToPoint.x / 2 + Vector->Origin.x),
                               (OriginToPoint.y / 2 + Vector->Origin.y),
                               (OriginToPoint.z / 2 + Vector->Origin.z));
    mat_4 Translation = TranslationMatrix(OffsetVector);

    vector_instance_data VectorEntityData = {};
    VectorEntityData.Transform            = Translation * Rotation * Scale;
    VectorEntityData.Color                = Vector->Color;

    PushAndCopy(sizeof(vector_instance_data), &VectorEntityData, &EntityManager.VectorInstanceData);
    EntityManager.VectorUpdateTypes |= UPDATE_RESOURCE_RECREATE;
}

static void UpdateVectorPosition(simulation_vector* Vector)
{
    auto* PreviousInstanceData = (vector_instance_data*)EntityManager.VectorInstanceData.Memory + Vector->InstanceIndex;

    vec_3 OriginToPoint      = Vector->Direction - Vector->Origin;
    vec_3 Direction          = Normalize(OriginToPoint);
    vec_3 DefaultOrientation = vec_3(0.0f, 0.0f, 1.0f);

    f32 ScaleFactor   = VectorLength(OriginToPoint);
    vec_3 ScaleVector = vec_3(1.0f, 1.0f, ScaleFactor);
    mat_4 Scale       = ScalingMatrix(ScaleVector);

    if (AreEqual(Direction, DefaultOrientation))
    {
        DefaultOrientation = vec_3(0.0f, 1.0f, 0.0f);
    }

    vec_3 Right = Normalize(VectorProduct(DefaultOrientation, Direction));
    vec_3 Up    = Normalize(VectorProduct(Direction, Right));

    mat_4 ObjectRotation = mat_4(
        vec_4(Right.x, Up.x, Direction.x, 0),
        vec_4(Right.y, Up.y, Direction.y, 0),
        vec_4(Right.z, Up.z, Direction.z, 0),
        vec_4(0, 0, 0, 1)
    );
    mat_4 EulerRotation = RotationMatrixFromEulerAngles(Vector->Rotation);
    mat_4 FinalRotation = ObjectRotation * EulerRotation;

    vec_3 OffsetVector = vec_3((OriginToPoint.x / 2 + Vector->Origin.x),
                               (OriginToPoint.y / 2 + Vector->Origin.y),
                               (OriginToPoint.z / 2 + Vector->Origin.z));
    mat_4 Translation = TranslationMatrix(OffsetVector);

    PreviousInstanceData->Transform = Translation * FinalRotation * Scale;
    EntityManager.VectorUpdateTypes |= UPDATE_RESOURCE_DISCARD;
}

static void UpdateVectorColor(simulation_vector* Vector)
{
    auto* PreviousInstanceData = (vector_instance_data*)EntityManager.VectorInstanceData.Memory + Vector->InstanceIndex;
    memcpy(PreviousInstanceData->Color.AsArray, Vector->Color.AsArray, sizeof(PreviousInstanceData->Color));
    EntityManager.VectorUpdateTypes |= UPDATE_RESOURCE_DISCARD;
}

// -----------------
// Cube Functions
// -----------------
static simulation_cube* CreateSimulationCube(vec_3 Position)
{
    if (EntityManager.CubesCount < MAX_CUBE_COUNT)
    {
        u32 Index = EntityManager.CubesCount;
        EntityManager.CubesCount += 1;
        cube_instance_data CubeInstanceData = {};
        CubeInstanceData.Time = 0;
        CubeInstanceData.Transform = TranslationMatrix(Position);
        simulation_cube* Cube = &EntityManager.Cube;
        Cube->AffectedByGravity = false;
        Cube->IsBeingSimulated = false;
        Cube->Mass = 1.0f;
        Cube->Position = Position;
        Cube->Velocity = vec_3();
        Cube->InstanceIndex = Index;
        return Cube;
    }
    return nullptr;
}

static void UpdateCube(simulation_cube* Cube)
{
    f32 DeltaTime   = 0.015f;
    f32 Mass        = Cube->Mass;
    vec_3 Direction = vec_3(1.0f, 0.0f, 0.0f);
     
    f32 Magnitude    = 2.0f;
    vec_3 PushForce  = Normalize(Direction) * Magnitude;
    vec_3 TotalForce = PushForce;

    if (Cube->AffectedByGravity)
    {
        vec_3 GravityAccel = vec_3(0.0f, -9.81f, 0.0f);
        vec_3 GravityForce = GravityAccel * Mass;
        TotalForce = GravityForce + PushForce;
    }

    vec_3 TotalAccel = TotalForce / Mass;

    Cube->Velocity = Cube->Velocity + (TotalAccel * DeltaTime);
    Cube->Position = Cube->Position + (Cube->Velocity * DeltaTime);

    mat_4 CubePosition = TranslationMatrix(Cube->Position);
    auto Now           = std::chrono::steady_clock::now();

    cube_object_data CubeData = {};
    CubeData.Time             = std::chrono::duration<float>(Now - StartTime).count();
    CubeData.Transform        = CubePosition;

    UpdateObjectData(EntityManager.CubeResourceKey, &CubeData, sizeof(cube_object_data), UPDATE_RESOURCE_DISCARD);
}

static void ApplyPushForce(simulation_cube* Cube, vec_3 ForceToApply)
{
    f32 Magnitude = Cube->ForceMagnitude;
    if (Magnitude < 0)
    {
        Magnitude = 1;
    }

    vec_3 PushForce = Normalize(ForceToApply) * Magnitude;
    vec_3 PushAccel = PushForce / Cube->Mass;
    Cube->Velocity  = Cube->Velocity + PushAccel;
}

static void ApplyGravity(simulation_cube* Cube)
{
    vec_3 GravityAccel = vec_3(0.0f, -3.2f, 0.0f);
    Cube->Velocity     = Cube->Velocity + (GravityAccel * CONSTANT_DELTA_TIME);
}

static void SimulateCube(simulation_cube* Cube)
{
    Cube->Position     = Cube->Position + (Cube->Velocity * CONSTANT_DELTA_TIME);

    mat_4 CubePosition = TranslationMatrix(Cube->Position);
    auto Now           = std::chrono::steady_clock::now();

    cube_object_data CubeData = {};
    CubeData.Time             = std::chrono::duration<float>(Now - StartTime).count();
    CubeData.Transform        = CubePosition;
    UpdateObjectData(EntityManager.CubeResourceKey, &CubeData, sizeof(cube_object_data), UPDATE_RESOURCE_DISCARD);
}

static void StopCubeSimulation(simulation_cube* Cube)
{
    Cube->Position         = vec_3(1.0f, 0.5f, 1.0f);
    Cube->Velocity         = vec_3();
    Cube->IsBeingSimulated = false;

    mat_4 CubePosition = TranslationMatrix(Cube->Position);
    auto Now           = std::chrono::steady_clock::now();

    cube_object_data CubeData = {};
    CubeData.Time             = std::chrono::duration<float>(Now - StartTime).count();
    CubeData.Transform        = CubePosition;

    UpdateObjectData(EntityManager.CubeResourceKey, &CubeData, sizeof(cube_object_data), UPDATE_RESOURCE_DISCARD);
}

// -----------------
// Entity Manager
// -----------------

static void InitializeEntityManager()
{
    u8 Dummy = 0;
    EntityManager.VectorPipeline = CreateRenderPipeline(PipelineTable[PIPELINE_GIZMOS]);
    EntityManager.VectorMesh = LoadMesh(AssetTable[ENTITY_ASSET_VECTOR_GIZMO].Path);
    EntityManager.VectorInstanceData = CreateBumpAllocator(Kilobytes(2), BUMP_RESIZABLE, "Vector Entitys");
    EntityManager.VectorInstanceResourceKey = CreateInstancedResource(1, &Dummy, sizeof(vector_instance_data));

    EntityManager.CubePipeline = CreateRenderPipeline(PipelineTable[PIPELINE_CUBE]);
    EntityManager.CubeMesh = LoadMesh(AssetTable[ENTITY_ASSET_CUBE].Path);
    EntityManager.Cube.Position = vec_3(1.5f, 0.5f, 1.5f);
    EntityManager.Cube.AffectedByGravity = false;
    EntityManager.Cube.Mass = 1.0f;

    cube_object_data CubeDefault = {};
    CubeDefault.Transform = TranslationMatrix(vec_3(1.5f, 0.5f, 1.5f));
    EntityManager.CubeResourceKey = CreateObjectResource(&CubeDefault, sizeof(cube_object_data));
}

static void UpdateEntities()
{
    if (EntityManager.VectorUpdateTypes != UPDATE_RESOURCE_NONE)
    {
        size_t ResourceOffset = 0;
        void* UpdatedResource = EntityManager.VectorInstanceData.Memory;
        UpdateInstanceData(EntityManager.VectorInstanceResourceKey, UpdatedResource, sizeof(vector_instance_data),
            EntityManager.VectorEntitysCount, ResourceOffset, EntityManager.VectorUpdateTypes);
    }
    PushDrawCommand(0, EntityManager.VectorInstanceResourceKey, EntityManager.VectorMesh, EntityManager.VectorPipeline);
    EntityManager.VectorUpdateTypes = UPDATE_RESOURCE_NONE;
    simulation_cube* Cube = &EntityManager.Cube;
    if (Cube->IsBeingSimulated)
    {
        if (Cube->AffectedByGravity)
            ApplyGravity(Cube);
        SimulateCube(Cube);
    }
    PushDrawCommand(EntityManager.CubeResourceKey, 0, EntityManager.CubeMesh, EntityManager.CubePipeline);
}

static inline bool CanCreateVector()
{
    return (EntityManager.VectorEntitysCount + 1) < MAX_VECTORS;
}
