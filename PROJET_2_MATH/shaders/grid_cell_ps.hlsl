cbuffer MaterialData : register(b0)
{
    float3 Ambient;
    float3 Diffuse;
    float3 Specular;
    float3 Emissive;

    float Optical;
    float Dissolve;
    float SpecularExponent;
}


struct PS_INPUT
{
    float4 Pos : SV_Position;
    float3 Normal : NORMAL;
    float3 FragmentPosition : TEXCOORD1;
};


float4 main(PS_INPUT Input) : SV_Target
{
    return float4(0.35f, 0.35f, 0.35f, 1.0f); // Force to red
}