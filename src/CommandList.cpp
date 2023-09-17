#include "CommandList.h"

CommandList::CommandList()
    : m_pCmdList(nullptr)
    , m_pAllocators()
    , m_Index(0)
{
}

CommandList::~CommandList()
{
    Term();
}

bool CommandList::Init(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type, uint32_t count)
{
    if (pDevice == nullptr || count == 0)
    {
        return false;
    }

    m_pAllocators.resize(count);

    for (auto i = 0u; i < count; ++i)
    {
        auto hr = pDevice->CreateCommandAllocator(
            type, IID_PPV_ARGS(m_pAllocators[i].GetAddressOf()));
        if (FAILED(hr))
        {
            return false;
        }
    }

    {
        auto hr = pDevice->CreateCommandList(
            1,
            type,
            m_pAllocators[0].Get(),
            nullptr,
            IID_PPV_ARGS(m_pCmdList.GetAddressOf()));
        if (FAILED(hr))
        {
            return false;
        }

        m_pCmdList->Close();
    }

    m_Index = 0;
    return true;
}

void CommandList::Term()
{
    m_pCmdList.Reset();

    for (size_t i = 0; i < m_pAllocators.size(); ++i)
    {
        m_pAllocators[i].Reset();
    }

    m_pAllocators.clear();
    m_pAllocators.shrink_to_fit();
}

ID3D12GraphicsCommandList* CommandList::Reset()
{
    auto hr = m_pAllocators[m_Index]->Reset();
    if (FAILED(hr))
    {
        return nullptr;
    }

    hr = m_pCmdList->Reset(m_pAllocators[m_Index].Get(), nullptr);
    if (FAILED(hr))
    {
        return nullptr;
    }

    m_Index = (m_Index + 1) % uint32_t(m_pAllocators.size());
    return m_pCmdList.Get();
}