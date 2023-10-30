#include <Windows.Foundation.h>
#include <wrl/wrappers/corewrappers.h>
#include <wrl/client.h>
#include <CommonStates.h>
#include "Renderer.h"
#include "ShaderUtil.h"
#include "WinPixUtil.h"
#include "FileUtil.h"
#include "Logger.h"

struct Transform
{
    DirectX::XMMATRIX World;
    DirectX::XMMATRIX View;
    DirectX::XMMATRIX Proj;
};

struct Light
{
    DirectX::XMFLOAT3 Color;     // 빛의 세기
    float Range;                 // 포인트/스포트라이트 전용
    DirectX::XMFLOAT3 Direction; // 디렉셔널/스포트라이트 전용
    float SpotPower;             // 스포트라이트 전용
    DirectX::XMFLOAT3 Position;  // 포인트라이트 전용
    float pad;
};

struct LightBuffer
{
    Light DirLight;
    Light PointLight;
    Light SpotLight;
};

struct MaterialBuffer
{
    DirectX::XMFLOAT3 Diffuse;
    float             Alpha;
    DirectX::XMFLOAT3 Specular;
    float             Shininess;
};

struct PassConstantBuffer
{
    alignas(16) DirectX::XMFLOAT3 CameraPosition;
    alignas(16) DirectX::XMFLOAT4 AmbientLight;
    int DiffuseMapUsable;
    int SpecularMapUsable;
    int ShininessMapUsable;
    int NormalMapUsable;
};

Renderer::Renderer(HINSTANCE hInst, HWND hWnd, uint32_t width, uint32_t height)
    : m_hInst(hInst)
    , m_hWnd(hWnd)
    , m_Width(width)
    , m_Height(height)
    , m_pDevice(nullptr)
    , m_FrameIndex(0)
{
    // 필수적인 요소 초기화
    InitD3DComponent();

    // 추가적인 요소 초기화
    InitD3DAsset();
}

D3D_FEATURE_LEVEL Renderer::FeatureLevel = D3D_FEATURE_LEVEL_12_0;
DXGI_FORMAT Renderer::BackBufferFormat   = DXGI_FORMAT_R8G8B8A8_UNORM;
UINT Renderer::Msaa4xQuality             = 0;

Renderer::~Renderer()
{
}

bool Renderer::Load(const wchar_t* filePath)
{
    std::wstring path = filePath;
    std::wstring dir = GetDirectoryPath(path.c_str());

    std::vector<ResMesh>        resMesh;
    std::vector<ResMaterial>    resMaterial;

    if (!LoadMesh(path.c_str(), resMesh, resMaterial))
    {
        ELOG("Error : Load Mesh Failed. filepath = %ls", path.c_str());
        return false;
    }

    m_pMesh.clear();
    m_pMesh.reserve(resMesh.size());

    for (size_t i = 0; i < resMesh.size(); ++i)
    {
        auto mesh = new (std::nothrow) Mesh();

        if (mesh == nullptr)
        {
            ELOG("Error : Out of memory.");
            return false;
        }

        if (!mesh->Init(m_pDevice.Get(), m_pQueue.Get(), &m_CommandList, &m_Fence, resMesh[i]))
        {
            ELOG("Error : Mesh Initialize Failed.");
            delete mesh;
            return false;
        }

        m_pMesh.push_back(mesh);
    }

    m_pMesh.shrink_to_fit();

    if (!m_Material.Init(
        m_pDevice.Get(),
        m_pPool[DescriptorPool::POOL_TYPE_RES],
        sizeof(MaterialBuffer),
        resMaterial.size()))
    {
        ELOG("Error : Material::Init() Failed.");
        return false;
    }

    DirectX::ResourceUploadBatch batch(m_pDevice.Get());
    batch.Begin();

    // Set shading config for shaders
    auto pPassCB = m_pPass->GetPtr<PassConstantBuffer>();

    for (size_t i = 0; i < resMaterial.size(); ++i)
    {
        auto ptr = m_Material.GetBufferPtr<MaterialBuffer>(i);
        ptr->Diffuse   = resMaterial[i].Diffuse;
        ptr->Alpha     = resMaterial[i].Alpha;
        ptr->Specular  = resMaterial[i].Specular;
        ptr->Shininess = resMaterial[i].Shininess;

        if (resMaterial[i].TextureData.DiffuseTex != nullptr)
        {
            m_Material.SetTexture(
                i,
                TU_DIFFUSE,
                dir + resMaterial[i].TexturePath.DiffuseMap,
                resMaterial[i].TextureData.DiffuseTex,
                resMaterial[i].TextureData.DiffuseTexSize,
                batch);

            pPassCB->DiffuseMapUsable = 1;
        }
        else
        {
            m_Material.SetTexture(
                i,
                TU_DIFFUSE,
                dir + resMaterial[i].TexturePath.DiffuseMap,
                batch);

            //pShdConfig->DiffuseMapUsable = 1;
        }

        if (resMaterial[i].TextureData.NormalTex != nullptr)
        {
            m_Material.SetTexture(
                i,
                TU_NORMAL,
                dir + resMaterial[i].TexturePath.NormalMap,
                resMaterial[i].TextureData.NormalTex,
                resMaterial[i].TextureData.NormalTexSize,
                batch);

            pPassCB->NormalMapUsable = 1;
        }
        else
        {
            m_Material.SetTexture(
                i,
                TU_NORMAL,
                dir + resMaterial[i].TexturePath.NormalMap,
                batch);

            //pShdConfig->NormalMapUsable = 1;
        }

        if (resMaterial[i].TextureData.SpecularTex != nullptr)
        {
            m_Material.SetTexture(
                i,
                TU_SPECULAR,
                dir + resMaterial[i].TexturePath.SpecularMap,
                resMaterial[i].TextureData.SpecularTex,
                resMaterial[i].TextureData.SpecularTexSize,
                batch);

            pPassCB->SpecularMapUsable = 1;
        }
        else
        {
            m_Material.SetTexture(
                i,
                TU_SPECULAR,
                dir + resMaterial[i].TexturePath.SpecularMap,
                batch);

            //pShdConfig->SpecularMapUsable = 1;
        }

        if(resMaterial[i].TextureData.DiffuseTex != nullptr)
            free(resMaterial[i].TextureData.DiffuseTex);

        if (resMaterial[i].TextureData.NormalTex != nullptr)
            free(resMaterial[i].TextureData.NormalTex);

        if (resMaterial[i].TextureData.SpecularTex != nullptr)
            free(resMaterial[i].TextureData.SpecularTex);
    }

    auto future = batch.End(m_pQueue.Get());
    future.wait();

    return true;
}

void Renderer::Resize(uint32_t width, uint32_t height)
{
    if (m_pDevice == nullptr || m_pSwapChain == nullptr)
        __debugbreak();

    m_Width  = width;
    m_Height = height;

    m_Fence.Sync(m_pQueue.Get());
    //auto pCmd = m_CommandList.Reset();

    for(int i = 0; i < FrameCount; ++i)
        m_ColorTarget[i].Term(); 
    m_DepthTarget.Term();

    // swap chain 조정
    auto hr = m_pSwapChain->ResizeBuffers(
        FrameCount,
        m_Width,
        m_Height,
        BackBufferFormat,
        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
    if (FAILED(hr))
        __debugbreak();

    m_FrameIndex = 0;

    // RTV, depth/stencil buffer 생성
    for (int i = 0; i < FrameCount; ++i)
    {
        m_ColorTarget[i].InitFromBackBuffer(
            m_pDevice.Get(),
            m_pPool[DescriptorPool::POOL_TYPE_RTV],
            i,
            m_pSwapChain.Get());
    }

    m_DepthTarget.Init(
        m_pDevice.Get(),
        m_pPool[DescriptorPool::POOL_TYPE_DSV],
        m_Width,
        m_Height,
        DXGI_FORMAT_D32_FLOAT);

    //pCmd->Close();

    //ID3D12CommandList* pLists[] = { pCmd };
    //m_pQueue->ExecuteCommandLists(1, pLists);

    //m_Fence.Sync(m_pQueue.Get());

    // viewport, scissor 크기 조정
    m_Viewport.TopLeftX = 0.0f;
    m_Viewport.TopLeftY = 0.0f;
    m_Viewport.Width    = float(m_Width);
    m_Viewport.Height   = float(m_Height);
    m_Viewport.MinDepth = 0.0f;
    m_Viewport.MaxDepth = 1.0f;

    m_Scissor.left   = 0;
    m_Scissor.right  = m_Width;
    m_Scissor.top    = 0;
    m_Scissor.bottom = m_Height;
}

void Renderer::Render()
{
    auto pTransform = m_Transform->GetPtr<Transform>();

    {
        m_RotateAngle += 0.010f;
        DirectX::XMMATRIX S = DirectX::XMMatrixScaling(Mesh::Scale, Mesh::Scale, Mesh::Scale);
        DirectX::XMMATRIX R = DirectX::XMMatrixRotationY(m_RotateAngle);
        pTransform->World = S * R;
    }

    auto pCmd = m_CommandList.Reset();

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource   = m_ColorTarget[m_FrameIndex].GetResource();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    pCmd->ResourceBarrier(1, &barrier);

    auto handleRTV = m_ColorTarget[m_FrameIndex].GetHandleRTV();
    auto handleDSV = m_DepthTarget.GetHandleDSV();

    pCmd->OMSetRenderTargets(1, &handleRTV->HandleCPU, FALSE, &handleDSV->HandleCPU);

    float clearColor[] = { 0.0f, 0.439f, 0.439f, 1.0f };    // 0.0, 0.52, 0.52
    pCmd->ClearRenderTargetView(handleRTV->HandleCPU, clearColor, 0, nullptr);
    pCmd->ClearDepthStencilView(handleDSV->HandleCPU, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    {
        ID3D12DescriptorHeap* const pHeaps[] = {
            m_pPool[DescriptorPool::POOL_TYPE_RES]->GetHeap()
        };

        pCmd->SetGraphicsRootSignature(m_pRootSig.Get());
        pCmd->SetDescriptorHeaps(1, pHeaps);
        //pCmd->SetGraphicsRootConstantBufferView(0, m_Transform[m_FrameIndex]->GetAddress());
        pCmd->SetGraphicsRootConstantBufferView(1, m_pLight->GetAddress());
        pCmd->SetPipelineState(m_pPSO.Get());
        pCmd->RSSetViewports(1, &m_Viewport);
        pCmd->RSSetScissorRects(1, &m_Scissor);

        for (size_t i = 0; i < m_pMesh.size(); ++i)
        {
            auto id = m_pMesh[i]->GetMaterialId();
            pCmd->SetGraphicsRootConstantBufferView(0, m_Transform->GetAddress());
            pCmd->SetGraphicsRootConstantBufferView(2, m_Material.GetBufferAddress(i));
            pCmd->SetGraphicsRootConstantBufferView(3, m_pPass->GetAddress());
            pCmd->SetGraphicsRootDescriptorTable(4, m_Material.GetTextureHandle(id, TU_DIFFUSE));
            pCmd->SetGraphicsRootDescriptorTable(5, m_Material.GetTextureHandle(id, TU_NORMAL));
            pCmd->SetGraphicsRootDescriptorTable(6, m_Material.GetTextureHandle(id, TU_SPECULAR));
            m_pMesh[i]->Draw(pCmd);

            // 그림자 렌더링
            pCmd->SetGraphicsRootConstantBufferView(0, m_TransformShadow[m_FrameIndex]->GetAddress());

            auto pLight = m_Transform->GetPtr<LightBuffer>();
            DirectX::XMVECTOR shadowPlane = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
            DirectX::XMVECTOR dirLightDir = DirectX::XMLoadFloat3(&pLight->DirLight.Direction);
            DirectX::XMMATRIX S = DirectX::XMMatrixShadow(shadowPlane, dirLightDir);

            auto pTransformShadow = m_TransformShadow[m_FrameIndex]->GetPtr<Transform>();
            pTransformShadow->World = pTransform->World * S;
            m_pMesh[i]->Draw(pCmd);
        }
    }

    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource   = m_ColorTarget[m_FrameIndex].GetResource();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    pCmd->ResourceBarrier(1, &barrier);

    pCmd->Close();

    ID3D12CommandList* pLists[] = { pCmd };
    m_pQueue->ExecuteCommandLists(1, pLists);

    Present(1);
}

void Renderer::SetDirLightDirectionX(float x)
{
    auto ptr = m_pLight->GetPtr<LightBuffer>();
    ptr->DirLight.Direction.x = x;
}

void Renderer::SetDirLightDirectionY(float y)
{
    auto ptr = m_pLight->GetPtr<LightBuffer>();
    ptr->DirLight.Direction.y = y;
}

void Renderer::SetDirLightDirectionZ(float z)
{
    auto ptr = m_pLight->GetPtr<LightBuffer>();
    ptr->DirLight.Direction.z = z;
}

void Renderer::SetDirLightColorR(float r)
{
    auto ptr = m_pLight->GetPtr<LightBuffer>();
    ptr->DirLight.Color.x = r;
}

void Renderer::SetDirLightColorG(float g)
{
    auto ptr = m_pLight->GetPtr<LightBuffer>();
    ptr->DirLight.Color.y = g;
}

void Renderer::SetDirLightColorB(float b)
{
    auto ptr = m_pLight->GetPtr<LightBuffer>();
    ptr->DirLight.Color.z = b;
}

void Renderer::SetPointLightPositionX(float x)
{
    auto ptr = m_pLight->GetPtr<LightBuffer>();
    ptr->PointLight.Position.x = x;
}

void Renderer::SetPointLightPositionY(float y)
{
    auto ptr = m_pLight->GetPtr<LightBuffer>();
    ptr->PointLight.Position.y = y;
}

void Renderer::SetPointLightPositionZ(float z)
{
    auto ptr = m_pLight->GetPtr<LightBuffer>();
    ptr->PointLight.Position.z = z;
}

void Renderer::SetPointLightColorR(float r)
{
    auto ptr = m_pLight->GetPtr<LightBuffer>();
    ptr->PointLight.Color.x = r;
}

void Renderer::SetPointLightColorG(float g)
{
    auto ptr = m_pLight->GetPtr<LightBuffer>();
    ptr->PointLight.Color.y = g;
}

void Renderer::SetPointLightColorB(float b)
{
    auto ptr = m_pLight->GetPtr<LightBuffer>();
    ptr->PointLight.Color.z = b;
}

void Renderer::SetPointLightRange(float range)
{
    auto ptr = m_pLight->GetPtr<LightBuffer>();
    ptr->PointLight.Range = range;
}

void Renderer::SetSpotLightPositionX(float x)
{
    auto ptr = m_pLight->GetPtr<LightBuffer>();
    ptr->SpotLight.Position.x = x;
}

void Renderer::SetSpotLightPositionY(float y)
{
    auto ptr = m_pLight->GetPtr<LightBuffer>();
    ptr->SpotLight.Position.y = y;
}

void Renderer::SetSpotLightPositionZ(float z)
{
    auto ptr = m_pLight->GetPtr<LightBuffer>();
    ptr->SpotLight.Position.z = z;
}

void Renderer::SetSpotLightDirectionX(float x)
{
    auto ptr = m_pLight->GetPtr<LightBuffer>();
    ptr->SpotLight.Direction.x = x;
}

void Renderer::SetSpotLightDirectionY(float y)
{
    auto ptr = m_pLight->GetPtr<LightBuffer>();
    ptr->SpotLight.Direction.y = y;
}

void Renderer::SetSpotLightDirectionZ(float z)
{
    auto ptr = m_pLight->GetPtr<LightBuffer>();
    ptr->SpotLight.Direction.z = z;
}

void Renderer::SetSpotLightColorR(float r)
{
    auto ptr = m_pLight->GetPtr<LightBuffer>();
    ptr->SpotLight.Color.x = r;
}

void Renderer::SetSpotLightColorG(float g)
{
    auto ptr = m_pLight->GetPtr<LightBuffer>();
    ptr->SpotLight.Color.y = g;
}

void Renderer::SetSpotLightColorB(float b)
{
    auto ptr = m_pLight->GetPtr<LightBuffer>();
    ptr->SpotLight.Color.z = b;
}

void Renderer::SetSpotLightRange(float range)
{
    auto ptr = m_pLight->GetPtr<LightBuffer>();
    ptr->SpotLight.Range = range;
}

void Renderer::SetSpotLightSpotPower(float spotPower)
{
    auto ptr = m_pLight->GetPtr<LightBuffer>();
    ptr->SpotLight.SpotPower = spotPower;
}

bool Renderer::InitD3DComponent()
{
    // WICTextureLoader 사용 용도
#if (_WIN32_WINNT >= 0x0A00 /*_WIN32_WINNT_WIN10*/)
    Microsoft::WRL::Wrappers::RoInitializeWrapper initialize(RO_INIT_MULTITHREADED);
    if (FAILED(initialize))
        __debugbreak();
#else
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
        __debugbreak(); 
#endif

    HRESULT hr;
    
#if defined(DEBUG) || defined(_DEBUG)
    // debug layer 활성화
    ComPtr<ID3D12Debug> pDebug;
    hr = D3D12GetDebugInterface(IID_PPV_ARGS(pDebug.GetAddressOf()));
    if (SUCCEEDED(hr))
    {
        pDebug->EnableDebugLayer();
    }

    // PIX attach 용도 설정
    if (GetModuleHandle(L"WinPixGpuCapturer.dll") == 0)
        LoadLibrary(GetLatestWinPixGpuCapturerPath().c_str());
#endif

    // device 생성
    const D3D_FEATURE_LEVEL FeatureLevels[] =
    {
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_2
    };

    for (int i = _countof(FeatureLevels) - 1; i >= 0; --i)
    {
        hr = D3D12CreateDevice(
            nullptr,
            FeatureLevels[i],
            IID_PPV_ARGS(m_pDevice.GetAddressOf()));

        if (SUCCEEDED(hr))
        {
            FeatureLevel = FeatureLevels[i];
            break;
        }
    }

    if (m_pDevice == nullptr)
        __debugbreak();

    // fence 생성
    m_Fence.Init(m_pDevice.Get());

    // command queue 생성
    D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
    cmdQueueDesc.Type     = D3D12_COMMAND_LIST_TYPE_DIRECT;
    cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    cmdQueueDesc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    cmdQueueDesc.NodeMask = 0;

    hr = m_pDevice->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(m_pQueue.GetAddressOf()));
    if (FAILED(hr))
        __debugbreak();

    // command list, command allocator 생성
    m_CommandList.Init(
        m_pDevice.Get(), 
        D3D12_COMMAND_LIST_TYPE_DIRECT, 
        FrameCount);

    // 4X MSAA 품질 수준 지원 점검
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
    msQualityLevels.Format           = BackBufferFormat;
    msQualityLevels.SampleCount      = 4;
    msQualityLevels.Flags            = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    msQualityLevels.NumQualityLevels = 0;

    hr = m_pDevice->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        &msQualityLevels,
        sizeof(msQualityLevels));
    if (FAILED(hr))
        __debugbreak();

    Msaa4xQuality = msQualityLevels.NumQualityLevels;
    assert(Msaa4xQuality > 0 && "Unexpected MSAA quality level.");

    // 스왑체인 생성
    CreateSwapChain();

    // 디스크립터 힙 생성
    D3D12_DESCRIPTOR_HEAP_DESC dhDesc = {};

    dhDesc.NodeMask       = 1;
    dhDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    dhDesc.NumDescriptors = 512;
    dhDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    DescriptorPool::Create(m_pDevice.Get(), &dhDesc, &m_pPool[DescriptorPool::POOL_TYPE_RES]);

    dhDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    dhDesc.NumDescriptors = 256;
    dhDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    DescriptorPool::Create(m_pDevice.Get(), &dhDesc, &m_pPool[DescriptorPool::POOL_TYPE_SMP]);

    dhDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    dhDesc.NumDescriptors = 512;
    dhDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    DescriptorPool::Create(m_pDevice.Get(), &dhDesc, &m_pPool[DescriptorPool::POOL_TYPE_RTV]);

    dhDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dhDesc.NumDescriptors = 512;
    dhDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    DescriptorPool::Create(m_pDevice.Get(), &dhDesc, &m_pPool[DescriptorPool::POOL_TYPE_DSV]);

    // RTV 생성
    for (int i = 0; i < FrameCount; ++i)
    {
        m_ColorTarget[i].InitFromBackBuffer(
            m_pDevice.Get(),
            m_pPool[DescriptorPool::POOL_TYPE_RTV],
            i,
            m_pSwapChain.Get());
    }

    m_DepthTarget.Init(
        m_pDevice.Get(),
        m_pPool[DescriptorPool::POOL_TYPE_DSV],
        m_Width,
        m_Height,
        DXGI_FORMAT_D32_FLOAT);

    m_Viewport.TopLeftX = 0.0f;
    m_Viewport.TopLeftY = 0.0f;
    m_Viewport.Width    = float(m_Width);
    m_Viewport.Height   = float(m_Height);
    m_Viewport.MinDepth = 0.0f;
    m_Viewport.MaxDepth = 1.0f;

    m_Scissor.left   = 0;
    m_Scissor.right  = m_Width;
    m_Scissor.top    = 0;
    m_Scissor.bottom = m_Height;

    return true;
}

bool Renderer::InitD3DAsset()
{
    // root signature 생성
    {
        auto flag = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
        flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
        flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        D3D12_DESCRIPTOR_RANGE range[3] = {};
        range[0].RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        range[0].NumDescriptors                    = 1;
        range[0].BaseShaderRegister                = 0;
        range[0].RegisterSpace                     = 0;
        range[0].OffsetInDescriptorsFromTableStart = 0;

        range[1].RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        range[1].NumDescriptors                    = 1;
        range[1].BaseShaderRegister                = 1;
        range[1].RegisterSpace                     = 0;
        range[1].OffsetInDescriptorsFromTableStart = 0;

        range[2].RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        range[2].NumDescriptors                    = 1;
        range[2].BaseShaderRegister                = 2;
        range[2].RegisterSpace                     = 0;
        range[2].OffsetInDescriptorsFromTableStart = 0;

        D3D12_ROOT_PARAMETER param[7] = {};
        param[0].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
        param[0].Descriptor.ShaderRegister = 0;
        param[0].Descriptor.RegisterSpace  = 0;
        param[0].ShaderVisibility          = D3D12_SHADER_VISIBILITY_VERTEX;

        param[1].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
        param[1].Descriptor.ShaderRegister = 1;
        param[1].Descriptor.RegisterSpace  = 0;
        param[1].ShaderVisibility          = D3D12_SHADER_VISIBILITY_PIXEL;

        param[2].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
        param[2].Descriptor.ShaderRegister = 2;
        param[2].Descriptor.RegisterSpace  = 0;
        param[2].ShaderVisibility          = D3D12_SHADER_VISIBILITY_PIXEL;

        param[3].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
        param[3].Descriptor.ShaderRegister = 3;
        param[3].Descriptor.RegisterSpace  = 0;
        param[3].ShaderVisibility          = D3D12_SHADER_VISIBILITY_PIXEL;

        param[4].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param[4].DescriptorTable.NumDescriptorRanges = 1;
        param[4].DescriptorTable.pDescriptorRanges   = &range[0];
        param[4].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_PIXEL;

        param[5].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param[5].DescriptorTable.NumDescriptorRanges = 1;
        param[5].DescriptorTable.pDescriptorRanges   = &range[1];
        param[5].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_PIXEL;

        param[6].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param[6].DescriptorTable.NumDescriptorRanges = 1;
        param[6].DescriptorTable.pDescriptorRanges   = &range[2];
        param[6].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter           = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.AddressV         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.AddressW         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.MipLODBias       = D3D12_DEFAULT_MIP_LOD_BIAS;
        sampler.MaxAnisotropy    = 1;
        sampler.ComparisonFunc   = D3D12_COMPARISON_FUNC_NEVER;
        sampler.BorderColor      = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        sampler.MinLOD           = -D3D12_FLOAT32_MAX;
        sampler.MaxLOD           = +D3D12_FLOAT32_MAX;
        sampler.ShaderRegister   = 0;
        sampler.RegisterSpace    = 0;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC desc = {};
        desc.NumParameters     = _countof(param);
        desc.NumStaticSamplers = 1;
        desc.pParameters       = param;
        desc.pStaticSamplers   = &sampler;
        desc.Flags             = flag;

        ComPtr<ID3DBlob> pBlob;
        ComPtr<ID3DBlob> pErrorBlob;

        auto hr = D3D12SerializeRootSignature(
            &desc,
            D3D_ROOT_SIGNATURE_VERSION_1,
            pBlob.GetAddressOf(),
            pErrorBlob.GetAddressOf());
        if (FAILED(hr))
        {
            return false;
        }

        hr = m_pDevice->CreateRootSignature(
            0,
            pBlob->GetBufferPointer(),
            pBlob->GetBufferSize(),
            IID_PPV_ARGS(m_pRootSig.GetAddressOf()));
        if (FAILED(hr))
        {
            ELOG("Error : Root Signature Create Failed. retcode = 0x%x", hr);
            return false;
        }
    }

    // pipeline state 생성
    {
        std::wstring vsPath;
        std::wstring psPath;

        vsPath = L"bin/CMake/Debug/SimpleVS.cso";
        psPath = L"bin/CMake/Debug/SimplePS.cso";

        ComPtr<ID3DBlob> pVSBlob;
        ComPtr<ID3DBlob> pPSBlob;

        auto hr = D3DReadFileToBlob(vsPath.c_str(), pVSBlob.GetAddressOf());
        if (FAILED(hr))
        {
            ELOG("Error : D3DReadFiledToBlob() Failed. path = %ls", vsPath.c_str());
            return false;
        }

        hr = D3DReadFileToBlob(psPath.c_str(), pPSBlob.GetAddressOf());
        if (FAILED(hr))
        {
            ELOG("Error : D3DReadFileToBlob() Failed. path = %ls", psPath.c_str());
            return false;
        }

        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
        desc.InputLayout           = MeshVertex::InputLayout;
        desc.pRootSignature        = m_pRootSig.Get();
        desc.VS                    = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
        desc.PS                    = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };
        desc.RasterizerState       = DirectX::CommonStates::CullNone;
        desc.BlendState            = DirectX::CommonStates::Opaque;
        desc.DepthStencilState     = DepthTarget::DepthDesc; //DirectX::CommonStates::DepthDefault;
        desc.SampleMask            = UINT_MAX;
        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets      = 1;
        desc.RTVFormats[0]         = m_ColorTarget[0].GetViewDesc().Format;
        desc.DSVFormat             = m_DepthTarget.GetViewDesc().Format;
        desc.SampleDesc.Count      = 1;
        desc.SampleDesc.Quality    = 0;

        hr = m_pDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(m_pPSO.GetAddressOf()));
        if (FAILED(hr))
        {
            ELOG("Error : ID3D12Device::CreateGraphicsPipelineState() Failed. retcode = 0x%x", hr);
            return false;
        }
    }

    // 변환행렬 설정 및 상수버퍼 생성
    {
        auto pCB = new (std::nothrow) ConstantBuffer();
        if (pCB == nullptr)
        {
            ELOG("Error : Out of memory.");
            return false;
        }

        if (!pCB->Init(m_pDevice.Get(), m_pPool[DescriptorPool::POOL_TYPE_RES], sizeof(Transform) * 2))
        {
            ELOG("Error : ConstantBuffer::Init() Failed.");
            return false;
        }

        // 카메라 설정
        auto eyePos = DirectX::XMVectorSet(0.0f, 0.4f, -2.0f, 0.0f); //DirectX::XMVectorSet(0.0f, 300.0f, -500.0f, 0.0f);
        auto targetPos = DirectX::XMVectorSet(0.0f, 0.4f, 0.0f, 0.0f);  //DirectX::XMVectorZero();
        auto upward = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

        constexpr auto fovY = DirectX::XMConvertToRadians(37.5f);
        auto aspect = static_cast<float>(m_Width) / static_cast<float>(m_Height);

        // 변환행렬 설정
        auto ptr = pCB->GetPtr<Transform>();
        ptr->World = DirectX::XMMatrixIdentity();
        ptr->View = DirectX::XMMatrixLookAtRH(eyePos, targetPos, upward);
        ptr->Proj = DirectX::XMMatrixPerspectiveFovRH(fovY, aspect, 1.0f, 1000.0f);

        m_Transform = pCB;

        m_TransformShadow.reserve(FrameCount);

        for (int i = 0; i < FrameCount; ++i)
        {
            auto pCB = new (std::nothrow) ConstantBuffer();
            if (pCB == nullptr)
            {
                ELOG("Error : Out of memory.");
                return false;
            }

            if (!pCB->Init(m_pDevice.Get(), m_pPool[DescriptorPool::POOL_TYPE_RES], sizeof(Transform) * 2))
            {
                ELOG("Error : ConstantBuffer::Init() Failed.");
                return false;
            }

            // 카메라 설정
            auto eyePos    = DirectX::XMVectorSet(0.0f, 0.4f, -2.0f, 0.0f); //DirectX::XMVectorSet(0.0f, 300.0f, -500.0f, 0.0f);
            auto targetPos = DirectX::XMVectorSet(0.0f, 0.4f, 0.0f, 0.0f);  //DirectX::XMVectorZero();
            auto upward    = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

            constexpr auto fovY = DirectX::XMConvertToRadians(37.5f);
            auto aspect = static_cast<float>(m_Width) / static_cast<float>(m_Height);

            // 변환행렬 설정
            auto ptr = pCB->GetPtr<Transform>();
            ptr->World = DirectX::XMMatrixIdentity();
            ptr->View = DirectX::XMMatrixLookAtRH(eyePos, targetPos, upward);
            ptr->Proj = DirectX::XMMatrixPerspectiveFovRH(fovY, aspect, 1.0f, 1000.0f);

            m_TransformShadow.push_back(pCB);
        }

        {
            auto pCB = new (std::nothrow) ConstantBuffer();
            if (pCB == nullptr)
            {
                ELOG("Error : Out of memory.");
                return false;
            }

            if (!pCB->Init(
                m_pDevice.Get(),
                m_pPool[DescriptorPool::POOL_TYPE_RES],
                sizeof(LightBuffer)))
            {
                ELOG("Error : ConstantBuffer::Init() Failed.");
                return false;
            }

            auto ptr = pCB->GetPtr<LightBuffer>();

            //ptr->DirLight.Direction  = DirectX::XMFLOAT3(0.0f, 300.0f, -150.0f);
            //ptr->DirLight.Color      = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);   // 1.0f, 0.753f, 0.796f

            //ptr->PointLight.Position = DirectX::XMFLOAT3(0.0f, 300.0f, -150.0f);
            //ptr->PointLight.Color    = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
            //ptr->PointLight.Range    = 300.0f;

            //ptr->SpotLight.Position  = DirectX::XMFLOAT3(0.0f, 20.0f, -50.0f);
            //ptr->SpotLight.Direction = DirectX::XMFLOAT3(1.0f, -1.0f, 1.0f);
            //ptr->SpotLight.Color     = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
            //ptr->SpotLight.Range     = 0.0f;
            //ptr->SpotLight.SpotPower = 0.0f;

            m_pLight = pCB;

            m_RotateAngle = 0.0f;
        }

        {
            auto pCB = new (std::nothrow) ConstantBuffer();
            if (pCB == nullptr)
            {
                ELOG("Error : Out of memory.");
                return false;
            }

            if (!pCB->Init(
                m_pDevice.Get(),
                m_pPool[DescriptorPool::POOL_TYPE_RES],
                sizeof(PassConstantBuffer)))
            {
                ELOG("Error : ConstantBuffer::Init() Failed.");
                return false;
            }

            auto ptr = pCB->GetPtr<PassConstantBuffer>();

            ptr->CameraPosition     = DirectX::XMFLOAT3(0.0f, 0.4f, -2.0f);
            ptr->AmbientLight       = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
            ptr->DiffuseMapUsable   = 0;
            ptr->SpecularMapUsable  = 0;
            ptr->ShininessMapUsable = 0;
            ptr->NormalMapUsable    = 0;

            m_pPass = pCB;
        }
    }

    return true;
}

void Renderer::TermD3D()
{
    m_Fence.Sync(m_pQueue.Get());
    m_Fence.Term();

    for (int i = 0u; i < FrameCount; ++i)
    {
        m_ColorTarget[i].Term();
    }

    m_DepthTarget.Term();
    m_CommandList.Term();

    for (int i = 0; i < DescriptorPool::POOL_COUNT; ++i)
    {
        if (m_pPool[i] != nullptr)
        {
            m_pPool[i]->Release();
            m_pPool[i] = nullptr;
        }
    }

    m_pSwapChain.Reset();
    m_pQueue.Reset();
    m_pDevice.Reset();
}

void Renderer::CreateSwapChain()
{
    m_pSwapChain.Reset();

    ComPtr<IDXGIFactory4> pFactory;
    auto hr = CreateDXGIFactory1(IID_PPV_ARGS(pFactory.GetAddressOf()));
    if (FAILED(hr))
        __debugbreak();

    DXGI_SWAP_CHAIN_DESC scDesc = {};
    scDesc.BufferDesc.Width                   = m_Width;
    scDesc.BufferDesc.Height                  = m_Height;
    scDesc.BufferDesc.RefreshRate.Numerator   = 60;
    scDesc.BufferDesc.RefreshRate.Denominator = 1;
    scDesc.BufferDesc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    scDesc.BufferDesc.Scaling                 = DXGI_MODE_SCALING_UNSPECIFIED;
    scDesc.BufferDesc.Format                  = BackBufferFormat;
    scDesc.SampleDesc.Count                   = 1;
    scDesc.SampleDesc.Quality                 = 0;
    scDesc.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.BufferCount                        = FrameCount;
    scDesc.OutputWindow                       = m_hWnd;
    scDesc.Windowed                           = TRUE;
    scDesc.SwapEffect                         = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scDesc.Flags                              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    ComPtr<IDXGISwapChain> pSwapChain;
    hr = pFactory->CreateSwapChain(
        m_pQueue.Get(), 
        &scDesc, 
        pSwapChain.GetAddressOf());
    if (FAILED(hr))
        __debugbreak();

    hr = pSwapChain.As(&m_pSwapChain);
    if (FAILED(hr))
        __debugbreak();

    m_FrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();

    pFactory.Reset();
    pSwapChain.Reset();
}

void Renderer::Present(uint32_t interval)
{
    m_pSwapChain->Present(interval, 0);
    m_Fence.Wait(m_pQueue.Get(), INFINITE);
    m_FrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();
}

extern "C" DLL_API IRenderer * CreateRenderer
(
    HINSTANCE   hInst,
    HWND        hWnd,
    uint32_t    width,
    uint32_t    height
)
{
    return new Renderer(hInst, hWnd, width, height);
}