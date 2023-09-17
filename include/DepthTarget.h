#pragma once

#include <d3d12.h>
#include <ComPtr.h>
#include <cstdint>

class DescriptorHandle;
class DescriptorPool;

class DepthTarget
{
public:
    DepthTarget();
    ~DepthTarget();

    bool Init(
        ID3D12Device*   pDevice,
        DescriptorPool* pPoolDSV,
        uint32_t        width,
        uint32_t        height,
        DXGI_FORMAT     format);

    void Term();

    DescriptorHandle* GetHandleDSV() const;
    ID3D12Resource* GetResource() const;
    D3D12_RESOURCE_DESC GetDesc() const;
    D3D12_DEPTH_STENCIL_VIEW_DESC GetViewDesc() const;

private:
    ComPtr<ID3D12Resource>          m_pTarget;
    DescriptorHandle*               m_pHandleDSV;
    DescriptorPool*                 m_pPoolDSV;
    D3D12_DEPTH_STENCIL_VIEW_DESC   m_ViewDesc;

    DepthTarget(const DepthTarget&) = delete;
    void operator = (const DepthTarget&) = delete;
};