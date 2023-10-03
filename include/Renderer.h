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
#include <Fence.h>
#include <Material.h>
#include <Mesh.h>
#include <Texture.h>

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
    virtual void SetPtLightPositionX(float x) = 0;
    virtual void SetPtLightPositionY(float y) = 0;
    virtual void SetPtLightPositionZ(float z) = 0;

    virtual void SetPtLightColorR(float r) = 0;
    virtual void SetPtLightColorG(float g) = 0;
    virtual void SetPtLightColorB(float b) = 0;

    virtual void SetPtLightRange(int range) = 0;

    // Spot Light
    virtual void SetSptLightPositionX(float x) = 0;
    virtual void SetSptLightPositionY(float y) = 0;
    virtual void SetSptLightPositionZ(float z) = 0;

    virtual void SetSptLightDirectionX(float x) = 0;
    virtual void SetSptLightDirectionY(float y) = 0;
    virtual void SetSptLightDirectionZ(float z) = 0;

    virtual void SetSptLightColorR(float r) = 0;
    virtual void SetSptLightColorG(float g) = 0;
    virtual void SetSptLightColorB(float b) = 0;

    virtual void SetSptLightRange(int range) = 0;
    virtual void SetSptLightAngle(int angle) = 0;
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
    void SetPtLightPositionX(float x);
    void SetPtLightPositionY(float y);
    void SetPtLightPositionZ(float z);

    void SetPtLightColorR(float r);
    void SetPtLightColorG(float g);
    void SetPtLightColorB(float b);

    void SetPtLightRange(int range);

    // Spot Light
    void SetSptLightPositionX(float x);
    void SetSptLightPositionY(float y);
    void SetSptLightPositionZ(float z);

    void SetSptLightDirectionX(float x);
    void SetSptLightDirectionY(float y);
    void SetSptLightDirectionZ(float z);

    void SetSptLightColorR(float r);
    void SetSptLightColorG(float g);
    void SetSptLightColorB(float b);

    void SetSptLightRange(int range);
    void SetSptLightAngle(int angle);

private:
    static const uint32_t    FrameCount = 2;
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
    Fence                      m_Fence;
    uint32_t                   m_FrameIndex;
    D3D12_VIEWPORT             m_Viewport;
    D3D12_RECT                 m_Scissor;

    std::vector<Mesh*>           m_pMesh;
    ConstantBuffer*              m_Transform;
    std::vector<ConstantBuffer*> m_TransformShadow;
    Material                     m_Material;
    ConstantBuffer*              m_pShadingConfig;
    ConstantBuffer*              m_pLight;
    ComPtr<ID3D12PipelineState>  m_pPSO;
    ComPtr<ID3D12RootSignature>  m_pRootSig;
    float                        m_RotateAngle;

private:
    bool InitD3DComponent();
    bool InitD3DAsset();

    void TermD3D();

    void CreateSwapChain();

    void Present(uint32_t interval);
};

extern "C" DLL_API IRenderer * CreateRenderer(
    HINSTANCE   hInst, 
    HWND        hWnd, 
    uint32_t    width, 
    uint32_t    height);