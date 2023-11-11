#pragma once

#include <d3d12.h>
#include <ComPtr.h>
#include <vector>

class DescriptorHandle;
class DescriptorPool;

class ConstantBuffer
{
public:
    ConstantBuffer();
    ~ConstantBuffer();

    bool Init(
        ID3D12Device*   pDevice,
        DescriptorPool* pPool,
        size_t          size,
        int             count);

    void Term();

    D3D12_GPU_VIRTUAL_ADDRESS GetAddress() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetHandleCPU() const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetHandleGPU() const;

    void* GetPtr() const;

    template<typename T>
    T* GetPtr()
    {
        return reinterpret_cast<T*>(GetPtr());
    }

    template<typename T>
    void CopyData(int dataIdx, const T& data)
    {
        BYTE* ptr = (BYTE*)m_pMappedPtr;
        memcpy(&ptr[dataIdx * m_ElementSize], &data, sizeof(T));
    }

    UINT64 GetElementSize() const { return m_ElementSize; }

private:
    ComPtr<ID3D12Resource>          m_pCB;
    DescriptorHandle*               m_pHandle;
    DescriptorPool*                 m_pPool;
    D3D12_CONSTANT_BUFFER_VIEW_DESC m_Desc;
    void*                           m_pMappedPtr;
    UINT64                          m_ElementSize;

    ConstantBuffer(const ConstantBuffer&) = delete;
    void operator = (const ConstantBuffer&) = delete;
};