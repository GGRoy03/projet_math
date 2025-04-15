cbuffer SharedData : register(b0)
{
    row_major matrix View;
    row_major matrix Projection;
}

cbuffer EntityData : register(b1)
{
    row_major matrix World;
}

struct VS_INPUT
{
    float3 Pos : POSITION;
    float2 TextureCoord : TEXCOORD;
    float3 Normal : NORMAL;
    uint InstanceID : SV_InstanceID;
};

struct VS_OUTPUT
{
    float4 Pos : SV_Position;
    float3 Normal : NORMAL;
    float3 FragmentPosition : TEXCOORD1;
};

struct cell_data
{
    float3 Position;
};

StructuredBuffer<cell_data> CellInstanceData : register(t0);

VS_OUTPUT main(VS_INPUT Input)
{
    VS_OUTPUT Output;
    
    cell_data InstanceData = CellInstanceData[Input.InstanceID];
    
    float4 WorldPosition  = mul(World, float4(Input.Pos + InstanceData.Position, 1.0f));
    float4 VertexPosition = mul(View, WorldPosition);
    VertexPosition        = mul(Projection, VertexPosition);
    
    Output.Pos              = VertexPosition;
    Output.Normal           = Input.Normal;
    Output.FragmentPosition = WorldPosition.xyz;
    
    return Output;
}