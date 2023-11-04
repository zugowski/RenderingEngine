#pragma once

#include <d3d12.h>
#include <ComPtr.h>

class Fence
{
public:
    Fence();
    ~Fence();

    void Init(ID3D12Device* pDevice);
    void Term();
    void Wait(ID3D12CommandQueue* pQueue, UINT timeout);
    void Sync(ID3D12CommandQueue* pQueue);

    // FrameResource��
    void Wait(UINT64 fenceValue, UINT timeout);
    void Signal();

private:
    ComPtr<ID3D12Fence> m_pFence;
    HANDLE              m_Event;
    UINT                m_Counter;

    Fence(const Fence&) = delete;
    void operator = (const Fence&) = delete;
};