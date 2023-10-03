#pragma once

#include <d3d12.h>
#include <ComPtr.h>
#include <cstdint>
#include <vector>

class CommandList
{
public:
    CommandList();
    ~CommandList();

    void Init(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type, uint32_t count);
    void Term();

    ID3D12GraphicsCommandList* Reset();

private:
    ComPtr<ID3D12GraphicsCommandList> m_pCmdList;
    ComPtr<ID3D12CommandAllocator>    m_pAllocator;

    CommandList(const CommandList&) = delete;
    void operator = (const CommandList&) = delete;
};