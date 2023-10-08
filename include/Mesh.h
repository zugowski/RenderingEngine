#pragma once

#include <ResMesh.h>
#include <VertexBuffer.h>
#include <IndexBuffer.h>
#include <CommandList.h>
#include <Fence.h>

class Mesh
{
public:
    static float Scale;

public:
    Mesh();
    virtual ~Mesh();

    bool Init(
        ID3D12Device* pDevice,
        ID3D12CommandQueue* pQueue,
        CommandList* pCmdList,
        Fence* pFence,
        const ResMesh& resource);

    void Term();

    void Draw(ID3D12GraphicsCommandList* pCmdList);

    uint32_t GetMaterialId() const;

private:
    VertexBuffer    m_VB;
    IndexBuffer     m_IB;
    uint32_t        m_MaterialId;
    uint32_t        m_IndexCount;

    Mesh(const Mesh&) = delete;
    void operator = (const Mesh&) = delete;
};