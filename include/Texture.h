#pragma once

#include <d3d12.h>
#include <ComPtr.h>
#include <ResourceUploadBatch.h>

class DescriptorHandle;
class DescriptorPool;

class Texture
{
public:
    Texture();
    ~Texture();

    bool Init(
        ID3D12Device*                   pDevice,
        DescriptorPool*                 pPool,
        const wchar_t*                  filename,
        DirectX::ResourceUploadBatch&   batch);

    bool Init(
        ID3D12Device*                   pDevice,
        DescriptorPool*                 pPool,
        const uint8_t*                  data,
        int                             size,
        DirectX::ResourceUploadBatch&   batch);

    bool Init(
        ID3D12Device*               pDevice,
        DescriptorPool*             pPool,
        const D3D12_RESOURCE_DESC*  pDesc,
        bool                        isCube);

    void Term();

    D3D12_CPU_DESCRIPTOR_HANDLE GetHandleCPU() const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetHandleGPU() const;

private:
    ComPtr<ID3D12Resource>  m_pTex;
    DescriptorHandle*       m_pHandle;
    DescriptorPool*         m_pPool;

    Texture(const Texture&) = delete;
    void operator = (const Texture&) = delete;

    D3D12_SHADER_RESOURCE_VIEW_DESC GetViewDesc(bool isCube);
};