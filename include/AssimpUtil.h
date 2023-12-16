#pragma once

#include <DirectXMath.h>
#include <assimp/postprocess.h>

namespace AssimpUtil
{
    DirectX::XMMATRIX ConvertToXMMATRIX(const aiMatrix4x4& mat)
    {
        return DirectX::XMLoadFloat4x4(&DirectX::XMFLOAT4X4(
            mat.a1, mat.b1, mat.c1, mat.d1,
            mat.a2, mat.b2, mat.c2, mat.d2,
            mat.a3, mat.b3, mat.c3, mat.d3,
            mat.a4, mat.b4, mat.c4, mat.d4));
    }
}