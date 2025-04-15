struct PS_INPUT
{
    float4 Position : SV_Position;
    float4 Color    : COLOR;
};

float4 main(PS_INPUT Input) : SV_Target
{
    return Input.Color;
}
