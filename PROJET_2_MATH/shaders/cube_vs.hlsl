cbuffer SharedData : register(b0)
{
    row_major matrix View;
    row_major matrix Projection;
}

cbuffer ObjectData : register(b1)
{
    row_major matrix Transform;
    float            Time;
}

struct VS_INPUT
{
    float3 Pos : POSITION;
    float2 TextureCoord : TEXCOORD;
    float3 Normal : NORMAL;
};

struct VS_OUTPUT
{
    float4 Position : SV_Position;
    float3 WorldPos : TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT Input)
{
    VS_OUTPUT Output;
    
    // Compute world position
    float4 worldPos = mul(Transform, float4(Input.Pos, 1.0f));

    // Pass worldPos.xyz to pixel shader
    Output.WorldPos = worldPos.xyz;

    // Standard view+projection transform for the output
    float4 viewPos = mul(View, worldPos);
    Output.Position = mul(Projection, viewPos);
    
    return Output;
}
