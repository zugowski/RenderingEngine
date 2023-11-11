#include <FrameResource.h>
#include <DescriptorPool.h>

FrameResource::FrameResource
(
    ID3D12Device* pDevice,
    DescriptorPool** pPool,
    D3D12_COMMAND_LIST_TYPE type,
    int count
) : Fence(0)
{
    auto hr = pDevice->CreateCommandAllocator(
        type, IID_PPV_ARGS(Allocator.GetAddressOf()));
    if (FAILED(hr))
        __debugbreak();

    if (!Transform.Init(
        pDevice,
        pPool[DescriptorPool::POOL_TYPE_RES],
        sizeof(TransformBuffer),
        count))
    {
        __debugbreak();
    }

    if (!Light.Init(
        pDevice,
        pPool[DescriptorPool::POOL_TYPE_RES],
        sizeof(LightBuffer),
        count))
    {
        __debugbreak();
    }

    if (!Material.Init(
        pDevice,
        pPool[DescriptorPool::POOL_TYPE_RES],
        sizeof(MaterialBuffer),
        count))
    {
        __debugbreak();
    }

    if (!Pass.Init(
        pDevice,
        pPool[DescriptorPool::POOL_TYPE_RES],
        sizeof(PassConstant),
        count))
    {
        __debugbreak();
    }
}

FrameResource::~FrameResource()
{

}