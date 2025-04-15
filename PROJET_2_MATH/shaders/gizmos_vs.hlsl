cbuffer SharedData : register(b0)
{
    row_major matrix View;
    row_major matrix Projection;
}

struct VS_INPUT
{
    float3 Pos          : POSITION;
    float2 TextureCoord : TEXCOORD;
    float3 Normal       : NORMAL;
    uint InstanceID     : SV_InstanceID;
};

struct VS_OUTPUT
{
    float4 Position : SV_Position;
    float4 Color     : COLOR;
};

struct gizmo_instance_data
{
    row_major matrix Transform;
    float4           Color;
};

StructuredBuffer<gizmo_instance_data> GizmoInstanceData : register(t0);

VS_OUTPUT main(VS_INPUT Input)
{
    VS_OUTPUT Output;
    
    gizmo_instance_data InstanceData = GizmoInstanceData[Input.InstanceID];
    
    float4 VertexPosition = mul(InstanceData.Transform, float4(Input.Pos, 1.0f));
    VertexPosition        = mul(View, VertexPosition);
    VertexPosition        = mul(Projection, VertexPosition);
    
    Output.Color    = InstanceData.Color;
    Output.Position = VertexPosition;
    
    return Output;
}