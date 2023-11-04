#pragma once

#include <Windows.h>
#include <cstdint>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <ComPtr.h>
#include <ConstantBuffer.h>
#include <DescriptorPool.h>
#include <DllExport.h>
#include <RenderTarget.h>
#include <DepthTarget.h>
#include <CommandList.h>
#include <VertexBuffer.h>
#include <IndexBuffer.h>
#include <Fence.h>
#include <Material.h>
#include <Mesh.h>
#include <Texture.h>
#include <FrameResource.h>

class IRenderer
{
public:
    virtual bool Load(const wchar_t* filePath) = 0;
    virtual void Resize(uint32_t width, uint32_t height) = 0;
    virtual void Render() = 0;

public:
    // Directional Light
    virtual void SetDirLightDirectionX(float x) = 0;
    virtual void SetDirLightDirectionY(float y) = 0;
    virtual void SetDirLightDirectionZ(float z) = 0;

    virtual void SetDirLightColorR(float r) = 0;
    virtual void SetDirLightColorG(float g) = 0;
    virtual void SetDirLightColorB(float b) = 0;

    // Point Light
    virtual void SetPointLightPositionX(float x) = 0;
    virtual void SetPointLightPositionY(float y) = 0;
    virtual void SetPointLightPositionZ(float z) = 0;

    virtual void SetPointLightColorR(float r) = 0;
    virtual void SetPointLightColorG(float g) = 0;
    virtual void SetPointLightColorB(float b) = 0;

    virtual void SetPointLightRange(float range) = 0;

    // Spot Light
    virtual void SetSpotLightPositionX(float x) = 0;
    virtual void SetSpotLightPositionY(float y) = 0;
    virtual void SetSpotLightPositionZ(float z) = 0;

    virtual void SetSpotLightDirectionX(float x) = 0;
    virtual void SetSpotLightDirectionY(float y) = 0;
    virtual void SetSpotLightDirectionZ(float z) = 0;

    virtual void SetSpotLightColorR(float r) = 0;
    virtual void SetSpotLightColorG(float g) = 0;
    virtual void SetSpotLightColorB(float b) = 0;

    virtual void SetSpotLightRange(float range) = 0;
    virtual void SetSpotLightSpotPower(float angle) = 0;
};

struct RenderItem
{
    VertexBuffer VB;
    IndexBuffer  IB;
};

class Renderer : public IRenderer
{
public:
    Renderer(HINSTANCE hInst, HWND hWnd, uint32_t width, uint32_t height);
    ~Renderer();

public:
    bool Load(const wchar_t* filePath);
    void Resize(uint32_t width, uint32_t height);
    void Render();

public:
    // Directional Light
    void SetDirLightDirectionX(float x);
    void SetDirLightDirectionY(float y);
    void SetDirLightDirectionZ(float z);

    void SetDirLightColorR(float r);
    void SetDirLightColorG(float g);
    void SetDirLightColorB(float b);

    // Point Light
    void SetPointLightPositionX(float x);
    void SetPointLightPositionY(float y);
    void SetPointLightPositionZ(float z);

    void SetPointLightColorR(float r);
    void SetPointLightColorG(float g);
    void SetPointLightColorB(float b);

    void SetPointLightRange(float range);

    // Spot Light
    void SetSpotLightPositionX(float x);
    void SetSpotLightPositionY(float y);
    void SetSpotLightPositionZ(float z);

    void SetSpotLightDirectionX(float x);
    void SetSpotLightDirectionY(float y);
    void SetSpotLightDirectionZ(float z);

    void SetSpotLightColorR(float r);
    void SetSpotLightColorG(float g);
    void SetSpotLightColorB(float b);

    void SetSpotLightRange(float range);
    void SetSpotLightSpotPower(float spotPower);

private:
    static const uint32_t    FrameCount = 2;
    static const int         FrameResourceCount = 3;
    static D3D_FEATURE_LEVEL FeatureLevel;
    static DXGI_FORMAT       BackBufferFormat;
    static UINT              Msaa4xQuality;

    HINSTANCE m_hInst;
    HWND      m_hWnd;
    uint32_t  m_Width;
    uint32_t  m_Height;

    ComPtr<ID3D12Device>       m_pDevice;
    ComPtr<ID3D12CommandQueue> m_pQueue;
    ComPtr<IDXGISwapChain3>    m_pSwapChain;
    RenderTarget               m_ColorTarget[FrameCount];
    DepthTarget                m_DepthTarget;
    DescriptorPool*            m_pPool[DescriptorPool::POOL_COUNT];
    CommandList                m_CommandList;
    ComPtr<ID3D12GraphicsCommandList> m_pCmdList;
    ComPtr<ID3D12CommandAllocator>    m_pDirCmdAllocator;
    Fence                      m_Fence;
    uint32_t                   m_FrameIndex;
    D3D12_VIEWPORT             m_Viewport;
    D3D12_RECT                 m_Scissor;

    std::vector<Mesh*>           m_pMesh;
    ConstantBuffer*              m_Transform;
    std::vector<ConstantBuffer*> m_TransformShadow;
    Material                     m_Material;
    ConstantBuffer*              m_pPass;
    ConstantBuffer*              m_pLight;
    ComPtr<ID3D12PipelineState>  m_pPSO;
    ComPtr<ID3D12RootSignature>  m_pRootSig;
    float                        m_RotateAngle;

    std::vector<FrameResource> m_FrameResources;
    int                        m_CurrFrameResIndex;
    FrameResource*             m_CurrFrameRes;
    std::vector<RenderItem>    m_RenderItems;

private:
    bool InitD3DComponent();
    bool InitD3DAsset();

    void TermD3D();

    void CreateSwapChain();
    
    void BuildRenderItems();

    void Update();
    void UpdateTransform();
    void UpdateLight();
    void UpdateMaterial();
    void UpdatePass();

    void Draw();

    void Present(uint32_t interval);
};

extern "C" DLL_API IRenderer * CreateRenderer(
    HINSTANCE   hInst, 
    HWND        hWnd, 
    uint32_t    width, 
    uint32_t    height);