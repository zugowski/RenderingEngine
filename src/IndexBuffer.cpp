#include "IndexBuffer.h"

IndexBuffer::IndexBuffer()
    : m_pIB(nullptr)
{
    memset(&m_View, 0, sizeof(m_View));
}

IndexBuffer::~IndexBuffer()
{
    Term();
}

bool IndexBuffer::Init
(
    ID3D12Device* pDevice,
    ID3D12CommandQueue* pQueue,
    CommandList* pCmdList,
    Fence* pFence,
    size_t size,
    const uint32_t* pInitData
)
{
    // UPLOAD 힙 생성 및 데이터 복사
    ComPtr<ID3D12Resource> uploadBuffer;

    D3D12_HEAP_PROPERTIES prop = {};
    prop.Type                 = D3D12_HEAP_TYPE_UPLOAD;
    prop.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    prop.CreationNodeMask     = 1;
    prop.VisibleNodeMask      = 1;

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment          = 0;
    desc.Width              = UINT64(size);
    desc.Height             = 1;
    desc.DepthOrArraySize   = 1;
    desc.MipLevels          = 1;
    desc.Format             = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    auto hr = pDevice->CreateCommittedResource(
        &prop,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(uploadBuffer.GetAddressOf()));
    if (FAILED(hr))
        return false;

    if (pInitData != nullptr)
    {
        void* ptr;

        if (FAILED(uploadBuffer->Map(0, nullptr, &ptr)))
            return false;

        if (ptr == nullptr)
            return false;

        memcpy(ptr, pInitData, size);

        uploadBuffer->Unmap(0, nullptr);
    }

    // DEFAULT 힙 생성
    prop.Type = D3D12_HEAP_TYPE_DEFAULT;
    hr = pDevice->CreateCommittedResource(
        &prop,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(m_pIB.GetAddressOf()));
    if (FAILED(hr))
        return false;

    m_View.BufferLocation = m_pIB->GetGPUVirtualAddress();
    m_View.Format         = DXGI_FORMAT_R32_UINT;
    m_View.SizeInBytes    = UINT(size);

    // UPLOAD 힙으로 부터 데이터 복사
    auto pCmd = pCmdList->Reset();

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource   = m_pIB.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    pCmd->ResourceBarrier(1, &barrier);

    pCmd->CopyResource(m_pIB.Get(), uploadBuffer.Get());

    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource   = m_pIB.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COMMON;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    pCmd->ResourceBarrier(1, &barrier);

    pCmd->Close();

    ID3D12CommandList* pLists[] = { pCmd };
    pQueue->ExecuteCommandLists(1, pLists);
    pFence->Sync(pQueue);

    return true;
}

void IndexBuffer::Term()
{
    m_pIB.Reset();
    memset(&m_View, 0, sizeof(m_View));
}

uint32_t* IndexBuffer::Map()
{
    uint32_t* ptr;
    auto hr = m_pIB->Map(0, nullptr, reinterpret_cast<void**>(&ptr));
    if (FAILED(hr))
    {
        return nullptr;
    }

    return ptr;
}

void IndexBuffer::Unmap()
{
    m_pIB->Unmap(0, nullptr);
}

D3D12_INDEX_BUFFER_VIEW IndexBuffer::GetView() const
{
    return m_View;
}