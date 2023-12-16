#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <string>
#include <vector>

#define MAX_INFLUENCE_BONE_COUNT  4

struct TexturePath
{
    std::wstring DiffuseMap;
    std::wstring SpecularMap;
    std::wstring ShininessMap;
    std::wstring NormalMap;

    TexturePath()
        : DiffuseMap    (L"")
        , SpecularMap   (L"")
        , ShininessMap  (L"")
        , NormalMap     (L"")
    {}
};

struct TextureData
{
    uint8_t* DiffuseTex;
    int      DiffuseTexSize;
    uint8_t* SpecularTex;
    int      SpecularTexSize;
    uint8_t* ShininessTex;
    int      ShininessTexSize;
    uint8_t* NormalTex;
    int      NormalTexSize;

    TextureData()
        : DiffuseTex        (nullptr)
        , DiffuseTexSize    (0)
        , SpecularTex       (nullptr)
        , SpecularTexSize   (0)
        , ShininessTex      (nullptr)
        , ShininessTexSize  (0)
        , NormalTex         (nullptr)
        , NormalTexSize     (0)
    {}
};

struct ResMaterial
{
    DirectX::XMFLOAT3   Diffuse;
    DirectX::XMFLOAT3   Specular;
    float               Alpha;
    float               Shininess;
    struct TexturePath  TexturePath;
    struct TextureData  TextureData;

    ResMaterial()
        : Diffuse   (0.0f, 0.0f, 0.0f)
        , Specular  (0.0f, 0.0f, 0.0f)
        , Alpha     (0.0f)
        , Shininess (0.0f)
    {}
};

class MeshVertex
{
public:
    DirectX::XMFLOAT3   Position;
    DirectX::XMFLOAT3   Normal;
    DirectX::XMFLOAT2   TexCoord;
    DirectX::XMFLOAT3   Tangent;

    int   BoneIDs[MAX_INFLUENCE_BONE_COUNT];
    float BoneWeights[MAX_INFLUENCE_BONE_COUNT];

    MeshVertex() = default;

    MeshVertex(
        DirectX::XMFLOAT3 const& position,
        DirectX::XMFLOAT3 const& normal,
        DirectX::XMFLOAT2 const& texcoord,
        DirectX::XMFLOAT3 const& tangent)
        : Position  (position)
        , Normal    (normal)
        , TexCoord  (texcoord)
        , Tangent   (tangent)
    {
        for (int i = 0; i < MAX_INFLUENCE_BONE_COUNT; ++i)
        {
            BoneIDs[i] = -1;
            BoneWeights[i] = 0.0f;
        }
    }

    void SetVertexBoneData(int id, float weight)
    {
        for (int i = 0; i < MAX_INFLUENCE_BONE_COUNT; ++i)
        {
            if (BoneIDs[i] < 0)
            {
                BoneIDs[i] = id;
                BoneWeights[i] = weight;
            }
        }
    }

    static const D3D12_INPUT_LAYOUT_DESC InputLayout;

private:
    static const int InputElementCount = 4;
    static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
};

struct BoneInfo
{
    int Id;
    DirectX::XMMATRIX Offset;
};

struct ResMesh
{
    std::vector<MeshVertex> Vertices;
    std::vector<uint32_t>   Indices;
    uint32_t                MaterialId;
    std::vector<BoneInfo>   BonesInfo;
};

bool LoadMesh(
    const wchar_t*              filename,
    std::vector<ResMesh>&       meshes,
    std::vector<ResMaterial>&   materials);
