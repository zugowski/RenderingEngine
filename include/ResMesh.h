#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <string>
#include <vector>

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
    {}

    static const D3D12_INPUT_LAYOUT_DESC InputLayout;

private:
    static const int InputElementCount = 4;
    static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
};

struct ResMesh
{
    std::vector<MeshVertex> Vertices;
    std::vector<uint32_t>   Indices;
    uint32_t                MaterialId;
};

bool LoadMesh(
    const wchar_t*              filename,
    std::vector<ResMesh>&       meshes,
    std::vector<ResMaterial>&   materials);
