
#include "AttoAsset.h"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/std_image.h>

#include <string>

namespace atto {

    bool LeEngine::InitializeRenderer() {
        if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&renderer.factory)))) {
            ATTOFATAL("DXGI: Unable to create DXGIFactory")
                return false;
        }

        UINT deviceFlags = D3D11_CREATE_DEVICE_FLAG::D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if ATTO_DEBUG_RENDERING
        deviceFlags |= D3D11_CREATE_DEVICE_FLAG::D3D11_CREATE_DEVICE_DEBUG;
#endif

        D3D_FEATURE_LEVEL deviceFeatureLevel = D3D_FEATURE_LEVEL_11_1;
        if (FAILED(D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            deviceFlags,
            &deviceFeatureLevel,
            1,
            D3D11_SDK_VERSION,
            &renderer.device,
            nullptr,
            &renderer.context)))
        {
            ATTOFATAL("DX11: Unable to create device");
            return false;
        }

        renderer.swapChainWidth = app->windowWidth;
        renderer.swapChainHeight = app->windowHeight;

        DXGI_SWAP_CHAIN_DESC1 swapChainDescriptor = {};
        swapChainDescriptor.Width = app->windowWidth;
        swapChainDescriptor.Height = app->windowHeight;
        swapChainDescriptor.Format = DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM;
        swapChainDescriptor.SampleDesc.Count = 1;
        swapChainDescriptor.SampleDesc.Quality = 0;
        swapChainDescriptor.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDescriptor.BufferCount = 2;
        swapChainDescriptor.SwapEffect = DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDescriptor.Scaling = DXGI_SCALING::DXGI_SCALING_STRETCH;
        swapChainDescriptor.Flags = {};

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullscreenDescriptor = {};
        swapChainFullscreenDescriptor.Windowed = true;

        if (FAILED(renderer.factory->CreateSwapChainForHwnd(
            renderer.device.Get(),
            glfwGetWin32Window(app->window),
            &swapChainDescriptor,
            &swapChainFullscreenDescriptor,
            nullptr,
            &renderer.swapChain)))
        {
            ATTOFATAL("DX11: Unable to create swap chain");
            return false;
        }

        wrl::ComPtr<ID3D11Texture2D> backBuffer = nullptr;
        if (FAILED(renderer.swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)))) {
            ATTOFATAL("DX11: Unable to get back buffer");
            return false;
        }

        if (FAILED(renderer.device->CreateRenderTargetView(backBuffer.Get(), nullptr, &renderer.swapChainRenderTarget))) {
            ATTOFATAL("DX11: Unable to create render target view");
            return false;
        }

        // Create depth buffer
        D3D11_TEXTURE2D_DESC depthBufferDescriptor = {};
        depthBufferDescriptor.Width = app->windowWidth;
        depthBufferDescriptor.Height = app->windowHeight;
        depthBufferDescriptor.MipLevels = 1;
        depthBufferDescriptor.ArraySize = 1;
        depthBufferDescriptor.Format = DXGI_FORMAT::DXGI_FORMAT_D16_UNORM;
        depthBufferDescriptor.SampleDesc.Count = 1;
        depthBufferDescriptor.SampleDesc.Quality = 0;
        depthBufferDescriptor.Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
        depthBufferDescriptor.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_DEPTH_STENCIL;
        depthBufferDescriptor.CPUAccessFlags = 0;
        depthBufferDescriptor.MiscFlags = 0;

        if (FAILED(renderer.device->CreateTexture2D(&depthBufferDescriptor, nullptr, &renderer.swapChainDepthBuffer))) {
            ATTOFATAL("DX11: Unable to create depth buffer");
            return false;
        }

        D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDescriptor = {};
        depthStencilViewDescriptor.Format = DXGI_FORMAT::DXGI_FORMAT_D16_UNORM;
        depthStencilViewDescriptor.ViewDimension = D3D11_DSV_DIMENSION::D3D11_DSV_DIMENSION_TEXTURE2D;
        depthStencilViewDescriptor.Texture2D.MipSlice = 0;
        
        if (FAILED(renderer.device->CreateDepthStencilView(renderer.swapChainDepthBuffer.Get(), &depthStencilViewDescriptor, &renderer.swapChainDepthView))) {
            ATTOFATAL("DX11: Unable to create depth stencil view");
            return false;
        }

        D3D11_FEATURE_DATA_THREADING threadingOptions = {};
        renderer.device->CheckFeatureSupport(D3D11_FEATURE_THREADING, &threadingOptions, sizeof(threadingOptions));
        if (!threadingOptions.DriverCommandLists || !threadingOptions.DriverConcurrentCreates) {
            ATTOFATAL("Your GPU does not support mutlithreading ! Try updating your drivers.");
            return false;
        }

        // Create Sampler States
        {
            D3D11_SAMPLER_DESC pointWrap = {};
            pointWrap.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
            pointWrap.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
            pointWrap.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
            pointWrap.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

            HRESULT hr = renderer.device->CreateSamplerState(&pointWrap, &renderer.samplerStates.pointWrap);
            if (FAILED(hr)) {
                ATTOFATAL("DX11: Unable to create sampler state");
                return false;
            }

            D3D11_SAMPLER_DESC pointClamp = {};
            pointClamp.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
            pointClamp.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
            pointClamp.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
            pointClamp.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
            
            hr = renderer.device->CreateSamplerState(&pointClamp, &renderer.samplerStates.pointClamp);
            if (FAILED(hr)) {
                ATTOFATAL("DX11: Unable to create sampler state");
                return false;
            }
            
            D3D11_SAMPLER_DESC linearWrap = {};
            linearWrap.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            linearWrap.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
            linearWrap.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
            linearWrap.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
            
            hr = renderer.device->CreateSamplerState(&linearWrap, &renderer.samplerStates.linearWrap);
            if (FAILED(hr)) {
                ATTOFATAL("DX11: Unable to create sampler state");
                return false;
            }
            
            D3D11_SAMPLER_DESC linearClamp = {};
            linearClamp.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            linearClamp.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
            linearClamp.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
            linearClamp.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
            
            hr = renderer.device->CreateSamplerState(&linearClamp, &renderer.samplerStates.linearClamp);
            if (FAILED(hr)) {
                ATTOFATAL("DX11: Unable to create sampler state");
                return false;
            }
            
            D3D11_SAMPLER_DESC anisotropicWrap = {};
            anisotropicWrap.Filter = D3D11_FILTER_ANISOTROPIC;
            anisotropicWrap.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
            anisotropicWrap.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
            anisotropicWrap.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
            anisotropicWrap.MaxAnisotropy = 4;
            
            hr = renderer.device->CreateSamplerState(&anisotropicWrap, &renderer.samplerStates.anisotropicWrap);
            if (FAILED(hr)) {
                ATTOFATAL("DX11: Unable to create sampler state");
                return false;
            }
            
            D3D11_SAMPLER_DESC anisotropicClamp = {};
            anisotropicClamp.Filter = D3D11_FILTER_ANISOTROPIC;
            anisotropicClamp.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
            anisotropicClamp.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
            anisotropicClamp.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
            anisotropicClamp.MaxAnisotropy = 4;
            
            hr = renderer.device->CreateSamplerState(&anisotropicClamp, &renderer.samplerStates.anisotropicClamp);
            if (FAILED(hr)) {
                ATTOFATAL("DX11: Unable to create sampler state");
                return false;
            }

            // Bind samplers
            ID3D11SamplerState* samplers[] = {
                renderer.samplerStates.pointWrap.Get(),
                renderer.samplerStates.pointClamp.Get(),
                renderer.samplerStates.linearWrap.Get(),
                renderer.samplerStates.linearClamp.Get(),
                renderer.samplerStates.anisotropicWrap.Get(),
                renderer.samplerStates.anisotropicClamp.Get()
            };

            renderer.context->PSSetSamplers(0, 6, samplers);
        }

        // Create Depth States
        {
            D3D11_DEPTH_STENCIL_DESC depthStateDesc = {};
            depthStateDesc.DepthEnable = true;
            depthStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
            depthStateDesc.DepthFunc = D3D11_COMPARISON_LESS;
            depthStateDesc.StencilEnable = false;
            depthStateDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
            depthStateDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
            depthStateDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
            depthStateDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
            depthStateDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
            depthStateDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
            depthStateDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
            depthStateDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
            depthStateDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
            depthStateDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;

            HRESULT hr = renderer.device->CreateDepthStencilState(&depthStateDesc, &renderer.depthStates.depthEnabled);
            if (FAILED(hr)) {
                ATTOFATAL("DX11: Unable to create depth state");
                return false;
            }

            depthStateDesc.DepthEnable = false;
            hr = renderer.device->CreateDepthStencilState(&depthStateDesc, &renderer.depthStates.depthDisabled);
            if (FAILED(hr)) {
                ATTOFATAL("DX11: Unable to create depth state");
                return false;
            }

            depthStateDesc.DepthEnable = true;
            depthStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
            hr = renderer.device->CreateDepthStencilState(&depthStateDesc, &renderer.depthStates.depthEnabledNoWrite);
            if (FAILED(hr)) {
                ATTOFATAL("DX11: Unable to create depth state");
                return false;
            }
        }

        // Create rasterizer states
        {
            D3D11_RASTERIZER_DESC rasterizerDesc = {};
            rasterizerDesc.FillMode = D3D11_FILL_SOLID;
            rasterizerDesc.CullMode = D3D11_CULL_BACK;
            rasterizerDesc.FrontCounterClockwise = true;
            rasterizerDesc.DepthBias = 0;
            rasterizerDesc.DepthBiasClamp = 0.0f;
            rasterizerDesc.SlopeScaledDepthBias = 0.0f;
            rasterizerDesc.DepthClipEnable = true;
            rasterizerDesc.ScissorEnable = false;
            rasterizerDesc.MultisampleEnable = false;
            rasterizerDesc.AntialiasedLineEnable = false;

            HRESULT hr = renderer.device->CreateRasterizerState(&rasterizerDesc, &renderer.rasterizerStates.cullBack);
            if (FAILED(hr)) {
                ATTOFATAL("DX11: Unable to create rasterizer state");
                return false;
            }

            rasterizerDesc.CullMode = D3D11_CULL_FRONT;
            hr = renderer.device->CreateRasterizerState(&rasterizerDesc, &renderer.rasterizerStates.cullFront);
            if (FAILED(hr)) {
                ATTOFATAL("DX11: Unable to create rasterizer state");
                return false;
            }

            rasterizerDesc.CullMode = D3D11_CULL_NONE;
            hr = renderer.device->CreateRasterizerState(&rasterizerDesc, &renderer.rasterizerStates.cullNone);
            if (FAILED(hr)) {
                ATTOFATAL("DX11: Unable to create rasterizer state");
                return false;
            }
        }

        // Blend states
        {
            D3D11_BLEND_DESC blendDesc = {};
            blendDesc.AlphaToCoverageEnable = false;
            blendDesc.IndependentBlendEnable = false;
            blendDesc.RenderTarget[0].BlendEnable = false;
            blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
            blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
            blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
            blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
            blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
            blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
            blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

            HRESULT hr = renderer.device->CreateBlendState(&blendDesc, &renderer.blendStates.opaqueBlend);
            if (FAILED(hr)) {
                ATTOFATAL("DX11: Unable to create blend state");
                return false;
            }

            blendDesc.RenderTarget[0].BlendEnable = true;
            blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
            blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
            blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
            blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
            blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
            blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
            blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

            hr = renderer.device->CreateBlendState(&blendDesc, &renderer.blendStates.alphaBlend);
            if (FAILED(hr)) {
                ATTOFATAL("DX11: Unable to create blend state");
                return false;
            }
        }

        const char* testingShaderSource = R"(
            cbuffer InstanceData : register(b0) {
                matrix mvp;
                matrix model;
            }

            cbuffer CameraData : register(b1) {
                matrix persp;
                matrix view;
                matrix screenProjection;
            };

            struct VS_INPUT {
                float3 position : POSITION;
                float3 normal : NORMAL;
                float2 uv : UV;
            };

            struct VS_OUTPUT {
                float4 position : SV_POSITION;
                float3 worldPos : WORLD_POS;
                float3 worldNrm : WORLD_NORMAL;
                float2 uv : UV;
            };

            VS_OUTPUT VSMain(VS_INPUT input) {
                VS_OUTPUT output;
                output.position = mul(mvp, float4(input.position, 1));
                //output.position = float4(input.position, 1);
                output.worldPos =  mul(model, float4(input.position, 1)).xyz;
                output.worldNrm =  mul(model, float4(input.normal, 0)).xyz;
                output.uv = input.uv;
                return output;
            }

            SamplerState pointWrap : register(s0);
            SamplerState pointClamp : register(s1);
            SamplerState linearWrap : register(s2);
            SamplerState linearClamp : register(s3);
            SamplerState anisotropicWrap : register(s4);
            SamplerState anisotropicClamp : register(s5);

            Texture2D diffuseTexture : register(t0);
            Texture2D normalTexture : register(t1);
            Texture2D specularTexture : register(t2);

            float4 TriplanarSampling(float3 worldPos, float3 worldNorm, float fallOff) {
                float2 triX = float2(-worldPos.z, worldPos.y);
                float2 triY = float2(worldPos.x, -worldPos.z);
                float2 triZ = float2(worldPos.x, worldPos.y);

                triX.x *= worldNorm.x < 0.0 ? -1.0 : 1.0;
                triY.x *= worldNorm.y < 0.0 ? -1.0 : 1.0;
                triZ.x *= worldNorm.z < 0.0 ? -1.0 : 1.0;

                float3 x = diffuseTexture.Sample(linearWrap, triX).rgb;
                float3 y = diffuseTexture.Sample(linearWrap, triY).rgb;
                float3 z = diffuseTexture.Sample(linearWrap, triZ).rgb;
                
                float3 triW = abs(worldNorm);
                triW /= triW.x + triW.y + triW.z;

                float3 sample = x * triW.x + y * triW.y + z * triW.z;
                return float4(sample, 1);
                //return float4(1,1,1,1);
            }

            float4 PSMain(VS_OUTPUT input) : SV_TARGET {
                //float3 nrm = normalize(input.worldNrm);
                //return TriplanarSampling(input.worldPos, nrm, 1);
                //return float4(nrm * 2 - float3(1,1,1), 1);
                float4 diffuse = diffuseTexture.Sample(linearWrap, input.uv);
                return diffuse;
                //return float4(input.uv, 0, 1);
            }
        )";

        ShaderCompile(testingShaderSource, INPUT_LAYOUT_POSITION_NORMAL_UV, renderer.testingShader);
        
        MeshCreateUnitQuad(renderer.unitQuad);
        
        ShaderBufferCreate(renderer.shaderBufferInstance);
        ShaderBufferCreate(renderer.shaderBufferCamera);

#if ATTO_DEBUG_RENDERING
        renderer.debugDrawState.maxVertexCount= 1000;
        // Create vertex buffer
        {
            D3D11_BUFFER_DESC bufferDesc = {};
            bufferDesc.ByteWidth = sizeof(DebugDrawVertex) * renderer.debugDrawState.maxVertexCount;
            bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
            bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            bufferDesc.MiscFlags = 0;
            bufferDesc.StructureByteStride = 0;

            HRESULT hr = renderer.device->CreateBuffer(&bufferDesc, nullptr, &renderer.debugDrawState.vertexBuffer);
            if (FAILED(hr)) {
                ATTOFATAL("DX11: Unable to create vertex buffer");
                return false;
            }
        }

        const char* debugDrawShaderSource = R"(
            cbuffer CameraData : register(b1) {
                matrix persp;
                matrix view;
                matrix screenProjection;
            };

            struct VS_INPUT {
                float4 position : POSITION;
            };

            struct VS_OUTPUT {
                float4 position : SV_POSITION;
                float4 color    : COLOR;
            };

            VS_OUTPUT VSMain(VS_INPUT input) {
                VS_OUTPUT output;
                float4 p = float4(input.position.xyz, 1);
                output.position = mul(persp, mul(view, p));
                output.color = float4(0, 1, 0, 1);
                return output;
            }

            float4 PSMain(VS_OUTPUT input) : SV_TARGET {
                return input.color;
            }
        )";

        ShaderCompile(debugDrawShaderSource, INPUT_LAYOUT_POSITION, renderer.debugDrawState.shader);

#endif

        ATTOTRACE("Initalized DX11");

        return true;
    }

    void LeEngine::ShaderGetInputLayout(ShaderInputLayout layout, FixedList<D3D11_INPUT_ELEMENT_DESC, 8>& list) {
        switch (layout)
        {
        case atto::INPUT_LAYOUT_POSITION:
        {
            D3D11_INPUT_ELEMENT_DESC pos = {};
            pos.SemanticName = "Position";
            pos.SemanticIndex = 0;
            pos.Format = DXGI_FORMAT_R32G32B32_FLOAT;
            pos.InputSlot = 0;
            pos.AlignedByteOffset = 0;
            pos.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
            pos.InstanceDataStepRate = 0;

            list.Add(pos);
        }break;

        case atto::INPUT_LAYOUT_BASIC_FONT:
        {
            D3D11_INPUT_ELEMENT_DESC pos = {};
            pos.SemanticName = "Position";
            pos.SemanticIndex = 0;
            pos.Format = DXGI_FORMAT_R32G32_FLOAT;
            pos.InputSlot = 0;
            pos.AlignedByteOffset = 0;
            pos.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
            pos.InstanceDataStepRate = 0;

            D3D11_INPUT_ELEMENT_DESC txc = {};
            txc.SemanticName = "UV";
            txc.SemanticIndex = 0;
            txc.Format = DXGI_FORMAT_R32G32_FLOAT;
            txc.InputSlot = 0;
            txc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
            txc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
            txc.InstanceDataStepRate = 0;

            list.Add(pos);
            list.Add(txc);
        }break;

        case atto::INPUT_LAYOUT_POSITION_NORMAL_UV:
        {
            D3D11_INPUT_ELEMENT_DESC pos = {};
            pos.SemanticName = "Position";
            pos.SemanticIndex = 0;
            pos.Format = DXGI_FORMAT_R32G32B32_FLOAT;
            pos.InputSlot = 0;
            pos.AlignedByteOffset = 0;
            pos.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
            pos.InstanceDataStepRate = 0;

            D3D11_INPUT_ELEMENT_DESC nrm = {};
            nrm.SemanticName = "Normal";
            nrm.SemanticIndex = 0;
            nrm.Format = DXGI_FORMAT_R32G32B32_FLOAT;
            nrm.InputSlot = 0;
            nrm.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
            nrm.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
            nrm.InstanceDataStepRate = 0;

            D3D11_INPUT_ELEMENT_DESC txc = {};
            txc.SemanticName = "UV";
            txc.SemanticIndex = 0;
            txc.Format = DXGI_FORMAT_R32G32_FLOAT;
            txc.InputSlot = 0;
            txc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
            txc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
            txc.InstanceDataStepRate = 0;

            list.Add(pos);
            list.Add(nrm);
            list.Add(txc);
        } break;
        default:
            Assert(0, "");
            break;
        }

    }

    static inline std::wstring AnsiToWString(const std::string& str) {
        WCHAR buffer[512];
        MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
        return std::wstring(buffer);
    }

    ID3DBlob* LeEngine::ShaderCompileFile(const char* path, const char* entry, const char* target) {
        ID3DBlob* shader = nullptr;
        ID3DBlob* errorBuff = nullptr;
        u32 flags = 0;

#if ATTO_DEBUG_RENDERING
        flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

        HRESULT hr = D3DCompileFromFile(
            AnsiToWString(path).c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entry,
            target,
            flags,
            0,
            &shader,
            &errorBuff
        );

        if (FAILED(hr)) {
            OutputDebugStringA((char*)errorBuff->GetBufferPointer());
            ATTOERROR((char*)errorBuff->GetBufferPointer());
            errorBuff->Release();
            return nullptr;
        }

        return shader;
    }

    ID3DBlob* LeEngine::ShaderCompileSource(const char* source, const char* entry, const char* target) {
        ID3DBlob* shader = nullptr;
        ID3DBlob* errorBuff = nullptr;
        u32 flags = 0;

#if ATTO_DEBUG_RENDERING
        flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

        HRESULT hr = D3DCompile(
            source, strlen(source),
            nullptr, nullptr, nullptr,
            entry, target, flags,
            0, &shader, &errorBuff
        );


        if (FAILED(hr)) {
            OutputDebugStringA((char*)errorBuff->GetBufferPointer());
            ATTOERROR((char*)errorBuff->GetBufferPointer());
            errorBuff->Release();
            return nullptr;
        }

        return shader;
    }

    bool LeEngine::ShaderCompile(ShaderAsset& shader) {
        ID3DBlob* vertexSource = ShaderCompileFile(shader.path.GetCStr(), "VSMain", "vs_5_0");
        ID3DBlob* pixelSource = ShaderCompileFile(shader.path.GetCStr(), "PSMain", "ps_5_0");

        FixedList<D3D11_INPUT_ELEMENT_DESC, 8> inputs = {};
        ShaderGetInputLayout(shader.layout, inputs);

        if (vertexSource != nullptr) {
            if (FAILED(renderer.device->CreateInputLayout(
                inputs.GetData(),
                inputs.GetCount(),
                vertexSource->GetBufferPointer(),
                vertexSource->GetBufferSize(),
                &shader.inputLayout)))
            {
                ATTOERROR("Could not create input layout");
                return false;
            }

            if (FAILED(renderer.device->CreateVertexShader(
                vertexSource->GetBufferPointer(),
                vertexSource->GetBufferSize(),
                nullptr,
                &shader.vertexShader)))
            {
                ATTOERROR("Could not create vertex shader");
                return false;
            }

            vertexSource->Release();
        }

        if (pixelSource != nullptr) {
            if (FAILED(renderer.device->CreatePixelShader(
                pixelSource->GetBufferPointer(),
                pixelSource->GetBufferSize(),
                nullptr,
                &shader.pixelShader)))
            {
                ATTOERROR("Could not create pixel shader");
                return false;
            }

            pixelSource->Release();
        }

        return false;
    }

    bool LeEngine::ShaderCompile(const char* source, ShaderInputLayout layout, ShaderAsset& shader) {
        ID3DBlob* vertexSource = ShaderCompileSource(source, "VSMain", "vs_5_0");
        ID3DBlob* pixelSource = ShaderCompileSource(source, "PSMain", "ps_5_0");

        FixedList<D3D11_INPUT_ELEMENT_DESC, 8> inputs = {};
        ShaderGetInputLayout(layout, inputs);
        shader.layout = layout;

        if (vertexSource != nullptr) {
            if (FAILED(renderer.device->CreateInputLayout(
                inputs.GetData(),
                inputs.GetCount(),
                vertexSource->GetBufferPointer(),
                vertexSource->GetBufferSize(),
                &shader.inputLayout)))
            {
                ATTOERROR("Could not create input layout");
                return false;
            }

            if (FAILED(renderer.device->CreateVertexShader(
                vertexSource->GetBufferPointer(),
                vertexSource->GetBufferSize(),
                nullptr,
                &shader.vertexShader)))
            {
                ATTOERROR("Could not create vertex shader");
                return false;
            }

            vertexSource->Release();
        }

        if (pixelSource != nullptr) {
            if (FAILED(renderer.device->CreatePixelShader(
                pixelSource->GetBufferPointer(),
                pixelSource->GetBufferSize(),
                nullptr,
                &shader.pixelShader)))
            {
                ATTOERROR("Could not create pixel shader");
                return false;
            }

            pixelSource->Release();
        }

        return true;
    }

    void LeEngine::MeshCreateUnitQuad(MeshAsset& quad) {
        f32 vertices[] = {
            // pos                  // nrm              // tex
           -1.0f, 1.0f, 0.0f,   0.0f, 0.0f, 1.0f,       0.0f, 1.0f,
            1.0f, -1.0f, 0.0f,   0.0f, 0.0f, 1.0f,      1.0f, 0.0f,
            -1.0f, -1.0f, 0.0f,   0.0f, 0.0f, 1.0f,     0.0f, 0.0f,

            -1.0f, 1.0f, 0.0f,   0.0f, 0.0f, 1.0f,      0.0f, 1.0f,
            1.0f, 1.0f, 0.0f,   0.0f, 0.0f, 1.0f,       1.0f, 1.0f,
            1.0f, -1.0f, 0.0f,   0.0f, 0.0f, 1.0f,      1.0f, 0.0f
        };

        quad.vertexCount = 6;
        quad.vertexStride = (3 + 3 + 2) * sizeof(f32);
        quad.indexCount = 0;

        D3D11_BUFFER_DESC vertexDesc = {};
        vertexDesc.Usage = D3D11_USAGE_IMMUTABLE;
        vertexDesc.ByteWidth = sizeof(vertices);
        vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vertexDesc.CPUAccessFlags = 0;
        vertexDesc.MiscFlags = 0;

        D3D11_SUBRESOURCE_DATA vertexData = {};
        vertexData.pSysMem = vertices;

        if (FAILED(renderer.device->CreateBuffer(&vertexDesc, &vertexData, &quad.vertexBuffer))) {
            ATTOERROR("Could not create vertex buffer");
        }
    }

    static void ProcessMesh(aiMesh* mesh, const aiScene* scene, MeshData& resultingMesh) {
        resultingMesh.positions.Resize(mesh->mNumVertices);
        resultingMesh.normals.Resize(mesh->mNumVertices);
        resultingMesh.uvs.Resize(mesh->mNumVertices);
        resultingMesh.indices.Resize(mesh->mNumFaces * 3);

        for (u32 vertexIndex = 0; vertexIndex < mesh->mNumVertices; vertexIndex++) {
            glm::vec3 position = {};
            position.x = mesh->mVertices[vertexIndex].x;
            position.y = mesh->mVertices[vertexIndex].y;
            position.z = mesh->mVertices[vertexIndex].z;

            position = position;

            glm::vec3 normal = {};
            normal.x = mesh->mNormals[vertexIndex].x;
            normal.y = mesh->mNormals[vertexIndex].y;
            normal.z = mesh->mNormals[vertexIndex].z;

            glm::vec3 tex = {};
            tex.x = mesh->mTextureCoords[0][vertexIndex].x;
            tex.y = mesh->mTextureCoords[0][vertexIndex].y;

            resultingMesh.positions.Add(position);
            resultingMesh.normals.Add(normal);
            resultingMesh.uvs.Add(tex);
        }

        for (u32 faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++) {
            aiFace face = mesh->mFaces[faceIndex];
            for (u32 index = 0; index < face.mNumIndices; index++) {
                Assert(face.mNumIndices == 3, "Not triangluated");
                Assert(face.mIndices[index] < UINT16_MAX, "Too many vertices, index too large");
                resultingMesh.indices.Add((u16)face.mIndices[index]);
            }
        }
    }

    static void ProcessNode(aiNode* node, const aiScene* scene, List<MeshData>& meshes) {
        for (u32 i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

            MeshData &meshData = meshes.Alloc();
            meshData.name = mesh->mName.C_Str();
            
            ProcessMesh(mesh, scene, meshData);
            break;
        }

        for (u32 i = 0; i < node->mNumChildren; i++) {
            ProcessNode(node->mChildren[i], scene, meshes);
        }
    }

    void LeEngine::MeshDataPackPNT(const MeshData& meshData, const glm::mat3& scalingMatrix, List<f32>& data) {
        Assert(meshData.positions.GetNum() == meshData.normals.GetNum(), "Positions and normals must be the same size");
        Assert(meshData.positions.GetNum() == meshData.uvs.GetNum(), "Positions and uvs must be the same size");

        const i32 vertexCount = meshData.positions.GetNum();
        for (i32 vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++) {
            glm::vec3 position = scalingMatrix * meshData.positions[vertexIndex];
            
            data.Add(position.x);
            data.Add(position.y);
            data.Add(position.z);

            data.Add(meshData.normals[vertexIndex].x);
            data.Add(meshData.normals[vertexIndex].y);
            data.Add(meshData.normals[vertexIndex].z);

            data.Add(meshData.uvs[vertexIndex].x);
            data.Add(meshData.uvs[vertexIndex].y);
        }
    }

    void LeEngine::MeshCreate(MeshAsset& mesh) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(mesh.path.GetCStr(), aiProcess_Triangulate | aiProcess_JoinIdenticalVertices );
        
        if (scene == nullptr || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            ATTOFATAL("ERROR::ASSIMP::%s", importer.GetErrorString());
            return;
        }

        List<MeshData> meshes;
        ProcessNode(scene->mRootNode, scene, meshes);
        
        const i32 meshCount = meshes.GetNum();
        Assert(meshCount == 1, "Only one mesh supported right now");

        glm::mat3 scalingMatrix = glm::mat3(1);
        if (mesh.path.EndsWith("fbx")) {
            scalingMatrix = glm::scale(glm::mat4(1), glm::vec3(0.01f));
        }

        MeshData& meshData = meshes[0];
        List<f32> vertices;
        MeshDataPackPNT(meshData, scalingMatrix, vertices);

        // Create vertex buffer
        D3D11_BUFFER_DESC vertexDesc = {};
        vertexDesc.Usage = D3D11_USAGE_IMMUTABLE;
        vertexDesc.ByteWidth = vertices.GetNum() * sizeof(f32);
        vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vertexDesc.CPUAccessFlags = 0;
        vertexDesc.MiscFlags = 0;
        
        D3D11_SUBRESOURCE_DATA vertexData = {};
        vertexData.pSysMem = vertices.GetData();
        
        if (FAILED(renderer.device->CreateBuffer(&vertexDesc, &vertexData, &mesh.vertexBuffer))) {
            ATTOERROR("Could not create vertex buffer");
            return;
        }

        // Create index buffer
        D3D11_BUFFER_DESC indexDesc = {};
        indexDesc.Usage = D3D11_USAGE_IMMUTABLE;
        indexDesc.ByteWidth = meshData.indices.GetNum() * sizeof(u16);
        indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        indexDesc.CPUAccessFlags = 0;
        indexDesc.MiscFlags = 0;

        D3D11_SUBRESOURCE_DATA indexData = {};
        indexData.pSysMem = meshData.indices.GetData();
        
        if (FAILED(renderer.device->CreateBuffer(&indexDesc, &indexData, &mesh.indexBuffer))) {
            ATTOERROR("Could not create index buffer");
            return;
        }

        mesh.isLoaded = true;
        mesh.indexCount = meshData.indices.GetNum();
        mesh.vertexCount = meshData.positions.GetNum();
        mesh.vertexStride = sizeof(f32) * (3 + 3 + 2);

        ATTOTRACE("Loaded mesh: %s", mesh.path.GetCStr());
    }

    void LeEngine::TextureCreate(TextureAsset& texture) {
        stbi_set_flip_vertically_on_load(true);
        void* pixelData = stbi_load(texture.path.GetCStr(), &texture.width, &texture.height, &texture.channels, 4);
        if (pixelData == nullptr) {
            ATTOERROR("Could not load texture: %s", texture.path.GetCStr());
            return;
        }
        
        D3D11_TEXTURE2D_DESC textureDesc = {};
        textureDesc.Width = texture.width;
        textureDesc.Height = texture.height;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
        textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        textureDesc.CPUAccessFlags = 0;
        textureDesc.MiscFlags = 0;

        D3D11_SUBRESOURCE_DATA textureData = {};
        textureData.pSysMem = pixelData;
        textureData.SysMemPitch = texture.width * 4;
        textureData.SysMemSlicePitch = 0;
        
        if (FAILED(renderer.device->CreateTexture2D(&textureDesc, &textureData, &texture.texture))) {
            ATTOERROR("Could not create texture: %s", texture.path.GetCStr());
            return;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
        shaderResourceViewDesc.Format = textureDesc.Format;
        shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
        shaderResourceViewDesc.Texture2D.MipLevels = 1;
        
        if (FAILED(renderer.device->CreateShaderResourceView(texture.texture.Get(), &shaderResourceViewDesc, &texture.srv))) {
            ATTOERROR("Could not create shader resource view: %s", texture.path.GetCStr());
            return;
        }

        texture.isLoaded = true;

        stbi_image_free(pixelData);
        
        ATTOTRACE("Loaded texture: %s", texture.path.GetCStr());
    }


}

