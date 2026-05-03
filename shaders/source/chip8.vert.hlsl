struct VS_Input
{
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
};

struct VS_Output
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD0;
};

VS_Output main(VS_Input input)
{
    VS_Output output;
    output.Position = float4(input.Position, 1.0);
    output.TexCoord = input.TexCoord;
    return output;
}