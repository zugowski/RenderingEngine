#pragma once

#include <DescriptorPool.h>
#include <ResourceUploadBatch.h>
#include <Texture.h>
#include <ConstantBuffer.h>
#include <map>

class Material
{
public:
    enum TEXTURE_USAGE
    {
        TEXTURE_USAGE_DIFFUSE = 0,
        TEXTURE_USAGE_SPECULAR,
        TEXTURE_USAGE_SHININESS,
        TEXTURE_USAGE_NORMAL,
        TEXTURE_USAGE_COUNT
    };

    Material();
    ~Material();

    bool Init(
        ID3D12Device*   pDevice,
        DescriptorPool* pPool,
        size_t          bufferSize,
        size_t          count);

    void Term();

    bool SetTexture(
        size_t                          index,
        TEXTURE_USAGE                   usage,
        const std::wstring&             path,
        DirectX::ResourceUploadBatch&   batch);

    bool SetTexture(
        size_t                          index,
        TEXTURE_USAGE                   usage,
        const std::wstring&             path,
        const uint8_t*                  data,
        int                             size,
        DirectX::ResourceUploadBatch&   batch);

    void* GetBufferPtr(size_t index) const;

    template<typename T>
    T* GetBufferPtr(size_t index) const
    {
        return reinterpret_cast<T*>(GetBufferPtr(index));
    }

    D3D12_GPU_VIRTUAL_ADDRESS GetBufferAddress(size_t index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetTextureHandle(size_t index, TEXTURE_USAGE usage) const;

    size_t GetCount() const;

private:
    struct Subset
    {
        ConstantBuffer*             pCostantBuffer;
        D3D12_GPU_DESCRIPTOR_HANDLE TextureHandle[TEXTURE_USAGE_COUNT];
    };

    std::map<std::wstring, Texture*>    m_pTexture;
    std::vector<Subset>                 m_Subset;
    ID3D12Device*                       m_pDevice;
    DescriptorPool*                     m_pPool;

    Material(const Material&) = delete;
    void operator = (const Material&) = delete;
};

constexpr auto TU_DIFFUSE   = Material::TEXTURE_USAGE_DIFFUSE;
constexpr auto TU_SPECULAR  = Material::TEXTURE_USAGE_SPECULAR;
constexpr auto TU_SHININESS = Material::TEXTURE_USAGE_SHININESS;
constexpr auto TU_NORMAL    = Material::TEXTURE_USAGE_NORMAL;