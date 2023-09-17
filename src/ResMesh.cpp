#include "ResMesh.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <codecvt>
#include <cassert>
#include <limits>

namespace {
    std::string ToUTF8(const std::wstring& value)
    {
        auto length = WideCharToMultiByte(
            CP_UTF8, 0U, value.data(), -1, nullptr, 0, nullptr, nullptr);
        auto buffer = new char[length];

        WideCharToMultiByte(
            CP_UTF8, 0U, value.data(), -1, buffer, length, nullptr, nullptr);

        std::string result(buffer);
        delete[] buffer;
        buffer = nullptr;

        return result;
    }

    std::wstring Convert(const aiString& path)
    {
        wchar_t temp[256] = {};
        size_t  size;
        mbstowcs_s(&size, temp, path.C_Str(), 256);
        return std::wstring(temp);
    }

    class MeshLoader
    {
    public:
        MeshLoader();
        ~MeshLoader();

        bool Load(
            const wchar_t* filename,
            std::vector<ResMesh>& meshes,
            std::vector<ResMaterial>& materials);

        void Normalize(std::vector<ResMesh>& meshes);

    private:
        const aiScene* m_pScene = nullptr;

        void ParseMesh(ResMesh& dstMesh, const aiMesh* pSrcMesh);
        void ParseMaterial(ResMaterial& dstMaterial, const aiScene* pScene, const aiMaterial* pSrcMaterial);
    };

    MeshLoader::MeshLoader()
        : m_pScene(nullptr)
    {
    }

    MeshLoader::~MeshLoader()
    {
    }

    bool MeshLoader::Load
    (
        const wchar_t*              filename,
        std::vector<ResMesh>&       meshes,
        std::vector<ResMaterial>&   materials
    )
    {
        if (filename == nullptr)
        {
            return false;
        }

        auto path = ToUTF8(filename);

        Assimp::Importer importer;
        unsigned int flag = 0;
        flag |= aiProcess_Triangulate;
        flag |= aiProcess_PreTransformVertices;
        flag |= aiProcess_CalcTangentSpace;
        flag |= aiProcess_GenSmoothNormals;
        flag |= aiProcess_GenUVCoords;
        flag |= aiProcess_RemoveRedundantMaterials;
        flag |= aiProcess_OptimizeMeshes;
        flag |= aiProcess_ConvertToLeftHanded;

        m_pScene = importer.ReadFile(path, flag);

        if (m_pScene == nullptr)
        {
            return false;
        }

        meshes.clear();
        meshes.resize(m_pScene->mNumMeshes);

        for (size_t i = 0; i < meshes.size(); ++i)
        {
            const auto pMesh = m_pScene->mMeshes[i];
            ParseMesh(meshes[i], pMesh);
        }

        // 좌표 -1.0 ~ 1.0로 정규화
        // TODO: 메쉬 볼륨이 망가지는 버그가 있는데 수정 필요함
        // Normalize(meshes);

        materials.clear();
        materials.resize(m_pScene->mNumMaterials);

        for (size_t i = 0; i < materials.size(); ++i)
        {
            const auto pMaterial = m_pScene->mMaterials[i];
            ParseMaterial(materials[i], m_pScene, pMaterial);
        }

        importer.FreeScene();
        m_pScene = nullptr;
        
        return true;
    }

    void MeshLoader::Normalize(std::vector<ResMesh>& meshes)
    {
        float xmax, ymax, zmax;
        float xmin, ymin, zmin;

        xmax = ymax = zmax = FLT_MIN;
        xmin = ymin = zmin = FLT_MAX;

        for (auto& mesh : meshes)
        {
            for (int i = 0; i < mesh.Vertices.size(); ++i)
            {
                const MeshVertex vertex = mesh.Vertices[i];
                xmax = std::max(xmax, vertex.Position.x);
                xmin = std::min(xmin, vertex.Position.x);
                ymax = std::max(ymax, vertex.Position.y);
                ymin = std::min(ymin, vertex.Position.y);
                zmax = std::max(zmax, vertex.Position.z);
                zmin = std::min(zmin, vertex.Position.z);
            }
        }

        const float xDivisor = xmax - xmin;
        const float yDivisor = ymax - ymin;
        const float zDivisor = zmax - zmin;

        for (auto& mesh : meshes)
        {
            for (int i = 0; i < mesh.Vertices.size(); ++i)
            {
                MeshVertex& vertex = mesh.Vertices[i];
                vertex.Position.x = (2.0f * (vertex.Position.x - xmin) / xDivisor) - 1.0f;
                vertex.Position.y = (2.0f * (vertex.Position.y - ymin) / yDivisor) - 1.0f;
                vertex.Position.z = (2.0f * (vertex.Position.z - zmin) / zDivisor) - 1.0f;
            }
        }
    }

    void MeshLoader::ParseMesh(ResMesh& dstMesh, const aiMesh* pSrcMesh)
    {
        dstMesh.MaterialId = pSrcMesh->mMaterialIndex;

        aiVector3D zero3D(0.0f, 0.0f, 0.0f);

        dstMesh.Vertices.resize(pSrcMesh->mNumVertices);

        for (auto i = 0u; i < pSrcMesh->mNumVertices; ++i)
        {
            auto pPosition = &(pSrcMesh->mVertices[i]);
            auto pNormal = &(pSrcMesh->mNormals[i]);
            auto pTexCoord = (pSrcMesh->HasTextureCoords(0)) ? &(pSrcMesh->mTextureCoords[0][i]) : &zero3D;
            auto pTangent = (pSrcMesh->HasTangentsAndBitangents()) ? &(pSrcMesh->mTangents[i]) : &zero3D;

            dstMesh.Vertices[i] = MeshVertex(
                DirectX::XMFLOAT3(pPosition->x, pPosition->y, pPosition->z),
                DirectX::XMFLOAT3(pNormal->x, pNormal->y, pNormal->z),
                DirectX::XMFLOAT2(pTexCoord->x, pTexCoord->y),
                DirectX::XMFLOAT3(pTangent->x, pTangent->y, pTangent->z)
            );
        }

        dstMesh.Indices.resize(pSrcMesh->mNumFaces * 3);

        for (auto i = 0u; i < pSrcMesh->mNumFaces; ++i)
        {
            const auto& face = pSrcMesh->mFaces[i];
            assert(face.mNumIndices == 3);

            dstMesh.Indices[i * 3 + 0] = face.mIndices[0];
            dstMesh.Indices[i * 3 + 1] = face.mIndices[1];
            dstMesh.Indices[i * 3 + 2] = face.mIndices[2];
        }
    }

    void MeshLoader::ParseMaterial(ResMaterial& dstMaterial, const aiScene* pScene, const aiMaterial* pSrcMaterial)
    {
        {
            aiColor3D color(0.0f, 0.0f, 0.0f);

            if (pSrcMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
            {
                dstMaterial.Diffuse.x = color.r;
                dstMaterial.Diffuse.y = color.g;
                dstMaterial.Diffuse.z = color.b;
            }
            else
            {
                dstMaterial.Diffuse.x = 0.5f;
                dstMaterial.Diffuse.y = 0.5f;
                dstMaterial.Diffuse.z = 0.5f;
            }
        }

        {
            aiColor3D color(0.0f, 0.0f, 0.0f);

            if (pSrcMaterial->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS)
            {
                dstMaterial.Specular.x = color.r;
                dstMaterial.Specular.y = color.g;
                dstMaterial.Specular.z = color.b;
            }
            else
            {
                dstMaterial.Specular.x = 0.0f;
                dstMaterial.Specular.y = 0.0f;
                dstMaterial.Specular.z = 0.0f;
            }
        }

        {
            auto shininess = 0.0f;
            if (pSrcMaterial->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS)
            {
                dstMaterial.Shininess = shininess;
            }
            else
            {
                dstMaterial.Shininess = 0.0f;
            }
        }

        {
            aiString path;
            if (pSrcMaterial->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), path) == AI_SUCCESS)
            {
                dstMaterial.TexturePath.DiffuseMap = Convert(path);

                const auto texture = pScene->GetEmbeddedTexture(path.C_Str());
                if (texture != nullptr)
                {
                    if (texture->mHeight != 0)
                    {
                        // TODO: Handle when texture->mHeight = 0
                    }
                    else
                    {
                        dstMaterial.TextureData.DiffuseTex = (uint8_t*)malloc(texture->mWidth);
                        if (dstMaterial.TextureData.DiffuseTex != nullptr)
                        {
                            memcpy(dstMaterial.TextureData.DiffuseTex, texture->pcData, texture->mWidth);
                            dstMaterial.TextureData.DiffuseTexSize = texture->mWidth;
                        }
                    }
                }
                else
                {
                    // TODO: regular file, check if it exists and read it
                }
            }
            else
            {
                dstMaterial.TexturePath.DiffuseMap.clear();
            }
        }

        {
            aiString path;
            if (pSrcMaterial->Get(AI_MATKEY_TEXTURE_SPECULAR(0), path) == AI_SUCCESS)
            {
                dstMaterial.TexturePath.SpecularMap = Convert(path);

                const auto texture = pScene->GetEmbeddedTexture(path.C_Str());
                if (texture != nullptr)
                {
                    if (texture->mHeight != 0)
                    {
                        // TODO: Handle when texture->mHeight = 0
                    }
                    else
                    {
                        dstMaterial.TextureData.SpecularTex = (uint8_t*)malloc(texture->mWidth);
                        if (dstMaterial.TextureData.SpecularTex != nullptr)
                        {
                            memcpy(dstMaterial.TextureData.SpecularTex, texture->pcData, texture->mWidth);
                            dstMaterial.TextureData.SpecularTexSize = texture->mWidth;
                        }
                    }
                }
                else
                {
                    // TODO: regular file, check if it exists and read it
                }
            }
            else
            {
                dstMaterial.TexturePath.SpecularMap.clear();
            }
        }

        {
            aiString path;
            if (pSrcMaterial->Get(AI_MATKEY_TEXTURE_SHININESS(0), path) == AI_SUCCESS)
            {
                dstMaterial.TexturePath.ShininessMap = Convert(path);
            }
            else
            {
                dstMaterial.TexturePath.ShininessMap.clear();
            }
        }

        {
            aiString path;
            if (pSrcMaterial->Get(AI_MATKEY_TEXTURE_NORMALS(0), path) == AI_SUCCESS)
            {
                dstMaterial.TexturePath.NormalMap = Convert(path);

                const auto texture = pScene->GetEmbeddedTexture(path.C_Str());
                if (texture != nullptr)
                {
                    if (texture->mHeight != 0)
                    {
                        // TODO: Handle when texture->mHeight = 0
                    }
                    else
                    {
                        dstMaterial.TextureData.NormalTex = (uint8_t*)malloc(texture->mWidth);
                        if (dstMaterial.TextureData.NormalTex != nullptr)
                        {
                            memcpy(dstMaterial.TextureData.NormalTex, texture->pcData, texture->mWidth);
                            dstMaterial.TextureData.NormalTexSize = texture->mWidth;
                        }
                    }
                }
                else
                {
                    // TODO: regular file, check if it exists and read it
                }
            }
            else
            {
                if (pSrcMaterial->Get(AI_MATKEY_TEXTURE_HEIGHT(0), path) == AI_SUCCESS)
                {
                    dstMaterial.TexturePath.NormalMap = Convert(path);
                }
                else
                {
                    dstMaterial.TexturePath.NormalMap.clear();
                }
            }
        }
    }
}

const D3D12_INPUT_ELEMENT_DESC MeshVertex::InputElements[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
};
const D3D12_INPUT_LAYOUT_DESC MeshVertex::InputLayout = { MeshVertex::InputElements, MeshVertex::InputElementCount };
static_assert(sizeof(MeshVertex) == 44, "Vertex struct/layout mismatch");

bool LoadMesh
(
    const wchar_t* filename,
    std::vector<ResMesh>& meshes,
    std::vector<ResMaterial>& materials
)
{
    MeshLoader loader;
    return loader.Load(filename, meshes, materials);
}