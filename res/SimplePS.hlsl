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

struct Light
{
    float3 Color;     // 빛의 세기
    float Range;      // 포인트/스포트라이트 전용
    float3 Direction; // 디렉셔널/스포트라이트 전용
    float SpotPower;  // 스포트라이트 전용
    float3 Position;  // 포인트라이트 전용
    float pad;
};

cbuffer LightBuffer : register(b1)
{
    Light DirLight;
    Light PointLight;
    Light SpotLight;
}

cbuffer MaterialBuffer : register(b2)
{
    float3 Diffuse  : packoffset(c0);
    float  Alpha    : packoffset(c0.w);
    float3 Specular : packoffset(c1);
    float Shininess : packoffset(c1.w);
}

cbuffer PassConstantBuffer : register(b3)
{
    float3 CameraPosition  : packoffset(c0);
    float4 AmbientLight    : packoffset(c1);
    int DiffuseMapUsable   : packoffset(c2.x);
    int SpecularMapUsable  : packoffset(c2.y);
    int ShininessMapUsable : packoffset(c2.z);
    int NormalMapUsable    : packoffset(c2.w);
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
    float3 diffuse = ComputeLambert(-DirLight.Direction, DirLight.Color, normal);
    float3 specular = ComputePhong(g_Input.WorldPos, -DirLight.Direction, DirLight.Color, normal);
    
    return diffuse + specular;
}

float3 GetLightFromPointLight(float3 normal)
{
    float3 L = normalize(g_Input.WorldPos.xyz - PointLight.Position);
    
    float3 diffuse = ComputeLambert(L, PointLight.Color, normal);
    float3 specular = ComputePhong(g_Input.WorldPos, L, PointLight.Color, normal);
    
    float dist = length(g_Input.WorldPos.xyz - PointLight.Position);
    float affect = saturate(1.0f - 1.0f / PointLight.Range * dist);
    affect = pow(affect, 3.0f);
    
    diffuse *= affect;
    specular *= affect;
    
    return diffuse + specular;
}

float3 GetLightFromSpotLight(float3 normal)
{
    float3 L = normalize(g_Input.WorldPos.xyz - SpotLight.Position);
    
    float3 diffuse = ComputeLambert(L, SpotLight.Color, normal);
    float3 specular = ComputePhong(g_Input.WorldPos, L, SpotLight.Color, normal);
    
    float dist = length(g_Input.WorldPos.wyz - SpotLight.Position);
    float affect = saturate(1.0f - 1.0f / SpotLight.Range * dist);
    //affect = pow(affect, 3.0f);
    
    diffuse *= affect;
    specular *= affect;
    
    float spotFactor = pow(max(dot(-L, normalize(SpotLight.Direction)), 0.0f), SpotLight.SpotPower);

    diffuse *= spotFactor;
    specular *= spotFactor;
    
    //float angle = dot(L, SpotLight.Direction);
    //angle = abs(acos(angle));
    //affect = saturate(1.0f - 1.0f / SptAngle * angle);
    //affect = pow(affect, 0.5f);
    //diffuse *= affect;
    //specular *= affect;
    
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