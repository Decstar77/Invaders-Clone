#include "AttoAsset.h"

#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis/stb_vorbis.c"

#include <audio/AudioFile.h>
#include <json/json.hpp>

#include <filesystem>
#include <random>

namespace atto {
    static void FindAllFiles(const char* path, const char* extension, List<LargeString>& files) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
            if (entry.path().extension() == extension) {
                files.Add(LargeString::FromLiteral(entry.path().string().c_str()));
            }
        }
    }

    void BoxBounds::Translate(const glm::vec2& translation) {
        min += translation;
        max += translation;
    }

    void BoxBounds::CreateFromCenterSize(const glm::vec2& center, const glm::vec2& size) {
        max = center + size * 0.5f;
        min = center - size * 0.5f;
    }

    f32 LeEngine::Random() {
        return Random(0.0f, 1.0f);
    }

    f32 LeEngine::Random(f32 min, f32 max) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<f32> dis(min, max);
        return dis(gen);
    }

    i32 LeEngine::RandomInt(i32 min, i32 max) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<i32> dis(min, max);
        return dis(gen);
    }

    glm::vec2 LeEngine::ScreenPosToNDC(glm::vec2 pos) {
        f32 x = (pos.x / (f32)renderer.swapChainWidth) * 2.0f - 1.0f;
        f32 y = -((pos.y / (f32)renderer.swapChainHeight) * 2.0f - 1.0f);

        return glm::vec2(x, y);
    }

    glm::vec2 LeEngine::ScreenToWorld(glm::vec2 inPos) {
        glm::mat4 cameraView = currentCamera->GetViewMatrix();

        glm::mat4 ip = glm::inverse(cameraProjection);
        glm::mat4 iv = glm::inverse(cameraView);

        glm::vec4 mousePosClipSpace = glm::vec4(inPos.x, inPos.y, 0, 1);
        glm::vec4 mousePosEyeSpace = ip * mousePosClipSpace;
        glm::vec4 mousePosWorldSpace = iv * mousePosEyeSpace;

        glm::vec2 pos = glm::vec2(mousePosWorldSpace.x, mousePosWorldSpace.y);

        return pos;
    }

    glm::vec2 LeEngine::GetMousePosWorldSpace() {
        glm::vec2 mousePos = ScreenPosToNDC(app->input->mousePosPixels);
        return ScreenToWorld(mousePos);
    }

    void LeEngine::CameraSet(Camera& camera) {
        currentCamera = &camera;
        cameraProjection = glm::perspectiveFovRH_ZO(camera.fov, (f32)renderer.swapChainWidth, (f32)renderer.swapChainHeight, camera.nearPlane, camera.farPlane);
    }

    void LeEngine::CameraDoFreeFlyKeys(Camera &camera) {
        f32 speed = 1.0f;
        if (app->input->keys[KEY_CODE_W]) {
            camera.pos += camera.ori.forward * speed * app->deltaTime;
        }
        if (app->input->keys[KEY_CODE_S]) {
            camera.pos -= camera.ori.forward * speed * app->deltaTime;
        }
        if (app->input->keys[KEY_CODE_A]) {
            camera.pos -= camera.ori.right * speed * app->deltaTime;
        }
        if (app->input->keys[KEY_CODE_D]) {
            camera.pos += camera.ori.right * speed * app->deltaTime;
        }
        if (app->input->keys[KEY_CODE_E]) {
            camera.pos += camera.ori.up * speed * app->deltaTime;
        }
        if (app->input->keys[KEY_CODE_Q]) {
            camera.pos -= camera.ori.up * speed * app->deltaTime;
        }
    }

    void LeEngine::CameraDoFreeFlyMouse(Camera& camera, f32 x, f32 y) {
        f32 sensitivity = 0.1f;
        camera.yaw += x * sensitivity;
        camera.pitch += y * sensitivity;

        if (camera.pitch > 89.0f) {
            camera.pitch = 89.0f;
        }
        if (camera.pitch < -89.0f) {
            camera.pitch = -89.0f;
        }

        camera.ori.forward.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
        camera.ori.forward.y = sin(glm::radians(camera.pitch));
        camera.ori.forward.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
        camera.ori.forward = glm::normalize(camera.ori.forward);

        camera.ori.right = glm::normalize(glm::cross(camera.ori.forward, glm::vec3(0, 1, 0)));
        camera.ori.up = glm::normalize(glm::cross(camera.ori.right, camera.ori.forward));
    }

    bool LeEngine::Initialize(AppState* appState) {
        app = appState;

        RegisterAssets();
        InitializeRenderer();
        InitializeAudio();
        InitializeFonts();

        FontCreate(fontAssets[0]);

        editorCamera = Camera::CreateDefault();
        gameCamera = Camera::CreateIsometric();
        CameraSet(editorCamera);
        screenProjection = glm::orthoLH_ZO(0.0f, (f32)renderer.swapChainWidth, (f32)renderer.swapChainHeight, 0.0f, 0.0f, 1.0f);

        buildingBlock1x1_01 = LoadMeshAsset(MeshAssetId::Create("assets/prototype/SM_Buildings_Block_1x1_01"));
        buildingBlock1x1_02 = LoadMeshAsset(MeshAssetId::Create("assets/prototype/SM_Buildings_Block_1x1_02"));
        buildingBlock1x1_03 = LoadMeshAsset(MeshAssetId::Create("assets/prototype/SM_Buildings_Block_1x1_03"));
        primitiveCapsule = LoadMeshAsset(MeshAssetId::Create("assets/primitives/capsule"));
        textureGrid_01 = LoadTextureAsset(TextureAssetId::Create("assets/prototype/Texture_Grid_01"));
        textureTriplanarTest = LoadTextureAsset(TextureAssetId::Create("assets/prototype/Texture_Triplanar_Test"));

        return true;
    }

    void LeEngine::Update(AppState* app) {
        CameraDoFreeFlyKeys(editorCamera);
        if (IsKeyJustDown(app->input, KEY_CODE_Y)) {
            ATTOINFO("UYes");
        }
    }

    void LeEngine::Render(AppState* app) {
        D3D11_VIEWPORT viewport = {};
        viewport.TopLeftX = 0;
        viewport.TopLeftY = 0;
        viewport.Width = static_cast<float>(renderer.swapChainWidth);
        viewport.Height = static_cast<float>(renderer.swapChainHeight);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;

        constexpr float clearColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };

        renderer.context->ClearRenderTargetView(renderer.swapChainRenderTarget.Get(), clearColor);
        renderer.context->ClearDepthStencilView(renderer.swapChainDepthView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
        renderer.context->RSSetViewports(1, &viewport);
        renderer.context->RSSetState(renderer.rasterizerStates.cullNone.Get());
        renderer.context->OMSetDepthStencilState(renderer.depthStates.depthEnabled.Get(), 0);
        renderer.context->OMSetRenderTargets( 1, renderer.swapChainRenderTarget.GetAddressOf(), renderer.swapChainDepthView.Get());

        renderer.context->IASetInputLayout(renderer.testingShader.inputLayout.Get());
        renderer.context->VSSetShader(renderer.testingShader.vertexShader.Get(), nullptr, 0);
        renderer.context->PSSetShader(renderer.testingShader.pixelShader.Get(), nullptr, 0);

        if (currentCamera != nullptr) {
            renderer.shaderBufferCamera.data.projection = cameraProjection;
            renderer.shaderBufferCamera.data.view = currentCamera->GetViewMatrix();
            renderer.shaderBufferCamera.data.screenProjection = screenProjection;

            ShaderBufferBind(renderer.shaderBufferCamera, 1, true, false);
            ShaderBufferUpload(renderer.shaderBufferCamera);

            static f32 x = 0;
            if (app->input->keys[KEY_CODE_LEFT]) {
                x -= 10.1f * app->deltaTime;
            }
            if (app->input->keys[KEY_CODE_RIGHT]) {
                x += 10.1f * app->deltaTime;
            }

            renderer.shaderBufferInstance.data.model = glm::rotate(glm::mat4(1), glm::radians(x), glm::vec3(1, 0, 0));
            renderer.shaderBufferInstance.data.mvp = renderer.shaderBufferCamera.data.projection * renderer.shaderBufferCamera.data.view * renderer.shaderBufferInstance.data.model;

            ShaderBufferBind(renderer.shaderBufferInstance, 0, true, false);
            ShaderBufferUpload(renderer.shaderBufferInstance);

            MeshBind(&renderer.unitQuad);
            MeshDraw(&renderer.unitQuad);

            //TextureBind(textureGrid_01, 0);
            //MeshBind(buildingBlock1x1_01);
            //MeshDraw(buildingBlock1x1_01);

            //MeshBind(primitiveCapsule);
            //MeshDraw(primitiveCapsule);

            FontRenderText("The quick brown fox jumps over the lazy cat.", FontAssetId::Create("assets/fonts/Roboto_Regular"), glm::vec2(100, 100));
        }
        //FontRenderText("Yallow", someFont, 0, 0, 0.5f, glm::vec4(1, 1, 0, 1));

        renderer.swapChain->Present(1, 0);
    }

    void LeEngine::Shutdown() {

    }

    void LeEngine::CallbackResize(i32 width, i32 height) {
        renderer.swapChainWidth = width;
        renderer.swapChainHeight = height;
        
        renderer.context->Flush();
        renderer.swapChainRenderTarget.Reset();
        
        if (FAILED(renderer.swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM, 0))) {
            ATTOFATAL("Failed to resize swap chain buffers");
            return;
        }

        wrl::ComPtr<ID3D11Texture2D> backBuffer = nullptr;
        if (FAILED(renderer.swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)))) {
            ATTOFATAL("DX11: Unable to get back buffer");
        }

        if (FAILED(renderer.device->CreateRenderTargetView(backBuffer.Get(), nullptr, &renderer.swapChainRenderTarget))) {
            ATTOFATAL("DX11: Unable to create render target view");
        }

        renderer.swapChainDepthView->Release();
        renderer.swapChainDepthBuffer->Release();
        
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
        }

        D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDescriptor = {};
        depthStencilViewDescriptor.Format = DXGI_FORMAT::DXGI_FORMAT_D16_UNORM;
        depthStencilViewDescriptor.ViewDimension = D3D11_DSV_DIMENSION::D3D11_DSV_DIMENSION_TEXTURE2D;
        depthStencilViewDescriptor.Texture2D.MipSlice = 0;

        if (FAILED(renderer.device->CreateDepthStencilView(renderer.swapChainDepthBuffer.Get(), &depthStencilViewDescriptor, &renderer.swapChainDepthView))) {
            ATTOFATAL("DX11: Unable to create depth stencil view");
        }
    }

    void LeEngine::CallbackMouseWheel(f32 x, f32 y) {

    }

    void LeEngine::CallbackMousePosition(f32 x, f32 y) {
        CameraDoFreeFlyMouse(editorCamera, x, y);
    }

    MeshAsset* LeEngine::LoadMeshAsset(MeshAssetId id) {
        MeshAsset* meshAsset = FindAsset(meshAssets, id.ToRawId());
        if (meshAsset == nullptr) {
            return nullptr;
        }

        if (meshAsset->isLoaded) {
            return meshAsset;
        }

        MeshCreate(*meshAsset);

        return meshAsset;
    }

    TextureAsset* LeEngine::LoadTextureAsset(TextureAssetId id) {
        TextureAsset* textureAsset = FindAsset(textureAssets, id.ToRawId());
        if (textureAsset == nullptr) {
            return nullptr;
        }

        if (textureAsset->isLoaded) {
            return textureAsset;
        }

        TextureCreate(*textureAsset);

        return textureAsset;
    }
    
    FontAsset* LeEngine::LoadFontAsset(FontAssetId id) {
        return nullptr;
    }

    AudioAsset* LeEngine::LoadAudioAsset(AudioAssetId id) {
        return nullptr;
    }

   
    void LeEngine::MeshBind(MeshAsset* mesh) {
        u32 offset = 0;
        renderer.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        renderer.context->IASetVertexBuffers(0, 1, mesh->vertexBuffer.GetAddressOf(), &mesh->vertexStride, &offset);
        if (mesh->indexBuffer != nullptr) {
            renderer.context->IASetIndexBuffer(mesh->indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
        }
    }

    void LeEngine::MeshDraw(MeshAsset* mesh) {
        if (mesh->indexBuffer != nullptr) {
            renderer.context->DrawIndexed(mesh->indexCount, 0, 0);
        }
        else {
            renderer.context->Draw(mesh->vertexCount, 0);
        }
    }

    void LeEngine::TextureBind(TextureAsset* texture, i32 slot) {
        if (texture->isLoaded == false) {
            ATTOWARN("Binding texture that is not loaded");
            return;
        }

        if (texture->srv == nullptr) {
            ATTOWARN("Binding texture with no srv");
            return;
        }

        renderer.context->PSSetShaderResources(slot, 1, texture->srv.GetAddressOf());
    }

    void LeEngine::RegisterAssets() {
        List<LargeString> meshPaths;
        FindAllFiles(app->looseAssetPath.GetCStr(), ".obj", meshPaths);
        FindAllFiles(app->looseAssetPath.GetCStr(), ".fbx", meshPaths);

        List<LargeString> texturePaths;
        FindAllFiles(app->looseAssetPath.GetCStr(), ".png", texturePaths);
        FindAllFiles(app->looseAssetPath.GetCStr(), ".jpg", texturePaths);

        List<LargeString> audioPaths;
        FindAllFiles(app->looseAssetPath.GetCStr(), ".ogg", audioPaths);
        FindAllFiles(app->looseAssetPath.GetCStr(), ".wav", audioPaths);

        List<LargeString> fontPaths;
        FindAllFiles(app->looseAssetPath.GetCStr(), ".ttf", fontPaths);

        const i32 meshCount = meshPaths.GetNum();
        for (i32 meshPathIndex = 0; meshPathIndex < meshCount; ++meshPathIndex) {
            MeshAsset asset = MeshAsset::CreateDefault();

            LargeString path = meshPaths[meshPathIndex];
            path.BackSlashesToSlashes();
            asset.path = path;
            path.StripFileExtension();
            asset.id = AssetId::Create(path.GetCStr());

            meshAssets.Add(asset);

            ATTOTRACE("Found mesh asset: %s", path.GetCStr());
        }

        const i32 textureCount = texturePaths.GetNum();
        for (i32 texturePathIndex = 0; texturePathIndex < textureCount; ++texturePathIndex) {
            TextureAsset asset = TextureAsset::CreateDefault();

            LargeString path = texturePaths[texturePathIndex];
            path.BackSlashesToSlashes();
            asset.path = path;
            path.StripFileExtension();
            asset.id = AssetId::Create(path.GetCStr());

            textureAssets.Add(asset);

            ATTOTRACE("Found texture asset: %s", path.GetCStr());
        }

        const i32 audioCount = audioPaths.GetNum();
        for (i32 audioPathIndex = 0; audioPathIndex < audioCount; ++audioPathIndex) {
            AudioAsset asset = AudioAsset::CreateDefault();;

            LargeString path = audioPaths[audioPathIndex];
            path.BackSlashesToSlashes();
            asset.path = path;
            path.StripFileExtension();
            asset.id = AssetId::Create(path.GetCStr());

            audioAssets.Add(asset);

            ATTOTRACE("Found audio asset: %s", path.GetCStr());
        }

        const i32 fontCount = fontPaths.GetNum();
        for (i32 fontPathIndex = 0; fontPathIndex < fontCount; ++fontPathIndex) {
            FontAsset asset = FontAsset::CreateDefault();
            
            LargeString path = fontPaths[fontPathIndex];
            path.BackSlashesToSlashes();
            asset.path = path;
            path.StripFileExtension();
            asset.id = AssetId::Create(path.GetCStr());

            fontAssets.Add(asset);

            ATTOTRACE("Found font asset: %s", path.GetCStr());
        }

        //// TODO: Hard coded string !!
        //std::ifstream spritesRegisterFile("assets/sprites.json");
        //if (spritesRegisterFile.is_open()) {
        //    nlohmann::json j = nlohmann::json::parse(spritesRegisterFile);
        //    
        //    for (nlohmann::json::iterator it = j.begin(); it != j.end(); ++it) {
        //        std::string spriteName = it.key();
        //        nlohmann::json spriteData = it.value();
        //        
        //        std::string texture = spriteData["texture"].get<std::string>();

        //        SpriteAsset sprite = SpriteAsset::CreateDefault();
        //        sprite.id = AssetId::Create(spriteName.c_str());
        //        sprite.origin = SPRITE_ORIGIN_CENTER;
        //        sprite.textureId = TextureAssetId::Create(texture.c_str());
        //        sprite.frameCount = spriteData["frameCount"].get<i32>();
        //        sprite.frameSize.x = spriteData["frameSize"]["x"].get<f32>();
        //        sprite.frameSize.y = spriteData["frameSize"]["y"].get<f32>();

        //        registeredSprites.Add(sprite);
        //    }

        //    spritesRegisterFile.close();
        //}

    }

    void PackedAssetFile::PutData(byte* data, i32 size) {
        const i32 s = storedData.GetNum();
        storedData.SetNum(s + size, true);
        std::memcpy(storedData.GetData() + s, data, size);
    }

    void PackedAssetFile::GetData(byte* data, i32 size) {
        Assert(currentOffset + size <= storedData.GetNum(), "PackedAssetFile::Get: currentOffset + size > storedData.GetNum()");
        byte* p = storedData.GetData() + currentOffset;
        std::memcpy(data, p, size);
        currentOffset += size;
    }

    void PackedAssetFile::SerializeAsset(TextureAsset& textureAsset) {
        Serialize(textureAsset.width);
        Serialize(textureAsset.height);
        Serialize(textureAsset.channels);
        Serialize(textureAsset.generateMipMaps);
        //Serialize(textureAsset.data);
    }

    void PackedAssetFile::SerializeAsset(AudioAsset& audioAsset) {
        Serialize(audioAsset.channels);
        Serialize(audioAsset.sampleRate);
        Serialize(audioAsset.sizeBytes);
        Serialize(audioAsset.bitDepth);
    }

    void PackedAssetFile::SerializeAsset(FontAsset& fontAsset) {
        Serialize(fontAsset.fontSize);
        //SerializeAsset(fontAsset.textureAsset);
    }

    void PackedAssetFile::Reset() {
        currentOffset = 0;
    }

    bool PackedAssetFile::Save(const char* name) {
        std::ofstream file(name, std::ios::binary);
        if (!file.is_open()) {
            ATTOERROR("PackedAssetFile::Save -> Could not open file %s", name)
                return false;
        }

        file.write((char*)storedData.GetData(), storedData.GetNum());
        file.close();

        return true;
    }

    bool PackedAssetFile::Load(const char* name) {
        std::ifstream file(name, std::ios::binary);
        if (!file.is_open()) {
            ATTOERROR("PackedAssetFile::Load -> Could not open file %s", name);
            return false;
        }

        file.seekg(0, std::ios::end);
        const i32 size = (i32)file.tellg();
        file.seekg(0, std::ios::beg);

        storedData.SetNum(size, true);
        file.read((char*)storedData.GetData(), size);
        file.close();

        isLoading = true;

        return true;
    }

    void PackedAssetFile::Finished() {
        storedData.Clear();
    }

}

