struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
    float4 WorldPos : WORLD_POS;
    float3x3 InvTangentBasis : INV_TANGENT_BASIS;
};

struct PSOutput
{
    float4 Color : SV_TARGET0;
};

cbuffer LightBuffer : register(b1)
{
    float3 CameraPosition : packoffset(c0);
    
    float3 DirLightDirection : packoffset(c1);
    float3 DirLightColor : packoffset(c2);
    
    float3 PtLightPosition : packoffset(c3);
    float3 PtLightColor : packoffset(c4);
    float PtLightRange : packoffset(c4.w);
    
    float3 SptLightPosition : packoffset(c5);
    float3 SptLightColor : packoffset(c6);
    float SptLightRange : packoffset(c6.w);
    float3 SptLightDirection : packoffset(c7);
    float SptAngle : packoffset(c7.w);
}

cbuffer MaterialBuffer : register(b2)
{
    float3 Diffuse : packoffset(c0);
    float Alpha : packoffset(c0.w);
    float3 Specular : packoffset(c1);
    float Shininess : packoffset(c1.w);
}

SamplerState ColorSmp : register(s0);
Texture2D ColorMap : register(t0);
Texture2D NormalMap : register(t1);

float3 GetLightFromDirLight(VSOutput input, float3 normal);
float3 GetLightFromPointLight(VSOutput input, float3 normal);
float3 GetLightFromSpotLight(VSOutput input, float3 normal);

// diffuse
float3 ComputeLambert(
    float3 lightDir,
    float3 lightColor,
    float3 normal);

// specular
float3 ComputePhong(
    float4 worldPos,
    float3 lightDir,
    float3 lightColor,
    float3 normal);

PSOutput main(VSOutput input)
{
    PSOutput output = (PSOutput) 0;

    // Normal vector
    float3 N = NormalMap.Sample(ColorSmp, input.TexCoord).xyz * 2.0f - 1.0f;
    N = mul(input.InvTangentBasis, N);
    
    float3 dirLight = GetLightFromDirLight(input, N);
    float3 ptLight = GetLightFromPointLight(input, N);
    float3 sptLight = GetLightFromSpotLight(input, N);
    
    float4 color = ColorMap.Sample(ColorSmp, input.TexCoord);
    
    float light = dirLight + ptLight + sptLight;
    output.Color = float4(color.rgb * light, color.a * Alpha);
    return output;
}

float3 GetLightFromDirLight(VSOutput input, float3 normal)
{
    float3 diffuse = ComputeLambert(-DirLightDirection, DirLightColor, normal);
    float3 specular = ComputePhong(input.WorldPos, -DirLightDirection, DirLightColor, normal);
    
    return diffuse + specular;
}

float3 GetLightFromPointLight(VSOutput input, float3 normal)
{
    float3 L = normalize(input.WorldPos.xyz - PtLightPosition);
    
    float3 diffuse = ComputeLambert(L, PtLightColor, normal);
    float3 specular = ComputePhong(input.WorldPos, L, PtLightColor, normal);
    
    float dist = length(input.WorldPos.xyz - PtLightPosition);
    float affect = saturate(1.0f - 1.0f / PtLightRange * dist);
    affect = pow(affect, 3.0f);
    
    diffuse *= affect;
    specular *= affect;
    
    return diffuse + specular;
}

float3 GetLightFromSpotLight(VSOutput input, float3 normal)
{
    float3 L = normalize(input.WorldPos.xyz - SptLightPosition);
    
    float3 diffuse = ComputeLambert(L, SptLightColor, normal);
    float3 specular = ComputePhong(input.WorldPos, L, SptLightColor, normal);
    
    float dist = length(input.WorldPos.wyz - SptLightPosition);
    float affect = saturate(1.0f - 1.0f / SptLightRange * dist);
    affect = pow(affect, 3.0f);
    
    diffuse *= affect;
    specular *= affect;
    
    float angle = dot(L, SptLightDirection);
    angle = abs(acos(angle));
    affect = saturate(1.0f - 1.0f / SptAngle * angle);
    
    affect = pow(affect, 0.5f);
    
    diffuse *= affect;
    specular *= affect;
    
    return diffuse + specular;
}

float3 ComputeLambert
(
    float3 lightDir,
    float3 lightColor,
    float3 normal
)
{
    return lightColor * Diffuse * saturate(dot(lightDir, normal) * -1);
}

float3 ComputePhong
(
    float4 worldPos,
    float3 lightDir,
    float3 lightColor,
    float3 normal
)
{
    float3 V = normalize(CameraPosition - worldPos.xyz);
    float3 R = normalize(reflect(lightDir, normal));
    return lightColor * Specular * pow(saturate(dot(V, R)), Shininess);
}