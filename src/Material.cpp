#include "Material.h"
#include "FileUtil.h"
#include "Logger.h"

namespace {
    const wchar_t* DummyTag = L"";
}

Material::Material()
    : m_pDevice(nullptr)
    , m_pPool(nullptr)
{
}

Material::~Material()
{
    Term();
}

bool Material::Init
(
    ID3D12Device*   pDevice,
    DescriptorPool* pPool,
    size_t          bufferSize,
    size_t          count
)
{
    if (pDevice == nullptr || pPool == nullptr || count == 0)
    {
        ELOG("Error : Invalid Argument.");
        return false;
    }

    Term();

    m_pDevice = pDevice;
    m_pDevice->AddRef();

    m_pPool = pPool;
    m_pPool->AddRef();

    m_Subset.resize(count);

    auto pTexture = new (std::nothrow) Texture();
    if (pTexture == nullptr)
    {
        return false;
    }

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width              = 1;
    desc.Height             = 1;
    desc.DepthOrArraySize   = 1;
    desc.MipLevels          = 1;
    desc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;

    if (!pTexture->Init(pDevice, pPool, &desc, false))
    {
        ELOG("Error : Texture::Init() Failed.");
        pTexture->Term();
        delete pTexture;
        return false;
    }

    m_pTexture[DummyTag] = pTexture;

    auto size = bufferSize * count;
    if (size > 0)
    {
        for (size_t i = 0; i < m_Subset.size(); ++i)
        {
            auto pBuffer = new (std::nothrow) ConstantBuffer();
            if (pBuffer == nullptr)
            {
                ELOG("Error : Out of memory.");
                return false;
            }

            if (!pBuffer->Init(pDevice, pPool, bufferSize, 1))
            {
                ELOG("Error : ConstantBuffer::Init() Failed.");
                return false;
            }

            m_Subset[i].pCostantBuffer = pBuffer;
            for (auto j = 0; j < TEXTURE_USAGE_COUNT; ++j)
                m_Subset[i].TextureHandle[j].ptr = 0;
        }
    }
    else
    {
        for (size_t i = 0; i < m_Subset.size(); ++i)
        {
            m_Subset[i].pCostantBuffer = nullptr;
            for (auto j = 0; j < TEXTURE_USAGE_COUNT; ++j)
                m_Subset[i].TextureHandle[j].ptr = 0;
        }
    }

    return true;
}

void Material::Term()
{
    for (auto& itr : m_pTexture)
    {
        if (itr.second != nullptr)
        {
            itr.second->Term();
            delete itr.second;
            itr.second = nullptr;
        }
    }

    for (size_t i = 0; i < m_Subset.size(); ++i)
    {
        if (m_Subset[i].pCostantBuffer != nullptr)
        {
            m_Subset[i].pCostantBuffer->Term();
            delete m_Subset[i].pCostantBuffer;
            m_Subset[i].pCostantBuffer = nullptr;
        }
    }

    m_pTexture.clear();
    m_Subset.clear();

    if (m_pDevice != nullptr)
    {
        m_pDevice->Release();
        m_pDevice = nullptr;
    }

    if (m_pPool != nullptr)
    {
        m_pPool->Release();
        m_pPool = nullptr;
    }
}

bool Material::SetTexture
(
    size_t                          index,
    TEXTURE_USAGE                   usage,
    const std::wstring&             path,
    DirectX::ResourceUploadBatch&   batch
)
{
    if (index >= GetCount())
        return false;

    DWORD fileAttr = GetFileAttributes(path.c_str());

    if (PathFileExistsW(path.c_str()) == FALSE || (fileAttr & FILE_ATTRIBUTE_DIRECTORY))
    {
        m_Subset[index].TextureHandle[usage] = m_pTexture[DummyTag]->GetHandleGPU();
        return true;
    }

    if (m_pTexture.find(path) != m_pTexture.end())
    {
        m_Subset[index].TextureHandle[usage] = m_pTexture[path]->GetHandleGPU();
        return true;
    }

    auto pTexture = new (std::nothrow) Texture();
    if (pTexture == nullptr)
    {
        ELOG("Error : Out of memory.");
        return false;
    }

    bool res = false;
    if (!pTexture->Init(m_pDevice, m_pPool, path.c_str(), batch))
    {
        ELOG("Error : Texture::Init() Failed.");
        pTexture->Term();
        delete pTexture;
        return false;
    }

    m_pTexture[path] = pTexture;
    m_Subset[index].TextureHandle[usage] = pTexture->GetHandleGPU();

    return true;
}

bool Material::SetTexture
(
    size_t                          index,
    TEXTURE_USAGE                   usage,
    const std::wstring&             path,
    const uint8_t*                  data,
    int                             size,
    DirectX::ResourceUploadBatch&   batch
)
{
    if (index >= GetCount())
        return false;

    if (m_pTexture.find(path) != m_pTexture.end())
    {
        m_Subset[index].TextureHandle[usage] = m_pTexture[path]->GetHandleGPU();
        return true;
    }

    auto pTexture = new (std::nothrow) Texture();
    if (pTexture == nullptr)
    {
        ELOG("Error : Out of memory.");
        return false;
    }

    if (!pTexture->Init(m_pDevice, m_pPool, data, size, batch))
    {
        ELOG("Error : Texture::Init() Failed.");
        pTexture->Term();
        delete pTexture;
        return false;
    }

    m_pTexture[path] = pTexture;
    m_Subset[index].TextureHandle[usage] = pTexture->GetHandleGPU();

    return true;
}

void* Material::GetBufferPtr(size_t index) const
{
    if (index >= GetCount())
    {
        return nullptr;
    }

    return m_Subset[index].pCostantBuffer->GetPtr();
}

D3D12_GPU_VIRTUAL_ADDRESS Material::GetBufferAddress(size_t index) const
{
    if (index >= GetCount())
    {
        return D3D12_GPU_VIRTUAL_ADDRESS();
    }

    return m_Subset[index].pCostantBuffer->GetAddress();
}

D3D12_GPU_DESCRIPTOR_HANDLE Material::GetTextureHandle(size_t index, TEXTURE_USAGE usage) const
{
    if (index >= GetCount())
    {
        return D3D12_GPU_DESCRIPTOR_HANDLE();
    }

    return m_Subset[index].TextureHandle[usage];
}

size_t Material::GetCount() const
{
    return m_Subset.size();
}