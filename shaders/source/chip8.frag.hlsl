struct VS_Output
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD0;
};

Texture2D Chip8Texture : register(t0);
SamplerState Chip8Sampler : register(s0);

float4 main(VS_Output input) : SV_Target
{
    return Chip8Texture.Sample(Chip8Sampler, input.TexCoord);
}