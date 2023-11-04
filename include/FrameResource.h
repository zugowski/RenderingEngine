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
    DirectX::XMFLOAT3 Color;     // ���� ����
    float Range;                 // ����Ʈ/����Ʈ����Ʈ ����
    DirectX::XMFLOAT3 Direction; // �𷺼ų�/����Ʈ����Ʈ ����
    float SpotPower;             // ����Ʈ����Ʈ ����
    DirectX::XMFLOAT3 Position;  // ����Ʈ����Ʈ ����
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

    // GPU�� �־��� ����� ó���ϴ� ���� ���� �������� ���� �ڿ��� CPU�� ������Ʈ
    // �� �� ���� ������ FrameResource Ŭ�������� ������ �ڿ��� �Ҵ���
    ConstantBuffer Transform;
    ConstantBuffer Light;
    ConstantBuffer Material;
    ConstantBuffer Pass;

    UINT64 Fence;
};