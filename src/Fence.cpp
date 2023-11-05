#include "Fence.h"

Fence::Fence()
    : m_pFence(nullptr)
    , m_Event(nullptr)
    , m_Counter(0)
{
}

Fence::~Fence()
{
    Term();
}

void Fence::Init(ID3D12Device* pDevice)
{
    if (pDevice == nullptr)
        __debugbreak();

    m_Event = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
    if (m_Event == nullptr)
        __debugbreak();

    auto hr = pDevice->CreateFence(
        0,
        D3D12_FENCE_FLAG_NONE,
        IID_PPV_ARGS(m_pFence.GetAddressOf()));
    if (FAILED(hr))
        __debugbreak();

    m_Counter = 1;
}

void Fence::Term()
{
    if (m_Event != nullptr)
    {
        CloseHandle(m_Event);
        m_Event = nullptr;
    }
    m_pFence.Reset();
    m_Counter = 0;
}

void Fence::Wait(ID3D12CommandQueue* pQueue, UINT timeout)
{
    if (pQueue == nullptr)
    {
        return;
    }

    const auto fenceValue = m_Counter;

    auto hr = pQueue->Signal(m_pFence.Get(), fenceValue);
    if (FAILED(hr))
        __debugbreak();

    m_Counter++;

    if (m_pFence->GetCompletedValue() < fenceValue)
    {
        auto hr = m_pFence->SetEventOnCompletion(fenceValue, m_Event);
        if (FAILED(hr))
            __debugbreak();

        if (WAIT_OBJECT_0 != WaitForSingleObjectEx(m_Event, timeout, FALSE))
            return;
    }
}

void Fence::Sync(ID3D12CommandQueue* pQueue)
{
    if (pQueue == nullptr)
        return;

    auto hr = pQueue->Signal(m_pFence.Get(), m_Counter);
    if (FAILED(hr))
        __debugbreak();

    hr = m_pFence->SetEventOnCompletion(m_Counter, m_Event);
    if (FAILED(hr))
        __debugbreak();

    if (WAIT_OBJECT_0 != WaitForSingleObjectEx(m_Event, INFINITE, FALSE))
        return;

    m_Counter++;
}

void Fence::Wait(UINT64 fenceValue, UINT timeout)
{
    if (fenceValue == 0)
        return;

    if (fenceValue < m_pFence->GetCompletedValue())
    {
        auto hr = m_pFence->SetEventOnCompletion(fenceValue, m_Event);
        if (FAILED(hr))
            __debugbreak();

        if (WAIT_OBJECT_0 != WaitForSingleObjectEx(m_Event, timeout, FALSE))
            return;
    }
}

void Fence::Signal(ID3D12CommandQueue* pQueue)
{
    pQueue->Signal(m_pFence.Get(), m_Counter);
}