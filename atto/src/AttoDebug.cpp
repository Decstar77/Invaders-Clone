#include "AttoAsset.h"

#if ATTO_DEBUG_RENDERING

namespace atto
{
    bool LeEngine::InitializeDebug() {
        {
            D3D11_BUFFER_DESC bufferDesc = {};
            bufferDesc.ByteWidth = sizeof(DebugDrawVertex) * renderer.debugDrawState.lineVertices.GetCapcity();
            bufferDesc.Usage = D3D11_USAGE_DEFAULT;
            bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            bufferDesc.CPUAccessFlags = 0;
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

        ShaderCompile(debugDrawShaderSource, INPUT_LAYOUT_DEBUG_LINE, renderer.debugDrawState.shader);
        
        return true;
    }

    void LeEngine::DebugRender() {
        DebugDrawState& state = renderer.debugDrawState;
        if (state.lineVertices.GetCount() == 0) {
            return;
        }

        renderer.context->UpdateSubresource(state.vertexBuffer.Get(), 0, nullptr, state.lineVertices.GetData(), 0, 0);

        const u32 offset = 0;
        const u32 stride = sizeof(DebugDrawVertex);
        const u32 count = state.lineVertices.GetCount();
        renderer.context->IASetInputLayout(state.shader.inputLayout.Get());
        renderer.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        renderer.context->IASetVertexBuffers(0, 1, state.vertexBuffer.GetAddressOf(), &stride, &offset);
        renderer.context->OMSetDepthStencilState(renderer.depthStates.depthEnabled.Get(), 0);
        renderer.context->VSSetShader(state.shader.vertexShader.Get(), nullptr, 0);
        renderer.context->PSSetShader(state.shader.pixelShader.Get(), nullptr, 0);
        renderer.context->Draw(count, 0);
        
        state.lineVertices.Clear();
    }

    void LeEngine::DebugAddLine(glm::vec3 a, glm::vec3 b) {
        DebugDrawVertex v1 = {};
        v1.positionColorIndex = glm::vec4(a, 1);
        DebugDrawVertex v2 = {};
        v2.positionColorIndex = glm::vec4(b, 1);
        renderer.debugDrawState.lineVertices.Add(v1);
        renderer.debugDrawState.lineVertices.Add(v2);
    }
    
    void LeEngine::DebugAddPoint(glm::vec3 p) {
        DebugAddLine(p - glm::vec3(0.1f, 0.0f, 0.0f), p + glm::vec3(0.1f, 0.0f, 0.0f));
        DebugAddLine(p - glm::vec3(0.0f, 0.1f, 0.0f), p + glm::vec3(0.0f, 0.1f, 0.0f));
        DebugAddLine(p - glm::vec3(0.0f, 0.0f, 0.1f), p + glm::vec3(0.0f, 0.0f, 0.1f));
    }

    void LeEngine::DebugAddCircle(glm::vec3 p, glm::vec3 n, f32 radius) {
        f32 angle = 0.0f;
        constexpr f32 steps = 24.0f;
        constexpr f32 angleStep = 2.0f * glm::pi<f32>() / steps;
        for (i32 i = 0; i < (i32)steps; i++) {
            glm::vec2 a = glm::vec2(cos(angle), sin(angle)) * radius;
            glm::vec2 b = glm::vec2(cos(angle + angleStep), sin(angle + angleStep)) * radius;

            DebugAddLine(p + glm::vec3(a.x, 0.0f, a.y), p + glm::vec3(b.x, 0.0f, b.y));

            angle += angleStep;
        }
    }
    
    void LeEngine::DebugAddRay(Ray ray) {
        DebugAddLine(ray.origin, ray.origin + ray.direction * 1000.0f);
    }
}

#endif

