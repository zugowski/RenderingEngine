#include <Windows.Foundation.h>
#include <wrl/wrappers/corewrappers.h>
#include <wrl/client.h>
#include <CommonStates.h>
#include <Renderer.h>
#include <ShaderUtil.h>
#include <WinPixUtil.h>
#include <FileUtil.h>
#include <Logger.h>

D3D_FEATURE_LEVEL IRenderer::FeatureLevel = D3D_FEATURE_LEVEL_12_0;
DXGI_FORMAT IRenderer::BackBufferFormat   = DXGI_FORMAT_R8G8B8A8_UNORM;
UINT IRenderer::Msaa4xQuality             = 0;
DirectX::XMFLOAT3 IRenderer::EyePos = DirectX::XMFLOAT3(0.0f, 0.4f, -2.0f);

Renderer::Renderer(HINSTANCE hInst, HWND hWnd, uint32_t width, uint32_t height)
    : m_hInst(hInst)
    , m_hWnd(hWnd)
    , m_Width(width)
    , m_Height(height)
    , m_pDevice(nullptr)
    , m_FrameIndex(0)
    , m_CurrFrameResIndex(0)
    , m_RotateAngle(0.0f)
{
    // 필수적인 요소 초기화
    InitD3DComponent();

    // 추가적인 요소 초기화
    InitD3DAsset();
}

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

            Pass.DiffuseMapUsable = 1;
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

            Pass.NormalMapUsable = 1;
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

            Pass.SpecularMapUsable = 1;
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
    
    BuildRenderItems();
    BuildFrameResources();

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
    Update();
    Draw();
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

    // command list, command allocator 생성 (제거 예정)
    m_CommandList.Init(
        m_pDevice.Get(), 
        D3D12_COMMAND_LIST_TYPE_DIRECT, 
        FrameCount);

    // command allocator, command list 생성
    hr = m_pDevice->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(m_pDirCmdAllocator.GetAddressOf()));
    if (FAILED(hr))
        __debugbreak();

    hr = m_pDevice->CreateCommandList(
        1,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        m_pDirCmdAllocator.Get(),
        nullptr,
        IID_PPV_ARGS(m_pCmdList.GetAddressOf()));
    if (FAILED(hr))
        __debugbreak();

    m_pCmdList->Close();

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
    
    auto eyePos    = DirectX::XMVectorSet(EyePos.x, EyePos.y, EyePos.z, 0.0f);
    auto targetPos = DirectX::XMVectorSet(0.0f, 0.4f, 0.0f, 0.0f);
    auto upward    = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    constexpr auto fovY = DirectX::XMConvertToRadians(37.5f);
    auto aspect = static_cast<float>(m_Width) / static_cast<float>(m_Height);

    Transform.World = DirectX::XMMatrixIdentity();
    Transform.View  = DirectX::XMMatrixLookAtRH(eyePos, targetPos, upward);
    Transform.Proj  = DirectX::XMMatrixPerspectiveFovRH(fovY, aspect, 1.0f, 1000.0f);

    Light.DirLight.Direction  = DirLightInitDir;
    Light.DirLight.Color      = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);

    Light.PointLight.Position = PointLightInitPos;
    Light.PointLight.Color    = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
    Light.PointLight.Range    = PointLightInitRange;

    Light.SpotLight.Position  = SpotLightInitPos;
    Light.SpotLight.Direction = SpotLightInitDir;
    Light.SpotLight.Color     = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
    Light.SpotLight.Range     = SpotLightInitRange;
    Light.SpotLight.SpotPower = SpotLightInitSpotPower;

    Pass.CameraPosition     = EyePos;

    BuildFrameResources();

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

void Renderer::BuildRenderItems()
{
    auto eyePos    = DirectX::XMLoadFloat3(&EyePos);
    auto targetPos = DirectX::XMVectorSet(0.0f, 0.4f, 0.0f, 0.0f);
    auto upward    = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    constexpr auto fovY = DirectX::XMConvertToRadians(37.5f);
    auto aspect = static_cast<float>(m_Width) / static_cast<float>(m_Height);

    const DirectX::XMMATRIX S1 = DirectX::XMMatrixScaling(Mesh::Scale, Mesh::Scale, Mesh::Scale);
    //const DirectX::XMMATRIX R = DirectX::XMMatrixRotationY(m_RotateAngle);

    const DirectX::XMVECTOR shadowPlane = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    const DirectX::XMVECTOR dirLightDir = DirectX::XMLoadFloat3(&Light.DirLight.Direction);
    const DirectX::XMMATRIX S2 = DirectX::XMMatrixShadow(shadowPlane, dirLightDir);

    int dataIdx = 0;

    // mesh
    for (int i = 0; i < m_pMesh.size(); ++i)
    {
        RenderItem rItem;
        rItem.IsShadow = false;
        rItem.MeshIdx  = i;
        rItem.DataIdx  = dataIdx++;
        rItem.Transform.World = S1;
        rItem.Transform.View  = DirectX::XMMatrixLookAtRH(eyePos, targetPos, upward);
        rItem.Transform.Proj  = DirectX::XMMatrixPerspectiveFovRH(fovY, aspect, 1.0f, 1000.0f);
        m_RenderItems.push_back(rItem);
    }

    // shadow
    for (int i = 0; i < m_pMesh.size(); ++i)
    {
        RenderItem rItem;
        rItem.IsShadow = true;
        rItem.MeshIdx  = i;
        rItem.DataIdx  = dataIdx++;
        rItem.Transform.World = S1 * S2;
        rItem.Transform.View  = DirectX::XMMatrixLookAtRH(eyePos, targetPos, upward);
        rItem.Transform.Proj  = DirectX::XMMatrixPerspectiveFovRH(fovY, aspect, 1.0f, 1000.0f);
        m_RenderItems.push_back(rItem);
    }
}

void Renderer::BuildFrameResources()
{
    m_FrameResources.clear();
    for (int i = 0; i < FrameResourceCount; ++i)
    {
        FrameResource* res = new FrameResource(
            m_pDevice.Get(),
            m_pPool,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            m_RenderItems.size() == 0 ? 1 : m_RenderItems.size());
        m_FrameResources.push_back(res);
    }
}

void Renderer::Update()
{
    m_CurrFrameResIndex = (m_CurrFrameResIndex + 1) % FrameResourceCount;
    m_CurrFrameRes = m_FrameResources[m_CurrFrameResIndex];

    m_Fence.Wait(m_CurrFrameRes->Fence, INFINITE);

    for (int i = 0; i < m_RenderItems.size(); ++i)
    {
        auto& rItem = m_RenderItems[i];
        //rItem.Transform = Transform;

        const DirectX::XMMATRIX S1 = DirectX::XMMatrixScaling(Mesh::Scale, Mesh::Scale, Mesh::Scale);
        const DirectX::XMMATRIX R = DirectX::XMMatrixRotationY(m_RotateAngle);

        if (!rItem.IsShadow)
        {
            rItem.Transform.World = S1 * R;
        }
        else
        {
            const DirectX::XMVECTOR shadowPlane = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
            const DirectX::XMVECTOR dirLightDir = DirectX::XMLoadFloat3(&Light.DirLight.Direction);
            const DirectX::XMMATRIX S2 = DirectX::XMMatrixShadow(shadowPlane, dirLightDir);
            rItem.Transform.World = S1 * S2 * R;
        }

        rItem.Light     = rItem.IsShadow ? rItem.Light : Light;
        rItem.Material  = *(m_Material.GetBufferPtr<MaterialBuffer>(rItem.MeshIdx));
        rItem.Pass      = Pass;
    }

    m_RotateAngle += 0.010f;

    UpdateTransform();
    UpdateLight();
    UpdateMaterial();
    UpdatePass();
}

void Renderer::UpdateTransform()
{
    for (int i = 0; i < m_RenderItems.size(); ++i)
    {
        const auto& rItem = m_RenderItems[i];
        m_CurrFrameRes->Transform.CopyData(rItem.DataIdx, rItem.Transform);
    }
}

void Renderer::UpdateLight()
{
    for (int i = 0; i < m_RenderItems.size(); ++i)
    {
        const auto& rItem = m_RenderItems[i];
        m_CurrFrameRes->Light.CopyData(rItem.DataIdx, rItem.Light);
    }
}

void Renderer::UpdateMaterial()
{
    for (int i = 0; i < m_RenderItems.size(); ++i)
    {
        const auto& rItem = m_RenderItems[i];
        m_CurrFrameRes->Material.CopyData(rItem.DataIdx, rItem.Material);
    }
}

void Renderer::UpdatePass()
{
    for (int i = 0; i < m_RenderItems.size(); ++i)
    {
        const auto& rItem = m_RenderItems[i];
        m_CurrFrameRes->Pass.CopyData(rItem.DataIdx, rItem.Pass);
    }
}

void Renderer::Draw()
{
    auto pCmdAllocator = m_CurrFrameRes->Allocator;
    auto hr = pCmdAllocator->Reset();
    if (FAILED(hr))
        __debugbreak();

    hr = m_pCmdList->Reset(pCmdAllocator.Get(), nullptr);
    if (FAILED(hr))
        __debugbreak();

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = m_ColorTarget[m_FrameIndex].GetResource();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    m_pCmdList->ResourceBarrier(1, &barrier);

    auto handleRTV = m_ColorTarget[m_FrameIndex].GetHandleRTV();
    auto handleDSV = m_DepthTarget.GetHandleDSV();

    m_pCmdList->OMSetRenderTargets(1, &handleRTV->HandleCPU, FALSE, &handleDSV->HandleCPU);

    float clearColor[] = { 0.0f, 0.439f, 0.439f, 1.0f };    // 0.0, 0.52, 0.52
    m_pCmdList->ClearRenderTargetView(handleRTV->HandleCPU, clearColor, 0, nullptr);
    m_pCmdList->ClearDepthStencilView(handleDSV->HandleCPU, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    {
        ID3D12DescriptorHeap* const pHeaps[] = {
            m_pPool[DescriptorPool::POOL_TYPE_RES]->GetHeap()
        };

        m_pCmdList->SetGraphicsRootSignature(m_pRootSig.Get());
        m_pCmdList->SetDescriptorHeaps(1, pHeaps);
        m_pCmdList->SetPipelineState(m_pPSO.Get());
        m_pCmdList->RSSetViewports(1, &m_Viewport);
        m_pCmdList->RSSetScissorRects(1, &m_Scissor);

        DrawRenderItems();
    }

    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = m_ColorTarget[m_FrameIndex].GetResource();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    m_pCmdList->ResourceBarrier(1, &barrier);

    m_pCmdList->Close();

    ID3D12CommandList* pLists[] = { m_pCmdList.Get() };
    m_pQueue->ExecuteCommandLists(1, pLists);

    m_pSwapChain->Present(1, 0);
    m_FrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();
    m_CurrFrameRes->Fence = m_Fence.Signal(m_pQueue.Get());
}

void Renderer::DrawRenderItems()
{
    const auto pTransform = m_CurrFrameRes->Transform.GetAddress();
    const auto pLight     = m_CurrFrameRes->Light.GetAddress();
    const auto pMaterial  = m_CurrFrameRes->Material.GetAddress();
    const auto pPass      = m_CurrFrameRes->Pass.GetAddress();

    for (int i = 0; i < m_RenderItems.size(); ++i)
    {
        const auto& rItem = m_RenderItems[i];
        const int id = m_pMesh[rItem.MeshIdx]->GetMaterialId();
        const int dataIdx = rItem.DataIdx;

        const UINT64 transformSize = m_CurrFrameRes->Transform.GetElementSize();
        const UINT64 lightSize     = m_CurrFrameRes->Light.GetElementSize();
        const UINT64 materialSize  = m_CurrFrameRes->Material.GetElementSize();
        const UINT64 passSize      = m_CurrFrameRes->Pass.GetElementSize();

        m_pCmdList->SetGraphicsRootConstantBufferView(0, pTransform + dataIdx * transformSize);
        m_pCmdList->SetGraphicsRootConstantBufferView(1, pLight + dataIdx * lightSize);
        m_pCmdList->SetGraphicsRootConstantBufferView(2, pMaterial + dataIdx * materialSize);
        m_pCmdList->SetGraphicsRootConstantBufferView(3, pPass + dataIdx * passSize);
        m_pCmdList->SetGraphicsRootDescriptorTable(4, m_Material.GetTextureHandle(id, TU_DIFFUSE));
        m_pCmdList->SetGraphicsRootDescriptorTable(5, m_Material.GetTextureHandle(id, TU_NORMAL));
        m_pCmdList->SetGraphicsRootDescriptorTable(6, m_Material.GetTextureHandle(id, TU_SPECULAR));
        m_pMesh[rItem.MeshIdx]->Draw(m_pCmdList.Get());
    }
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