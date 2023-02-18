#pragma once

#include "AttoLib.h"
#include "AttoLua.h"

#include <wrl.h>
namespace wrl = Microsoft::WRL;
#include <d3d11_3.h>
#include <dxgi1_2.h>
#include <dxgidebug.h>
#include <d3dcompiler.h>

#include <xaudio2.h>
#include <stb_truetype/stb_truetype.h>

namespace glm
{
    struct basis {
        vec3 right;
        vec3 up;
        vec3 forward;
        
        inline static basis identity() {
            return basis{
                vec3(1, 0, 0),
                vec3(0, 1, 0),
                vec3(0, 0, 1)
            };
        }
    };

    inline basis toBasis(const glm::mat4& m) {
        basis result;
        result.right = glm::vec3(m[0][0], m[1][0], m[2][0]);
        result.up = glm::vec3(m[0][1], m[1][1], m[2][1]);
        result.forward = glm::vec3(m[0][2], m[1][2], m[2][2]);
        
        return result;
    }

    inline glm::mat4 toMat4(const basis& b) {
        glm::mat4 result;

        result[0][0] = b.right.x;
        result[1][0] = b.right.y;
        result[2][0] = b.right.z;
        result[3][0] = 0;

        result[0][1] = b.up.x;
        result[1][1] = b.up.y;
        result[2][1] = b.up.z;
        result[3][1] = 0;

        result[0][2] = b.forward.x;
        result[1][2] = b.forward.y;
        result[2][2] = b.forward.z;
        result[3][2] = 0;

        result[0][3] = 0;
        result[1][3] = 0;
        result[2][3] = 0;
        result[3][3] = 1;

        return result;
    }

    inline basis basisFromEuler(const glm::vec3& euler) {
        glm::quat q(euler);
        return toBasis(glm::toMat4(q));
    }
}

namespace atto
{
    inline f32 Lerp(const f32 a, const f32 b, const f32 t) {
        return a + (b - a) * t;
    }

    inline glm::vec2 ClampLength(glm::vec2 v, f32 maxLength) {
        f32 magSqrd = glm::dot(v, v);
        if (magSqrd > maxLength * maxLength) {
            return glm::normalize(v) * maxLength;
        }

        return v;
    }

    inline f32 ApproxEqual(f32 a, f32 b, f32 epsilon = 0.0001f) {
        return std::fabs(a - b) < epsilon;
    }

    inline f32 NormalizeEulerAngle(f32 a) {
        if (a > 180.0f) {
            a -= 360.0f;
        }
        else if (a < -180.0f) {
            a += 360.0f;
        }
        
        return a;
    }

    struct BoxBounds {
        glm::vec2 min;
        glm::vec2 max;

        void Translate(const glm::vec2& translation);
        void CreateFromCenterSize(const glm::vec2& center, const glm::vec2& size);
        bool Intersects(const BoxBounds& other);
    };

    struct RayInfo {
        f32 t;
        glm::vec2 normal;
        glm::vec2 point;
    };

    enum AssetType {
        ASSET_TYPE_INVALID = 0,
        ASSET_TYPE_MESH,
        ASSET_TYPE_TEXTURE,
        ASSET_TYPE_AUDIO,
        ASSET_TYPE_FONT,
        ASSET_TYPE_SPRITE,
        ASSET_TYPE_TILESHEET,
        ASSET_TYPE_COUNT
    };

    struct AssetId {
        inline static AssetId Create(const char* str) {
            AssetId id;
            id.id = StringHash::Hash(str);
            return id; 
        }

        inline static AssetId Create(u32 idNumber) {
            AssetId id;
            id.id = idNumber;
            return id;
        }

        inline b8 IsValid() const { return id != 0; }

        inline b8 operator ==(const AssetId& other) const { return id == other.id; }
        inline b8 operator !=(const AssetId& other) const { return id != other.id; }

        u32 id;
    };

    template<u32 _type_>
    class TypedAssetId {
    public:
        inline static TypedAssetId<_type_> Create(const char* str) {
            TypedAssetId<_type_> id;
            id.id = StringHash::Hash(str);
            return id;
        }

        inline b8 IsValid() const { return id != 0; }
        inline u32 GetValue() const { return id; }
        inline AssetId ToRawId() const { return AssetId::Create(id); }

        inline b8 operator ==(const AssetId& other) const { return id == other.id; }
        inline b8 operator !=(const AssetId& other) const { return id != other.id; }

        u32 id;
    };
    
    typedef TypedAssetId<ASSET_TYPE_MESH>    MeshAssetId;
    typedef TypedAssetId<ASSET_TYPE_TEXTURE> TextureAssetId;
    typedef TypedAssetId<ASSET_TYPE_AUDIO>   AudioAssetId;
    typedef TypedAssetId<ASSET_TYPE_FONT>    FontAssetId;
    
    struct MeshData {
        SmallString name;
        List<glm::vec3> positions;
        List<glm::vec3> normals;
        List<glm::vec2> uvs;
        List<u16> indices;
    };

    struct MeshAsset {
        AssetId                         id;
        bool                            isLoaded;
        wrl::ComPtr<ID3D11Buffer>       vertexBuffer;
        wrl::ComPtr<ID3D11Buffer>       indexBuffer;
        u32                             vertexCount;
        u32                             vertexStride;
        u32                             indexCount;
        LargeString                     path;

        static MeshAsset CreateDefault() {
            MeshAsset meshAsset = {};
            return meshAsset;
        }
    };
    
    struct TextureAsset {
        AssetId                                 id;
        bool                                    isLoaded;
        wrl::ComPtr<ID3D11Texture2D>            texture;
        wrl::ComPtr<ID3D11ShaderResourceView>   srv;
        i32                                     slot;
        i32                                     width;
        i32                                     height;
        i32                                     channels;
        bool                                    generateMipMaps;
        LargeString                             path;

        static TextureAsset CreateDefault() {
            TextureAsset textureAsset = {};
            return textureAsset;
        }
    };

    enum ShaderInputLayout {
        INPUT_LAYOUT_DEBUG_LINE = 0,
        INPUT_LAYOUT_BASIC_FONT,
        INPUT_LAYOUT_DRAW_2D,
        INPUT_LAYOUT_POSITION_NORMAL_UV,
    };

    struct ShaderAsset {
        AssetId                         id;
        LargeString                     path;
        ShaderInputLayout               layout;
        wrl::ComPtr<ID3D11VertexShader> vertexShader;
        wrl::ComPtr<ID3D11PixelShader>  pixelShader;
        wrl::ComPtr<ID3D11InputLayout>  inputLayout;
        
        static ShaderAsset CreateDefault() {
            ShaderAsset shaderAsset = {};
            return shaderAsset;
        }
    };

    template<typename _type_>
    struct ShaderBuffer {
        wrl::ComPtr<ID3D11Buffer> buffer;
        i32                       slot;
        _type_                    data;
    };

#define Float4Align __declspec(align(16))

    struct ShaderBufferInstance {
        Float4Align glm::mat4 mvp;
        Float4Align glm::mat4 model;
    };

    struct ShaderBufferCamera {
        Float4Align glm::mat4 projection;
        Float4Align glm::mat4 view;
        Float4Align glm::mat4 screenProjection;
    };

    struct ShaderBufferMaterial {
        Float4Align glm::vec4 settings; // x = Use Triplanar, Y = use textures
        Float4Align glm::vec4 diffuseColor;
    };

    struct ShaderBufferDraw2D {
        Float4Align glm::vec4 posdims;
        Float4Align glm::vec4 innerColor;
        Float4Align glm::vec4 outerColor;
        Float4Align glm::vec4 params;
    };

    struct AudioAsset {
        AssetId     id;
        u32         bufferHandle;
        i32         channels;
        i32         sampleRate;
        i32         sizeBytes;
        i32         bitDepth;
        LargeString path;
        
        static AudioAsset CreateDefault() {
            return {};
        }
    };

    struct Speaker {
        u32         sourceHandle;
        i32         index;
    };

    enum SpriteOrigin {
        SPRITE_ORIGIN_BOTTOM_LEFT = 0,
        SPRITE_ORIGIN_CENTER = 1,
        SPRITE_ORIGIN_COUNT
    };

    struct FontAsset {
        AssetId                                 id;
        bool                                    isLoaded;
        LargeString                             path;
        stbtt_fontinfo                          info;
        FixedList<stbtt_bakedchar, 96>          chardata;
        i32                                     ascent;
        i32                                     descent;
        i32                                     lineGap;
        f32                                     fontSize;
        wrl::ComPtr<ID3D11Texture2D>            texture;
        wrl::ComPtr<ID3D11ShaderResourceView>   srv;

        static FontAsset CreateDefault() {
            FontAsset fontAsset = {};
            fontAsset.fontSize = 24;
            return fontAsset;
        }
    };

    class PackedAssetFile {
    public:
        void        PutData(byte* data, i32 size);
        void        GetData(byte* data, i32 size);

        void        SerializeAsset(TextureAsset& textureAsset);
        void        SerializeAsset(AudioAsset& audioAsset);
        void        SerializeAsset(FontAsset& fontAsset);

        void        Reset();

        bool        Save(const char* name);
        bool        Load(const char* name);

        void        Finished();

        bool        isLoading = false;
        i32         currentOffset = 0;
        List<byte>  storedData;

        template<typename _type_>
        void Serialize(_type_& value) {
            if (isLoading) {
                Get(value);
            }
            else {
                Put(value);
            }
        }

        template<typename _type_>
        void Put(const _type_& data) {
            const i32 s = storedData.GetNum();
            const i32 size = sizeof(_type_);
            storedData.SetNum(s + size, true);
            std::memcpy(storedData.GetData() + s, (const void*)&data, size);
        }

        template<typename _type_>
        void Put(const List<_type_>& data) {
            Put(data.GetNum());
            PutData((byte*)data.GetData(), data.GetNum() * sizeof(_type_));
        }

        template<typename _type_, u32 c>
        void Put(const FixedList<_type_, c>& data) {
            Put(data.GetCount());
            PutData((byte*)data.GetData(), data.GetCount() * sizeof(_type_));
        }

        template<typename _type_>
        void Get(_type_& data) {
            const i32 size = sizeof(_type_);
            Assert(currentOffset + size <= storedData.GetNum(), "PackedAssetFile::Get: currentOffset + size > storedData.GetNum()");
            byte* p = storedData.GetData() + currentOffset;
            std::memcpy((void*)&data, p, size);
            currentOffset += size;
        }

        template<typename _type_>
        void        Get(List<_type_>& data) {
            i32 num = 0;
            Get(num);
            data.SetNum(num, true);
            GetData((byte*)data.GetData(), num * sizeof(_type_));
        }

        template<typename _type_, u32 c>
        void        Get(FixedList<_type_, c>& data) {
            u32 num = 0;
            Get(num);
            data.SetCount(num);
            GetData((byte*)data.GetData(), data.GetCount() * sizeof(_type_));
        }
    };

    struct DebugDrawVertex {
        glm::vec4 positionColorIndex;
    };

    struct DebugDrawState {
        FixedList<DebugDrawVertex, 1000>        lineVertices;
        wrl::ComPtr<ID3D11Buffer>               vertexBuffer;
        ShaderAsset                             shader;
    };
    
    struct SamplerStates {
        wrl::ComPtr<ID3D11SamplerState> pointClamp;
        wrl::ComPtr<ID3D11SamplerState> pointWrap;
        wrl::ComPtr<ID3D11SamplerState> linearClamp;
        wrl::ComPtr<ID3D11SamplerState> linearWrap;
        wrl::ComPtr<ID3D11SamplerState> anisotropicClamp;
        wrl::ComPtr<ID3D11SamplerState> anisotropicWrap;
    };

    struct DepthStates {
        wrl::ComPtr<ID3D11DepthStencilState> depthDisabled;
        wrl::ComPtr<ID3D11DepthStencilState> depthEnabled;
        wrl::ComPtr<ID3D11DepthStencilState> depthEnabledNoWrite;
    };

    struct RasterizerStates {
        wrl::ComPtr<ID3D11RasterizerState> cullNone;
        wrl::ComPtr<ID3D11RasterizerState> cullBack;
        wrl::ComPtr<ID3D11RasterizerState> cullFront;
        wrl::ComPtr<ID3D11RasterizerState> wireframe;
    };
    
    struct BlendStates {
        wrl::ComPtr< ID3D11BlendState> opaqueBlend;
        wrl::ComPtr< ID3D11BlendState> alphaBlend;
        wrl::ComPtr< ID3D11BlendState> additiveBlend;
    };

    struct GlobalRenderer {
        wrl::ComPtr<IDXGIFactory2>              factory;
        wrl::ComPtr<ID3D11Device>               device;
        wrl::ComPtr<ID3D11DeviceContext>        context;
        wrl::ComPtr<IDXGISwapChain1>            swapChain;
        wrl::ComPtr<ID3D11RenderTargetView>     swapChainRenderTarget;
        wrl::ComPtr<ID3D11Texture2D>            swapChainDepthBuffer;
        wrl::ComPtr<ID3D11DepthStencilView>     swapChainDepthView;
        i32                                     swapChainWidth;
        i32                                     swapChainHeight;
        MeshAsset                               unitQuad;
        MeshAsset                               unitCube;
        MeshAsset                               unitHex;
        ShaderAsset                             testingShader;
        ShaderBuffer<ShaderBufferInstance>      shaderBufferInstance;
        ShaderBuffer<ShaderBufferCamera>        shaderBufferCamera;
        ShaderBuffer<ShaderBufferMaterial>      shaderBufferMaterial;
        ShaderBuffer<ShaderBufferDraw2D>        shaderBufferDraw2D;
        SamplerStates                           samplerStates;
        DepthStates                             depthStates;
        RasterizerStates                        rasterizerStates;
        BlendStates                             blendStates;
        DebugDrawState                          debugDrawState;
        ShaderAsset                             fontShader;
        wrl::ComPtr<ID3D11Buffer>               fontVertexBuffer;
        ShaderAsset                             draw2DShader;
        wrl::ComPtr<ID3D11Buffer>               draw2DVertexBuffer;
        
    };

    struct GlobalAudio {
        wrl::ComPtr<IXAudio2>                  xAudio2;
        IXAudio2MasteringVoice*               masteringVoice;
    };

    enum Draw2DPrimitiveType {
        Draw2DPrimitiveType_Rect = 0,
        Draw2DPrimitiveType_Rect_Outlined = 1,
        Draw2DPrimitiveType_Rect_Rounded,
        Draw2DPrimitiveType_Num,
    };

    struct Draw2DParams {
        Draw2DPrimitiveType primType;
        glm::vec2           pos;
        glm::vec2           dims;
        glm::vec4           innerColor;
        glm::vec4           outerColor;
        f32                 d;
    };

    struct Camera {
        glm::vec3   pos;
        glm::basis  ori;
        f32         fov;
        f32         nearPlane;
        f32         farPlane;
        f32         pitch;
        f32         yaw;
        f32         zoom;

        inline glm::mat4 GetViewMatrix() const {
            return glm::lookAt(pos, pos + ori.forward, ori.up);
        }

        inline static Camera CreateDefault() {
            Camera camera = {};
            camera.pos.z = 3.0f;
            camera.ori.right = glm::vec3(1.0f, 0.0f, 0.0f);
            camera.ori.up = glm::vec3(0.0f, 1.0f, 0.0f);
            camera.ori.forward = glm::vec3(0.0f, 0.0f, -1.0f);
            camera.fov = glm::radians(45.0f);
            camera.nearPlane = 0.1f;
            camera.farPlane = 100.0f;
            camera.pitch = 0.0f;
            camera.yaw = -90.0f;
            return camera;
        }

        inline static Camera CreateIsometric() {
            Camera camera = {};
            camera.pos = glm::vec3(7, 14, 7) * 0.5f;
            camera.zoom = 1.0f;
            camera.fov = glm::radians(45.0f);
            camera.ori.right = glm::vec3(1.0f, 0.0f, 0.0f);
            camera.ori.up = glm::vec3(0.0f, 1.0f, 0.0f);
            camera.ori.forward = glm::vec3(0.0f, 0.0f, -1.0f);
            glm::mat4 rot = glm::lookAt(camera.pos, glm::vec3(0,0,0), glm::vec3(0, 1, 0));
            
            //glm::mat4 rot = glm::rotate(glm::toMat4(camera.ori), glm::radians(35.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            //rot = glm::rotate(rot, glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            camera.ori = glm::toBasis(rot);
            camera.ori.forward = -camera.ori.forward;
            camera.nearPlane = 1.0f;
            camera.farPlane = 100.0f;
            camera.pos += glm::vec3(3.2, 0, 3.2);

            return camera;
        }

        inline static Camera CreateTopDown() {
            Camera camera = {};
            camera.zoom = 1.0f;
            camera.fov = glm::radians(45.0f);
            camera.nearPlane = 1.0f;
            camera.farPlane = 100.0f;
            camera.pos          = glm::vec3(7, 14, 7) * 0.5f;
            camera.ori.right    = glm::vec3(1.0f, 0.0f, 0.0f);
            camera.ori.up       = glm::vec3(0.0f, 0.4602f, -0.8877f);
            camera.ori.forward  = glm::vec3(0.0f, -0.8878f, -0.4601f);
            
            camera.pos += glm::vec3(1, 0, 4);
            return camera;
        }
    };

    enum FontHAlignment {
        FONT_HALIGN_LEFT,
        FONT_HALIGN_CENTER,
        FONT_HALIGN_RIGHT
    };

    enum FontVAlignment {
        FONT_VALIGN_BOTTOM,
        FONT_VALIGN_MIDDLE,
        FONT_VALIGN_TOP,
    };

    struct DrawEntryFont {
        LargeString         text;
        FontAsset*          font;
        FontHAlignment      hAlignment;
        FontVAlignment      vAlignment;
        glm::vec4           color;
        f32                 underlineThinkness;
        f32                 underlinePercent;
        f32                 textWidth;
        glm::vec2           pos;
        BoxBounds           bounds;
    };
    
    struct TextRenderingState {
        FontAsset *         font;
        FontHAlignment      hAlignment;
        FontVAlignment      vAlignment;
        f32                 underlineThinkness;
        f32                 underlinePercent;
        glm::vec4           color;
        //ShaderProgram       program;
        //VertexBuffer        vertexBuffer;
    };

    struct AABB2D {
        glm::vec2 min;
        glm::vec2 max;
        
        inline glm::vec2 GetCenter() const {
            return (min + max) * 0.5f;
        }
        
        inline glm::vec2 GetSize() const {
            return max - min;
        }

        inline bool Contains(glm::vec2 p) const {
            return p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y;
        }

        inline static AABB2D CreateFromMinMax(glm::vec2 min, glm::vec2 max) {
            AABB2D aabb;
            aabb.min = min;
            aabb.max = max;
            return aabb;
        }
    };

    struct UIWindow {
        SmallString     title;
        glm::vec2       pos;
        glm::vec2       dims;
        glm::vec2       dragOffset;
        bool            isBeingDragged;
    };

    struct UIContext {
        FontAsset*                  font;
        u32                         cursorPos;
        FixedList<SmallString, 8>   menuElemets;
        FixedList<UIWindow, 8>      windows;
    };

    struct Ray {
        glm::vec3 origin;
        glm::vec3 direction;

        inline glm::vec3 Travel(f32 t) const {
            return origin + direction * t;
        }
    };
    
    struct Plane {
        glm::vec3 normal;
        f32 d;

        inline static Plane Create(glm::vec3 point, glm::vec3 normal) {
            Plane plane = {};
            plane.normal = normal;
            plane.d = glm::dot(normal, point);
            return plane;
        }
    };

    struct Triangle {
        glm::vec3 a;
        glm::vec3 b;
        glm::vec3 c;
        
        inline static Triangle Create(glm::vec3 a, glm::vec3 b, glm::vec3 c) {
            Triangle triangle = {};
            triangle.a = a;
            triangle.b = b;
            triangle.c = c;
            return triangle;
        }
    };

    struct PNTTriangle {
        glm::vec3 aPos;
        glm::vec3 aNrm;
        glm::vec2 aUv;
        glm::vec3 bPos;
        glm::vec3 bNrm;
        glm::vec2 bUv;
        glm::vec3 cPos;
        glm::vec3 cNrm;
        glm::vec2 cUv;
        
        inline static PNTTriangle Create(glm::vec3 aPos, glm::vec3 aNrm, glm::vec2 aUv, glm::vec3 bPos, glm::vec3 bNrm, glm::vec2 bUv, glm::vec3 cPos, glm::vec3 cNrm, glm::vec2 cUv) {
            PNTTriangle triangle = {};
            triangle.aPos = aPos;
            triangle.aNrm = aNrm;
            triangle.aUv = aUv;
            triangle.bPos = bPos;
            triangle.bNrm = bNrm;
            triangle.bUv = bUv;
            triangle.cPos = cPos;
            triangle.cNrm = cNrm;
            triangle.cUv = cUv;
            return triangle;
        }
    };

    class RayTests {
    public:
        static bool RayPlaneIntersection(const Ray& ray, const Plane& plane, f32& t) {
            f32 denom = std::abs( glm::dot(plane.normal, ray.direction) );
            if ( !ApproxEqual(denom, 0.0) ) {
                glm::vec3 p0l0 = ray.origin;
                t = glm::dot(plane.normal, p0l0) + plane.d;
                t /= denom;
                return true;
            }
            return false;
        }
    };

    struct Material {
        MeshAsset*      mesh;
        TextureAsset*   diffuseMap;
        glm::vec4       diffuseColor;
        bool            useTriplanar;
        
        inline static Material CreateDefault() {
            Material material = {};
            material.diffuseColor = glm::vec4(0, 0, 0, 1.0f);
            return material;
        }
    };

    struct EntityRef {
        u32 index;
        u32 generation;
    };

    struct Unit {
        bool active;
        bool isSelected;
        i32 elevation;
        f32 rotation;
        glm::vec2 pos;
        glm::vec2 vel;
        glm::vec2 steering;
        
        // Target stuff
        bool hasTarget;
        glm::vec2 targetPos;
        EntityRef targetEnt;
    };

    struct MapTile {
        
    };

    struct Map {
        i32                         width;
        i32                         height;
        FixedList<MapTile, 1024>    tiles;

        inline i32 ToFlatIndex(i32 x, i32 y) const {
            return y * width + x;
        }

        inline MapTile& GetTile(i32 x, i32 y) {
            return tiles[ToFlatIndex(x, y)];
        }
    };

    struct Entity {
        glm::vec3       pos;
        glm::basis      ori;
        Material        material;
        BoxBounds       boundingBox;
        Unit            unit;
    };

    struct EditorState {
        bool                editorActive;
        UIContext           uiContext;
        Camera              camera;
        bool                isFlying;
    };

    class LeEngine {
    public:
        bool                                Initialize(AppState* app);
        void                                Update(AppState* app);
        void                                Render(AppState* app);
        void                                Shutdown();
        
        void                                CallbackResize(i32 width, i32 height);
        void                                CallbackMouseWheel(f32 x, f32 y);
        void                                CallbackMousePosition(f32 x, f32 y);

        f32                                 Random();
        f32                                 Random(f32 min, f32 max);
        i32                                 RandomInt(i32 min, i32 max);

        glm::vec2                           ScreenPosToNDC(glm::vec2 pos);
        glm::vec2                           ScreenToWorld(glm::vec2 pos);

        void                                CameraSet(Camera& camera);
        void                                CameraDoFreeFlyKeys(Camera &camera);
        void                                CameraDoFreePanKeys(Camera& camera);
        void                                CameraDoFreeFlyMouse(Camera& camera, f32 x, f32 y);
        Ray                                 CameraGetRay(Camera& camera, glm::vec2 pos);

        Entity*                             EntityCreate();
        Entity*                             EntityCreateProptypeWall(glm::vec2 pos);
        
        void                                UnitSetPos(Entity *unit, glm::vec2 pos);
        glm::vec2                           UnitSteerSeekCurrentTarget(const Unit& unit);
        glm::vec2                           UnitSteerSeekCurrentTargetKinematic(const Unit& unit);

        void                                RegisterAssets();

        MeshAsset*                          LoadMeshAsset(MeshAssetId id);
        void                                FreeMeshAsset(MeshAssetId id);

        TextureAsset*                       LoadTextureAsset(TextureAssetId id);
        void                                FreeTextureAsset(TextureAssetId id);

        FontAsset*                          LoadFontAsset(FontAssetId id);

        AudioAsset*                         LoadAudioAsset(AudioAssetId id);
        void                                FreeAudioAsset(AudioAssetId id);

        Speaker                             AudioPlay(AudioAssetId audioAssetId, bool looping = false, f32 volume = 1.0f);
        void                                AudioPause(Speaker speaker);
        void                                AudioStop(Speaker speaker);
        bool                                AudioIsSpeakerPlaying(Speaker speaker);
        bool                                AudioIsSpeakerAlive(Speaker speaker);

        void                                UIResetContext(UIContext& context);
        void                                UIRender(UIContext& context);
        void                                UIBeginWindow(UIContext& context, const char* title, const glm::vec2& firstPos, const glm::vec2& firstSize);
        void                                UIEndWindow(UIContext& context);
        void                                UIBeginMainMenuBar(UIContext& context);
        bool                                UIBeginMenu(UIContext& context, const char* text);
        void                                UIEndMainMenuBar(UIContext& context);


        glm::mat4                           cameraProjection;
        glm::mat4                           screenProjection;

        LargeString                         basePathSprites;
        
        Ray                                 testRay;
        glm::vec3                           testPoint;

    private:
        bool                                InitializeRenderer();
        bool                                InitializeDebug();
        bool                                InitializeFonts();
        bool                                InitializeDraw2D();
        bool                                InitializeAudio();

        byte*                               LoadEntireFile(const char *path, i32 &fileSize);

        void                                ShaderGetInputLayout(ShaderInputLayout layout, FixedList<D3D11_INPUT_ELEMENT_DESC, 8> & list);
        ID3DBlob*                           ShaderCompileFile(const char* path, const char* entry, const char* target);
        ID3DBlob*                           ShaderCompileSource(const char* source, const char* entry, const char* target);
        bool                                ShaderCompile(ShaderAsset& shader);
        bool                                ShaderCompile(const char *source, ShaderInputLayout layout, ShaderAsset& shader);
        void                                ShaderBind(ShaderAsset& shader);
        
        template<typename _type_> void      ShaderBufferBind(ShaderBuffer<_type_>& shaderBuffer, i32 slot, bool vertexShader, bool pixelShader);
        template<typename _type_> void      ShaderBufferUpload(ShaderBuffer<_type_>& shaderBuffer);
        template<typename _type_> void      ShaderBufferCreate(ShaderBuffer<_type_>& shaderBuffer);

        void                                MeshCreateUnitQuad(MeshAsset& quad);
        void                                MeshCreateUnitCube(MeshAsset& cube);
        void                                MeshCreateHex(MeshAsset& hex, f32 outerRadius, f32 innerRadius);
        void                                MeshDataPackPNT(const MeshData &meshData, const glm::mat3 &scalingMatrix, List<f32> &data);
        void                                MeshCreate(MeshAsset& mesh);
        void                                MeshBind(MeshAsset* mesh);
        void                                MeshDraw(MeshAsset* mesh);

        void                                TextureCreate(TextureAsset& texture);
        void                                TextureBind(TextureAsset* texture, i32 slot);
        
        void                                FontCreate(FontAsset& font);
        f32                                 FontWidth(FontAsset* fontAsset, const char* text);
        void                                FontRenderText(const char* text, FontAsset*fontAsset, glm::vec2 pos, glm::vec4 color = glm::vec4(1, 1, 1, 1));
        void                                FontRenderText(const char *text, FontAssetId fontId, glm::vec2 pos, glm::vec4 color = glm::vec4(1,1,1,1));

        void                                Draw2DPrimitive(const Draw2DParams &params);
        void                                Draw2DRectPosDims(glm::vec2 pos, glm::vec2 dims, glm::vec4 color = glm::vec4(1, 1, 1, 1));
        void                                Draw2DRectCenterDims(glm::vec2 center, glm::vec2 dims, glm::vec4 color = glm::vec4(1,1,1,1));
        void                                Draw2DRectOutlinePosDims(glm::vec2 pos, glm::vec2 dims, f32 w, glm::vec4 outerColor, glm::vec4 innerColor);
        void                                Draw2DRoundedRect(glm::vec2 pos, f32 r, glm::vec4 color = glm::vec4(1, 1, 1, 1));
        void                                Draw2DCircle(glm::vec2 pos, f32 r, glm::vec4 color = glm::vec4(1, 1, 1, 1));

        void                                AudioCreate(AudioAsset& audio);

#if     ATTO_DEBUG_RENDERING
        void                                DebugRender();
        void                                DebugAddLine(glm::vec3 a, glm::vec3 b);
        void                                DebugAddPoint(glm::vec3 p);
        void                                DebugAddCircle(glm::vec3 p, glm::vec3 n, f32 radius);
        void                                DebugAddRay(Ray ray);
#endif

        template<typename _type_> _type_*   FindAsset(FixedList<_type_, 2048> & assetList, AssetId id);

        AppState*                           app;

        GlobalRenderer                      renderer;
        GlobalAudio                         audio;

        EditorState                         editorState;

        Camera                              gameCamera;
        Camera*                             currentCamera;

        MeshAsset*                          primitiveCapsule;
        MeshAsset*                          buildingBase_01;
        MeshAsset*                          buildingBlock1x1_01;
        MeshAsset*                          buildingBlock1x1_02;
        MeshAsset*                          buildingBlock1x1_03;
        MeshAsset*                          tank_01;
        TextureAsset*                       textureGrid_01;
        TextureAsset*                       textureTriplanarTest;
        TextureAsset*                       textureBaseTank;

        FixedList<MeshAsset,    2048>       meshAssets;
        FixedList<TextureAsset, 2048>       textureAssets;
        FixedList<FontAsset,    2048>       fontAssets;
        FixedList<AudioAsset,   2048>       audioAssets;

        FixedList<Speaker,       64>        speakers;
        FixedList<Entity,      2048>        entities;
    };

    template<typename _type_>
    void LeEngine::ShaderBufferBind(ShaderBuffer<_type_>& shader, i32 slot, bool vertexShader, bool pixelShader) {
        shader.slot = slot;
        if (vertexShader) {
            renderer.context->VSSetConstantBuffers(slot, 1, shader.buffer.GetAddressOf());
        }
        if (pixelShader) {
            renderer.context->PSSetConstantBuffers(slot, 1, shader.buffer.GetAddressOf());
        }
    }

    template<typename _type_>
    void LeEngine::ShaderBufferUpload(ShaderBuffer<_type_>& shaderBuffer) {
        renderer.context->UpdateSubresource(shaderBuffer.buffer.Get(), 0, nullptr, &shaderBuffer.data, 0, 0);
    }

    template<typename _type_>
    void LeEngine::ShaderBufferCreate(ShaderBuffer<_type_>& shaderBuffer) {
        u32 sizeBytes = sizeof(_type_);
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeBytes;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        
        HRESULT hr = renderer.device->CreateBuffer(&desc, nullptr, &shaderBuffer.buffer);
        if (FAILED(hr)) {
            ATTOERROR("Failed to create constant buffer");
        }
    }

    template<typename _type_>
    _type_* LeEngine::FindAsset(FixedList<_type_, 2048> & assetList, AssetId id) {
        const i32 assetListCount = assetList.GetCount();
        for (i32 assetIndex = 0; assetIndex < assetListCount; assetIndex++) {
            _type_* asset = &assetList[assetIndex];
            if (asset->id == id) {
                return asset;
            }
        }
        
        ATTOERROR("Could not find asset with id %d", id.id);

        return nullptr;
    }

}
