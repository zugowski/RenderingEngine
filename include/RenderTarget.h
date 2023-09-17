#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <ComPtr.h>
#include <cstdint>

class DescriptorHandle;
class DescriptorPool;

class RenderTarget
{
public:
    RenderTarget();
    ~RenderTarget();

    bool Init(
        ID3D12Device*   pDevice,
        DescriptorPool* pPoolRTV,
        uint32_t        width,
        uint32_t        height,
        DXGI_FORMAT format);

    bool InitFromBackBuffer(
        ID3D12Device*   pDevice,
        DescriptorPool* pPoolRTV,
        uint32_t        index,
        IDXGISwapChain* pSwapChain);

    void Term();

    DescriptorHandle* GetHandleRTV() const;
    ID3D12Resource* GetResource() const;
    D3D12_RESOURCE_DESC GetDesc() const;
    D3D12_RENDER_TARGET_VIEW_DESC GetViewDesc() const;

private:
    ComPtr<ID3D12Resource>          m_pTarget;
    DescriptorHandle*               m_pHandleRTV;
    DescriptorPool*                 m_pPoolRTV;
    D3D12_RENDER_TARGET_VIEW_DESC   m_ViewDesc;

    RenderTarget(const RenderTarget&) = delete;
    void operator = (const RenderTarget&) = delete;
};