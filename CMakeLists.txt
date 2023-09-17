cmake_minimum_required(VERSION 3.11)

project( RenderingEngine
    VERSION 0.1
    DESCRIPTION "Rendering Engine Project"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/CMake")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/CMake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/CMake")

add_compile_definitions(DLL_EXPORTS)
add_compile_definitions(NOMINMAX)

set( HEADER_FILES
    include/CommandList.h
    include/ComPtr.h
    include/ConstantBuffer.h
    include/DepthTarget.h
    include/DescriptorPool.h
    include/DllExport.h
    include/Fence.h
    include/FileUtil.h
    include/framework.h
    include/IndexBuffer.h
    include/Logger.h
    include/Material.h
    include/Mesh.h
    include/Pool.h
    include/Renderer.h
    include/RenderTarget.h
    include/ResMesh.h
    include/rndEngine.h
    include/ShaderUtil.h
    include/Texture.h
    include/VertexBuffer.h
    include/WinPixUtil.h
)

set( SOURCE_FILES
    src/dllmain.cpp
    src/CommandList.cpp
    src/ConstantBuffer.cpp
    src/DepthTarget.cpp
    src/DescriptorPool.cpp
    src/Fence.cpp
    src/FileUtil.cpp
    src/IndexBuffer.cpp
    src/Logger.cpp
    src/Material.cpp
    src/Mesh.cpp
    src/Renderer.cpp
    src/RenderTarget.cpp
    src/ResMesh.cpp
    src/ShaderUtil.cpp
    src/Texture.cpp
    src/VertexBuffer.cpp
    src/WinPixUtil.cpp
)

set( SHADER_FILES
    res/SimpleVS.hlsl
    res/SimplePS.hlsl
)

source_group("Shader Files" FILES ${SHADER_FILES})

set_source_files_properties( res/SimpleVS.hlsl PROPERTIES 
    VS_SHADER_TYPE Vertex
    VS_SHADER_MODEL 5.0
    VS_SHADER_ENTRYPOINT main
    VS_SHADER_DISABLE_OPTIMIZATIONS YES
    VS_SHADER_ENABLE_DEBUG YES
)

set_source_files_properties( res/SimplePS.hlsl PROPERTIES 
    VS_SHADER_TYPE Pixel
    VS_SHADER_MODEL 5.0
    VS_SHADER_ENTRYPOINT main
    VS_SHADER_DISABLE_OPTIMIZATIONS YES
    VS_SHADER_ENABLE_DEBUG YES
)

add_library(${PROJECT_NAME} SHARED ${HEADER_FILES} ${SOURCE_FILES} ${SHADER_FILES})

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/assimp)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/DirectXTK12)

target_include_directories( ${PROJECT_NAME} PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# target_link_directories( ${PROJECT_NAME} PUBLIC 
    # ${CMAKE_BINARY_DIR}/bin/CMake/Debug
# )

target_link_libraries( ${PROJECT_NAME} PUBLIC
    d3d12.lib
    dxgi.lib
    dxguid.lib
    d3dcompiler.lib
    runtimeobject.lib
    shlwapi.lib
    assimp
    DirectXTK12
)

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _UNICODE UNICODE _WIN32_WINNT=0x0A00)
endif()