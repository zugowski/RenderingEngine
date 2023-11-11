#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <ComPtr.h>
#include <Material.h>
#include <ConstantBuffer.h>

struct TransformBuffer
{
    DirectX::XMMATRIX World;
    DirectX::XMMATRIX View;
    DirectX::XMMATRIX Proj;

    TransformBuffer()
    {
        World = DirectX::XMMatrixIdentity();
        View  = DirectX::XMMatrixIdentity();
        Proj  = DirectX::XMMatrixIdentity();
    }
};

struct LightItem
{
    DirectX::XMFLOAT3 Color;     // 빛의 세기
    float Range;                 // 포인트/스포트라이트 전용
    DirectX::XMFLOAT3 Direction; // 디렉셔널/스포트라이트 전용
    float SpotPower;             // 스포트라이트 전용
    DirectX::XMFLOAT3 Position;  // 포인트라이트 전용
    float pad;

    LightItem()
    {
        Color     = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
        Range     = 0.0f;
        Direction = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
        SpotPower = 0.0f;
        Position  = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
        pad       = 0.0f;
    }
};

struct LightBuffer
{
    LightItem DirLight;
    LightItem PointLight;
    LightItem SpotLight;
};

struct MaterialBuffer
{
    DirectX::XMFLOAT3 Diffuse;
    float             Alpha;
    DirectX::XMFLOAT3 Specular;
    float             Shininess;

    MaterialBuffer()
    {
        Diffuse   = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
        Alpha     = 0.0f;
        Specular  = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
        Shininess = 0.0f;
    }
};

struct PassConstant
{
    alignas(16) DirectX::XMFLOAT3 CameraPosition;
    alignas(16) DirectX::XMFLOAT4 AmbientLight;
    int DiffuseMapUsable;
    int SpecularMapUsable;
    int ShininessMapUsable;
    int NormalMapUsable;

    PassConstant()
    {
        CameraPosition     = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
        AmbientLight       = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
        DiffuseMapUsable   = 0;
        SpecularMapUsable  = 0;
        ShininessMapUsable = 0;
        NormalMapUsable    = 0;
    }
};

class FrameResource
{
public:
    FrameResource(
        ID3D12Device* pDevice,
        DescriptorPool** pPool,
        D3D12_COMMAND_LIST_TYPE type,
        int count);
    FrameResource(const FrameResource& rhs) = default;
    FrameResource& operator=(const FrameResource& rhs) = default;
    ~FrameResource();

public:
    ComPtr<ID3D12CommandAllocator> Allocator;

    // GPU가 주어진 명령을 처리하는 동안 다음 프레임을 위한 자원을 CPU가 업데이트
    // 할 수 없기 때문에 FrameResource 클래스에서 별도로 자원을 할당함
    ConstantBuffer Transform;
    ConstantBuffer Light;
    ConstantBuffer Material;
    ConstantBuffer Pass;

    UINT64 Fence;
};