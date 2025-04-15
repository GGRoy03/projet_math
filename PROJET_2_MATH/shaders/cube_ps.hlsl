cbuffer ObjectData : register(b1)
{
    row_major matrix Transform;
    float Time;
}

struct PS_INPUT
{
    float4 Position : SV_Position;
    float3 WorldPos : TEXCOORD0;
};

float4 main(PS_INPUT Input) : SV_Target
{
    // Use XZ plane for swirl effect.
    float2 posXZ = Input.WorldPos.xz;
    float2 center = float2(0.0, 0.0);
    float2 offset = posXZ - center;
    float radius = length(offset);
    float angle = atan2(offset.y, offset.x);

    float swirlSpeed = 1.5;
    float swirlFrequency = 3.0;
    float swirlAmplitude = 0.2;

    float swirlAngle = sin(Time * swirlSpeed + radius * swirlFrequency) * swirlAmplitude;
    angle += swirlAngle;
    float2 swirlPos = float2(cos(angle), sin(angle)) * radius;

    // Slightly brighter base colors.
    float3 baseColor;
    baseColor.r = 0.55 + 0.15 * sin(Time * 0.8 + swirlPos.x * 2.0);
    baseColor.g = 0.55 + 0.15 * sin(Time * 1.0 + swirlPos.y * 2.0 + 1.0);
    baseColor.b = 0.55 + 0.15 * sin(Time * 1.2 + radius * 2.0 + 2.0);

    // Reduced falloff: use a gentle multiplier and add a minimum brightness constant.
    float brightnessFalloff = saturate(1.0 - radius * 0.1) + 0.3;
    float3 finalColor = baseColor * brightnessFalloff;
    
    return float4(finalColor, 1.0);
}
