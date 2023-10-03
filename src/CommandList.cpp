#include "CommandList.h"

CommandList::CommandList()
    : m_pCmdList(nullptr)
    , m_pAllocator(nullptr)
{
}

CommandList::~CommandList()
{
    Term();
}

void CommandList::Init(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type, uint32_t count)
{
    if (pDevice == nullptr || count == 0)
        __debugbreak();

    for (auto i = 0u; i < count; ++i)
    {
        auto hr = pDevice->CreateCommandAllocator(
            type, IID_PPV_ARGS(m_pAllocator.GetAddressOf()));
        if (FAILED(hr))
            __debugbreak();
    }

    auto hr = pDevice->CreateCommandList(
        1,
        type,
        m_pAllocator.Get(),
        nullptr,
        IID_PPV_ARGS(m_pCmdList.GetAddressOf()));
    if (FAILED(hr))
        __debugbreak();

    m_pCmdList->Close();
}

void CommandList::Term()
{
    m_pCmdList.Reset();
    m_pAllocator.Reset();
}

ID3D12GraphicsCommandList* CommandList::Reset()
{
    auto hr = m_pAllocator->Reset();
    if (FAILED(hr))
    {
        return nullptr;
    }

    hr = m_pCmdList->Reset(m_pAllocator.Get(), nullptr);
    if (FAILED(hr))
    {
        return nullptr;
    }

    return m_pCmdList.Get();
}