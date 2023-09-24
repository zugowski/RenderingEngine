static const float PI = 3.1415926f;

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
    float  PtLightRange      : packoffset(c4.w);
    
    float3 SptLightPosition  : packoffset(c5);
    float3 SptLightColor     : packoffset(c6);
    float  SptLightRange     : packoffset(c6.w);
    float3 SptLightDirection : packoffset(c7);
    float  SptAngle          : packoffset(c7.w);
}

cbuffer MaterialBuffer : register(b2)
{
    float3 Diffuse  : packoffset(c0);
    float Alpha     : packoffset(c0.w);
    float3 Specular : packoffset(c1);
    float Shininess : packoffset(c1.w);
}

cbuffer ConfigureBuffer : register(b3)
{
    int DiffuseMapUsable      : packoffset(c0.x);
    int SpecularMapUsable     : packoffset(c0.y);
    int ShininessMapUsable    : packoffset(c0.z);
    int NormalMapUsable       : packoffset(c0.w);
    
    int PbrAlbedoMapUsable    : packoffset(c1.x);
    int PbrNormalMapUsable    : packoffset(c1.y);
    int PbrMetalicMapUsable   : packoffset(c1.z);
    int PbrRoughnessMapUsable : packoffset(c1.w);
}

SamplerState ColorSmp  : register(s0);
Texture2D AlbedoMap    : register(t0);
Texture2D NormalMap    : register(t1);
Texture2D MetalicMap   : register(t2);
Texture2D RoughnessMap : register(t3);

float3 GetLightFromDirLight(float3 normal, float3 specColor, float metallic);

// diffuse
float ComputeFrensel(
    float4 worldPos,
    float3 lightDir,
    float3 lightColor,
    float3 normal);

// diffuse
float3 ComputeLambert(
    float3 lightDir,
    float3 lightColor,
    float3 normal);

float Beckmann(float m, float t);
float SpcFresnel(float f0, float u);

// specular
float CookTorrance(
    float4 worldPos,
    float3 lightDir,
    float3 lightColor,
    float3 normal,
    float  metallic
);

static VSOutput g_Input;

PSOutput main(VSOutput input)
{
    PSOutput output = (PSOutput) 0;
    g_Input = input;

    // Normal vector
    float3 N;
    
    if (NormalMapUsable)
    {
        N = NormalMap.Sample(ColorSmp, input.TexCoord).xyz * 2.0f - 1.0f;
        N = mul(input.InvTangentBasis, N);
    }
    else
    {
        N = input.Normal;
    }
    
    float4 albedoColor = AlbedoMap.Sample(ColorSmp, input.TexCoord);
    float3 specColor = albedoColor;
    float smooth = MetalicMap.Sample(ColorSmp, input.TexCoord).a;
    float metallic = MetalicMap.Sample(ColorSmp, g_Input.TexCoord).r;
        
    float3 dirLight = GetLightFromDirLight(N, specColor, 100);
    
    float3 light = dirLight;
    output.Color = float4(albedoColor.rgb * light, albedoColor.a * Alpha);
    
    return output;
}

float3 GetLightFromDirLight(float3 normal, float3 specColor, float metallic)
{
    float diffuseFrensel = ComputeFrensel(g_Input.WorldPos, -DirLightDirection, DirLightColor, normal);
    float diffuseLambert = ComputeLambert(-DirLightDirection, DirLightColor, normal);
    float specular = CookTorrance(g_Input.WorldPos, -DirLightDirection, DirLightColor, normal, metallic);
    
    specular *= Specular * lerp(float3(1.0f, 1.0f, 1.0f), specColor, metallic);
    
    return DirLightColor * ((diffuseFrensel * diffuseLambert) + specular);
}

float ComputeFrensel
(
    float4 worldPos,
    float3 lightDir,
    float3 lightColor,
    float3 normal
)
{
    float3 V = normalize(CameraPosition - worldPos.xyz);
    return lightColor * Diffuse * saturate(dot(normal, lightDir)) * saturate(dot(normal, V));
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

float Beckmann(float m, float t)
{
    float t2 = t * t;
    float t4 = t * t * t * t;
    float m2 = m * m;
    float D = 1.0f / (4.0f * m2 * t4);
    D *= exp((-1.0f / m2) * (1.0f - t2) / t2);
    return D;
}

float SpcFresnel(float f0, float u)
{
    return f0 + (1 - f0) * pow(1 - u, 5);
}

float CookTorrance
(
    float4 worldPos,
    float3 lightDir,
    float3 lightColor,
    float3 normal,
    float  metallic
)
{
    float microfacet = 0.76f;

    float f0 = metallic;

    float3 V = normalize(CameraPosition - worldPos.xyz);
    float3 H = normalize(lightDir + V);

    float NdotH = saturate(dot(normal, H));
    float VdotH = saturate(dot(V, H));
    float NdotL = saturate(dot(normal, lightDir));
    float NdotV = saturate(dot(normal, V));
    
    float D = Beckmann(microfacet, NdotH);

    float F = SpcFresnel(f0, VdotH);

    float G = min(1.0f, min(2 * NdotH * NdotV / VdotH, 2 * NdotH * NdotL / VdotH));

    float m = PI * NdotV * NdotH;

    return max(F * D * G / m, 0.0);
}