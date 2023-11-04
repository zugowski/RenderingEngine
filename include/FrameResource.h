#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <ComPtr.h>
#include <ConstantBuffer.h>
#include <Material.h>
#include <Fence.h>

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

class FrameResource
{
public:
    FrameResource(
        ID3D12Device* pDevice,
        DescriptorPool** pPool,
        D3D12_COMMAND_LIST_TYPE type);
    //FrameResource(const FrameResource& rhs) = default;
    //FrameResource& operator=(const FrameResource& rhs) = default;
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