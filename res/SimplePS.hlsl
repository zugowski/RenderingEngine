struct VSOutput
{
    float4 Position          : SV_POSITION;
    float3 Normal            : NORMAL;
    float2 TexCoord          : TEXCOORD;
    float4 WorldPos          : WORLD_POS;
    float3x3 InvTangentBasis : INV_TANGENT_BASIS;
};

struct PSOutput
{
    float4 Color : SV_TARGET0;
};

cbuffer LightBuffer : register(b1)
{
    float3 CameraPosition    : packoffset(c0);
    
    float3 DirLightDirection : packoffset(c1);
    float3 DirLightColor     : packoffset(c2);
    
    float3 PtLightPosition   : packoffset(c3);
    float3 PtLightColor      : packoffset(c4);
    float PtLightRange       : packoffset(c4.w);
    
    float3 SptLightPosition  : packoffset(c5);
    float3 SptLightColor     : packoffset(c6);
    float SptLightRange      : packoffset(c6.w);
    float3 SptLightDirection : packoffset(c7);
    float SptAngle           : packoffset(c7.w);
}

cbuffer MaterialBuffer : register(b2)
{
    float3 Diffuse  : packoffset(c0);
    float  Alpha    : packoffset(c0.w);
    float3 Specular : packoffset(c1);
    float Shininess : packoffset(c1.w);
}

cbuffer ConfigureBuffer : register(b3)
{
    int DiffuseMapUsable   : packoffset(c0.x);
    int SpecularMapUsable  : packoffset(c0.y);
    int ShininessMapUsable : packoffset(c0.z);
    int NormalMapUsable    : packoffset(c0.w);
}

SamplerState ColorSmp    : register(s0);
Texture2D    ColorMap    : register(t0);
Texture2D    NormalMap   : register(t1);
Texture2D    SpecularMap : register(t2);

float3 GetLightFromDirLight(float3 normal);
float3 GetLightFromPointLight(float3 normal);
float3 GetLightFromSpotLight(float3 normal);

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

static VSOutput g_Input;

PSOutput main(VSOutput input)
{
    PSOutput output = (PSOutput) 0;
    g_Input = input;
    
    // Normal vector
    float3 N;
    
    if(NormalMapUsable)
    {
        N = NormalMap.Sample(ColorSmp, input.TexCoord).xyz * 2.0f - 1.0f;
        N = mul(input.InvTangentBasis, N);
    }
    else
    {
        N = input.Normal;
    }

    float3 dirLight = GetLightFromDirLight(N);
    float3 ptLight  = GetLightFromPointLight(N);
    float3 sptLight = GetLightFromSpotLight(N);
    
    float4 color;
    
    if(DiffuseMapUsable)
    {
        color = ColorMap.Sample(ColorSmp, input.TexCoord);
    }
    else
    {
        color = float4(1.0f, 0.0f, 0.0f, 1.0f);
    }
    
    float3 light = dirLight + ptLight + sptLight;
    output.Color = float4(color.rgb * light, color.a * Alpha);
    return output;
}

float3 GetLightFromDirLight(float3 normal)
{
    float3 diffuse = ComputeLambert(-DirLightDirection, DirLightColor, normal);
    float3 specular = ComputePhong(g_Input.WorldPos, -DirLightDirection, DirLightColor, normal);
    
    return diffuse + specular;
}

float3 GetLightFromPointLight(float3 normal)
{
    float3 L = normalize(g_Input.WorldPos.xyz - PtLightPosition);
    
    float3 diffuse = ComputeLambert(L, PtLightColor, normal);
    float3 specular = ComputePhong(g_Input.WorldPos, L, PtLightColor, normal);
    
    float dist = length(g_Input.WorldPos.xyz - PtLightPosition);
    float affect = saturate(1.0f - 1.0f / PtLightRange * dist);
    affect = pow(affect, 3.0f);
    
    diffuse *= affect;
    specular *= affect;
    
    return diffuse + specular;
}

float3 GetLightFromSpotLight(float3 normal)
{
    float3 L = normalize(g_Input.WorldPos.xyz - SptLightPosition);
    
    float3 diffuse = ComputeLambert(L, SptLightColor, normal);
    float3 specular = ComputePhong(g_Input.WorldPos, L, SptLightColor, normal);
    
    float dist = length(g_Input.WorldPos.wyz - SptLightPosition);
    float affect = saturate(1.0f - 1.0f / SptLightRange * dist);
    affect = pow(affect, 3.0f);
    
    diffuse *= affect;
    specular *= affect;
    
    float angle = dot(L, SptLightDirection);
    angle = abs(acos(angle));
    affect = saturate(1.0f - 1.0f / SptAngle * angle);
    
    //affect = pow(affect, 0.5f);
    
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
    
    float3 S = Specular;
    if (SpecularMapUsable)
        S = SpecularMap.Sample(ColorSmp, g_Input.TexCoord).xyz;

    return lightColor * S * pow(saturate(dot(V, R)), Shininess);
}