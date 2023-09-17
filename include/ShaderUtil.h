#pragma once

#include <d3d12.h>
#include <d3dcompiler.h>

HRESULT CreateShaderFromFile(const WCHAR* csoFileNameInOut, const WCHAR* hlslFileName,
    LPCSTR entryPoint, LPCSTR shaderModel, ID3DBlob** ppBlobOut);