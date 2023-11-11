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
#include <FrameResource.h>
#include <Fence.h>
#include <Material.h>
#include <Mesh.h>
#include <Texture.h>

constexpr auto DirLightInitDir        = DirectX::XMFLOAT3(0.0f, 30.0f, -15.0f);
constexpr auto PointLightInitPos      = DirectX::XMFLOAT3(0.0f, 30.0f, -15.0f);
constexpr auto PointLightInitRange    = 150.0f;
constexpr auto SpotLightInitPos       = DirectX::XMFLOAT3(0.0f, 20.0f, -50.0f);
constexpr auto SpotLightInitDir       = DirectX::XMFLOAT3(0.0f, 30.0f, -15.0f);
constexpr auto SpotLightInitRange     = 150.0f;
constexpr auto SpotLightInitSpotPower = 5.0f;

class IRenderer
{
public:
    virtual bool Load(const wchar_t* filePath) = 0;
    virtual void Resize(uint32_t width, uint32_t height) = 0;
    virtual void Render() = 0;

public:
    static const uint32_t    FrameCount = 2;
    static const int         FrameResourceCount = 3;
    static D3D_FEATURE_LEVEL FeatureLevel;
    static DXGI_FORMAT       BackBufferFormat;
    static UINT              Msaa4xQuality;
    static DirectX::XMFLOAT3 EyePos;

    TransformBuffer Transform;
    LightBuffer     Light;
    PassConstant    Pass;
};

struct RenderItem
{
    bool IsShadow;
    int MeshIdx;
    int DataIdx;
    TransformBuffer Transform;
    LightBuffer     Light;
    MaterialBuffer  Material;
    PassConstant    Pass;
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

private:
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
    Fence                      m_Fence;
    uint32_t                   m_FrameIndex;
    D3D12_VIEWPORT             m_Viewport;
    D3D12_RECT                 m_Scissor;

    std::vector<Mesh*>           m_pMesh;
    Material                     m_Material;
    ComPtr<ID3D12PipelineState>  m_pPSO;
    ComPtr<ID3D12RootSignature>  m_pRootSig;
    float                        m_RotateAngle;

    std::vector<FrameResource*>  m_FrameResources;
    FrameResource*               m_CurrFrameRes;
    int                          m_CurrFrameResIndex;

    ComPtr<ID3D12GraphicsCommandList> m_pCmdList;
    ComPtr<ID3D12CommandAllocator>    m_pDirCmdAllocator;

    std::vector<RenderItem> m_RenderItems;

private:
    bool InitD3DComponent();
    bool InitD3DAsset();

    void TermD3D();

    void CreateSwapChain();

    void BuildRenderItems();
    void BuildFrameResources();

    void Update();
    void UpdateTransform();
    void UpdateLight();
    void UpdateMaterial();
    void UpdatePass();

    void Draw();
    void DrawRenderItems();

    void Present(uint32_t interval);
};

extern "C" DLL_API IRenderer * CreateRenderer(
    HINSTANCE   hInst, 
    HWND        hWnd, 
    uint32_t    width, 
    uint32_t    height);