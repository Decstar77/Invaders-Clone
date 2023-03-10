#include "AttoAsset.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype/stb_truetype.h>

namespace atto
{
    static const char*  basicFontShader = R"(
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
                float4 position : SV_POSITION;
                float2 uv       : UV;
            };

            VS_OUTPUT VSMain(VS_INPUT input) {
                VS_OUTPUT output;
                output.position = mul(screenProjection, float4(input.position, 0, 1));
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

            float4 PSMain(VS_OUTPUT input) : SV_TARGET {
                float a = diffuseTexture.Sample(linearWrap, input.uv).r;
                return float4(a,a,a,a);
                //return float4(input.uv, 0, 1);
            }
        )";

    struct FontVertex {
        glm::vec2 position;
        glm::vec2 uv;
    };

    bool LeEngine::InitializeFonts() {
        bool compiled = ShaderCompile(basicFontShader, INPUT_LAYOUT_BASIC_FONT, renderer.fontShader);
        if (!compiled) {
            ATTOERROR("Could not compile font shader");
            return false;
        }

        D3D11_BUFFER_DESC bufferDesc = {};
        bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        bufferDesc.ByteWidth = sizeof(FontVertex) * 6;
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        HRESULT hr = renderer.device->CreateBuffer(&bufferDesc, nullptr, &renderer.fontVertexBuffer);
        if (FAILED(hr)) {
            ATTOERROR("Could not create font vertex buffer");
            return false;
        }

        return true;
    }

    void LeEngine::FontCreate(FontAsset& font) {
        i32 fileSize = 0;
        byte* tff = LoadEntireFile(font.path.GetCStr(), fileSize);
        //byte* tff = LoadEntireFile("C:/Windows/Fonts/Arial.ttf", fileSize);
        //byte* tff = LoadEntireFile("C:/Projects/Atto - G2/bin/assets/fonts/Roboto_Regular.ttf", fileSize);

        if (tff == nullptr) {
            return;
        }

        if (stbtt_InitFont(&font.info, tff, 0) == 0) {
            ATTOERROR("Could not init font");
        }
        
        stbtt_GetFontVMetrics(&font.info, &font.ascent, &font.descent, &font.lineGap);

        byte* pixels = new byte[256 * 256];
        stbtt_BakeFontBitmap(tff, 0, font.fontSize, pixels, 256, 256, 32, 96, font.chardata.GetData());

        D3D11_TEXTURE2D_DESC textureDesc = {};
        textureDesc.Width = 256;
        textureDesc.Height = 256;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_R8_UNORM;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        textureDesc.CPUAccessFlags = 0;
        textureDesc.MiscFlags = 0;
        
        D3D11_SUBRESOURCE_DATA subresourceData = {};
        subresourceData.pSysMem = pixels;
        subresourceData.SysMemPitch = 256;
        subresourceData.SysMemSlicePitch = 256 * 256;
        
        HRESULT hr = renderer.device->CreateTexture2D(&textureDesc, &subresourceData, &font.texture);
        if (FAILED(hr)) {
            ATTOERROR("Could not create texture");
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = textureDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        
        hr = renderer.device->CreateShaderResourceView(font.texture.Get(), &srvDesc, &font.srv);
        if (FAILED(hr)) {
            ATTOERROR("Could not create shader resource view");
        }

        font.isLoaded = true;

        delete[] pixels;
    }

    f32 LeEngine::FontWidth(FontAsset* font, const char* text) {
        const f32 scale = stbtt_ScaleForPixelHeight(&font->info, font->fontSize);
        const i32 textLength = (i32)strlen(text);

        u32 intWidth = 0;
        for (i32 charIndex = 0; charIndex < textLength; charIndex++) {
            i32 cp = (i32)text[charIndex];
            if (cp < 32 || cp >= 128) {
                continue;
            }

            i32 leftSideBearing = 0;
            i32 advance = 0;
            stbtt_GetCodepointHMetrics(&font->info, cp, &advance, &leftSideBearing);

            intWidth += advance;
            if (text[charIndex + 1]) {
                intWidth -= stbtt_GetCodepointKernAdvance(&font->info, cp, (i32)text[charIndex + 1]);
            }
        }

        return (f32)intWidth * scale;
    }
    
    void LeEngine::FontRenderText(const char* text, FontAsset* font, glm::vec2 pos, glm::vec4 color /*= glm::vec4(1, 1, 1, 1)*/) {
        f32 scale = stbtt_ScaleForPixelHeight(&font->info, font->fontSize);
        f32 xpos = pos.x;
        f32 ypos = pos.y;

        const i32 textLength = (i32)strlen(text);
        for (i32 charIndex = 0; charIndex < textLength; charIndex++) {
            i32 cp = (i32)text[charIndex];
            if (cp < 32 || cp >= 128) {
                continue;
            }

            stbtt_aligned_quad quad = {};
            stbtt_GetBakedQuad(font->chardata.GetData(), 256, 256, cp - 32, &xpos, &ypos, &quad, true);

            f32 vertices[6][4] = {
                { quad.x0, quad.y0, quad.s0, quad.t0 },
                { quad.x1, quad.y0, quad.s1, quad.t0 },
                { quad.x1, quad.y1, quad.s1, quad.t1 },

                { quad.x0, quad.y0, quad.s0, quad.t0 },
                { quad.x1, quad.y1, quad.s1, quad.t1 },
                { quad.x0, quad.y1, quad.s0, quad.t1 }
            };

            if (text[charIndex + 1]) {
                f32 kadv = (f32)stbtt_GetCodepointKernAdvance(&font->info, cp, (i32)text[charIndex + 1]);
                xpos += kadv * scale;
            }

            D3D11_MAPPED_SUBRESOURCE mappedResource = {};
            HRESULT hr = renderer.context->Map(renderer.fontVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
            if (FAILED(hr)) {
                ATTOERROR("Failed to map font vertex buffer");
                return;
            }

            std::memcpy(mappedResource.pData, vertices, sizeof(vertices));
            renderer.context->Unmap(renderer.fontVertexBuffer.Get(), 0);

            const u32 offset = 0;
            const u32 stride = sizeof(FontVertex);
            renderer.context->RSSetState(renderer.rasterizerStates.cullNone.Get());
            renderer.context->OMSetBlendState(renderer.blendStates.alphaBlend.Get(), nullptr, 0xffffffff);
            renderer.context->OMSetDepthStencilState(renderer.depthStates.depthDisabled.Get(), 0);
            renderer.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            renderer.context->IASetVertexBuffers(0, 1, renderer.fontVertexBuffer.GetAddressOf(), &stride, &offset);
            renderer.context->IASetInputLayout(renderer.fontShader.inputLayout.Get());
            renderer.context->VSSetShader(renderer.fontShader.vertexShader.Get(), nullptr, 0);
            renderer.context->PSSetShader(renderer.fontShader.pixelShader.Get(), nullptr, 0);
            renderer.context->PSSetShaderResources(0, 1, font->srv.GetAddressOf());
            renderer.context->Draw(6, 0);
        }
    }
    
    void LeEngine::FontRenderText(const char* text, FontAssetId fontId, glm::vec2 pos, glm::vec4 color) {
        FontAsset* font = FindAsset(fontAssets, fontId.ToRawId());
        if (font == nullptr) {
            ATTOERROR("Could not find font asset");
            return;
        }

        if (font->isLoaded == false) {
            FontCreate(*font);
        }
      
        FontRenderText(text, font, pos, color);
    }


}
