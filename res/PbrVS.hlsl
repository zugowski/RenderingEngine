struct VSInput
{
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD;
    float3 Tangent  : TANGENT;
};

struct VSOutput
{
    float4 Position          : SV_POSITION;
    float3 Normal            : NORMAL;
    float2 TexCoord          : TEXCOORD;
    float4 WorldPos          : WORLD_POS;
    float3x3 InvTangentBasis : INV_TANGENT_BASIS;
};

cbuffer Transform : register(b0)
{
    float4x4 World : packoffset(c0);
    float4x4 View  : packoffset(c4);
    float4x4 Proj  : packoffset(c8);
}

VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;

    float4 localPos = float4(input.Position, 1.0f);
    float4 worldPos = mul(World, localPos);
    float4 viewPos = mul(View, worldPos);
    float4 projPos = mul(Proj, viewPos);

    output.Position = projPos;
    output.TexCoord = input.TexCoord;
    output.WorldPos = worldPos;

    float3 N = normalize(mul((float3x3) World, input.Normal));
    float3 T = normalize(mul((float3x3) World, input.Tangent));
    float3 B = normalize(cross(N, T));
    
    output.InvTangentBasis = transpose(float3x3(T, B, N));
    output.Normal = N;
    
    return output;
}