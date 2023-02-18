#include "AttoAsset.h"

namespace atto
{
    static const char* draw2DShader = R"(
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
                float2 position : POSITION;
                float2 uv       : UV;
            };

            struct VS_OUTPUT {
                float4 position     : SV_POSITION;
                float2 uv           : UV;
            };

            VS_OUTPUT VSMain(VS_INPUT input) {
                VS_OUTPUT output;
                output.position = mul(screenProjection, float4(input.position, 0, 1));
                output.uv = input.uv;
                return output;
            }

            cbuffer Draw2DData : register(b0) {
                float4 posdims;
                float4 innerColor;
                float4 outerColor;
                float4 params;
            }

            SamplerState pointWrap : register(s0);
            SamplerState pointClamp : register(s1);
            SamplerState linearWrap : register(s2);
            SamplerState linearClamp : register(s3);
            SamplerState anisotropicWrap : register(s4);
            SamplerState anisotropicClamp : register(s5);

            float CircleSDF(float2 r, float2 p, float rad) {
                return 1 - max(length(p - r) - rad, 0);
            }

            float BoxSDF(float2 r, float2 p, float2 s) {
                return 1 - length(max(abs(p - r) - s, 0));
            }
            
            float RoundedBoxSDF(float2 r, float2 p, float2 s, float rad) {
                return 1 - (length(max(abs(p - r) - s + rad, 0)) - rad);
            }

            float4 PSMain(VS_OUTPUT input) : SV_TARGET {
                int prim = int(params.x);
                float2 pos = input.position.xy;
                float2 dim = posdims.zw;

                if (prim == 0) {
                    return innerColor;
                } else if (prim == 1) {
                    float thic = params.y;
                    float2 center = posdims.xy + dim / 2;
                    float dist1 = BoxSDF( center, pos, dim / 2 );
                    float dist2 = BoxSDF( center, pos, dim / 2 - thic );
                    float alpha = saturate(dist1 - dist2);
                    return lerp(innerColor, outerColor, alpha);
                }

                //float a = diffuseTexture.Sample(linearWrap, input.uv).r;
                //return float4(a,a,a,a);
                
                return float4(input.uv, 0, 1);
            }
        )";

    struct Draw2DVertex {
        glm::vec2 position;
        glm::vec2 uv;
    };
    
    bool LeEngine::InitializeDraw2D() {
        bool compiled = ShaderCompile(draw2DShader, INPUT_LAYOUT_DRAW_2D, renderer.draw2DShader);
        if (!compiled) {
            ATTOERROR("Could not compile draw2d shader");
            return false;
        }

        D3D11_BUFFER_DESC bufferDesc = {};
        bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        bufferDesc.ByteWidth = sizeof(Draw2DVertex) * 6;
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        HRESULT hr = renderer.device->CreateBuffer(&bufferDesc, nullptr, &renderer.draw2DVertexBuffer);
        if (FAILED(hr)) {
            ATTOERROR("Could not create draw2d vertex buffer");
            return false;
        }

        return true;
    }

    void LeEngine::Draw2DPrimitive(const Draw2DParams& params) {
        const f32 x1 = params.pos.x;
        const f32 y1 = params.pos.y;
        const f32 x2 = params.pos.x + params.dims.x;
        const f32 y2 = params.pos.y + params.dims.y;

        const f32 vertices[] = {
            x1, y1, 0, 1,
            x2, y1, 1, 1,
            x2, y2, 1, 0,
            x1, y1, 0, 1,
            x2, y2, 1, 0,
            x1, y2, 0, 0
        };

        D3D11_MAPPED_SUBRESOURCE mappedResource = {};
        HRESULT hr = renderer.context->Map(renderer.draw2DVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        if (FAILED(hr)) {
            ATTOERROR("Failed to map draw2D vertex buffer");
            return;
        }

        std::memcpy(mappedResource.pData, vertices, sizeof(vertices));
        renderer.context->Unmap(renderer.draw2DVertexBuffer.Get(), 0);

        renderer.shaderBufferDraw2D.data.posdims = glm::vec4(params.pos, params.dims);
        renderer.shaderBufferDraw2D.data.params = glm::vec4((f32)params.primType, params.d, 0, 0);
        renderer.shaderBufferDraw2D.data.innerColor = params.innerColor;
        renderer.shaderBufferDraw2D.data.outerColor = params.outerColor;
        ShaderBufferBind(renderer.shaderBufferDraw2D, 0, false, true);
        ShaderBufferUpload(renderer.shaderBufferDraw2D);

        const u32 offset = 0;
        const u32 stride = sizeof(Draw2DVertex);
        renderer.context->RSSetState(renderer.rasterizerStates.cullNone.Get());
        renderer.context->OMSetBlendState(renderer.blendStates.alphaBlend.Get(), nullptr, 0xffffffff);
        renderer.context->OMSetDepthStencilState(renderer.depthStates.depthDisabled.Get(), 0);
        renderer.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        renderer.context->IASetVertexBuffers(0, 1, renderer.draw2DVertexBuffer.GetAddressOf(), &stride, &offset);
        renderer.context->IASetInputLayout(renderer.draw2DShader.inputLayout.Get());
        renderer.context->VSSetShader(renderer.draw2DShader.vertexShader.Get(), nullptr, 0);
        renderer.context->PSSetShader(renderer.draw2DShader.pixelShader.Get(), nullptr, 0);
        renderer.context->Draw(6, 0);
    }

    void LeEngine::Draw2DRectPosDims(glm::vec2 pos, glm::vec2 dims, glm::vec4 color) {
        Draw2DParams params = {};
        params.primType = Draw2DPrimitiveType_Rect;
        params.pos = pos;
        params.dims = dims;
        params.innerColor = color;
        params.outerColor = color;
        params.d = 0.0f;
        Draw2DPrimitive(params);
    }

    void LeEngine::Draw2DRectCenterDims(glm::vec2 center, glm::vec2 dims, glm::vec4 color) {
        Draw2DParams params = {};
        params.primType = Draw2DPrimitiveType_Rect;
        params.pos.x = center.x - dims.x / 2.0f;
        params.pos.y = center.y - dims.y / 2.0f;
        params.dims = dims;
        params.innerColor = color;
        params.outerColor = color;
        params.d = 0.0f;
        Draw2DPrimitive(params);
    }

    void LeEngine::Draw2DRectOutlinePosDims(glm::vec2 pos, glm::vec2 dims, f32 w, glm::vec4 outerColor, glm::vec4 innerColor) {
        Draw2DParams params = {};
        params.primType = Draw2DPrimitiveType_Rect_Outlined;
        params.pos = pos;
        params.dims = dims;
        params.innerColor = innerColor;
        params.outerColor = outerColor;
        params.d = w;
        Draw2DPrimitive(params);
    }
}