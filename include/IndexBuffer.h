#pragma once

#include <d3d12.h>
#include <ComPtr.h>
#include <cstdint>
#include <CommandList.h>
#include <Fence.h>

class IndexBuffer
{
public:
    IndexBuffer();
    ~IndexBuffer();

    bool Init(
        ID3D12Device* pDevice,
        ID3D12CommandQueue* pQueue,
        CommandList* pCmdList,
        Fence* pFence,
        size_t size, const uint32_t* 
        pInitData = nullptr);

    void Term();

    uint32_t* Map();
    void Unmap();

    D3D12_INDEX_BUFFER_VIEW GetView() const;

private:
    ComPtr<ID3D12Resource>      m_pIB;
    D3D12_INDEX_BUFFER_VIEW     m_View;

    IndexBuffer(const IndexBuffer&) = delete;
    void operator = (const IndexBuffer&) = delete;
};