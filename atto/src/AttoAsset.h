#pragma once

#include "AttoLib.h"
#include "AttoLua.h"

namespace atto
{
    inline f32 Lerp(const f32 a, const f32 b, const f32 t) {
        return a + (b - a) * t;
    }

    struct Ray2D {
        glm::vec2 origin;
        glm::vec2 direction;
    };

    struct Circle {
        glm::vec2   pos;
        f32         rad;
    };

    struct BoxBounds {
        glm::vec2 min;
        glm::vec2 max;

        void Translate(const glm::vec2& translation);
        void CreateFromCenterSize(const glm::vec2& center, const glm::vec2& size);
        bool BoxBounds::Intersects(const BoxBounds& other);
    };

    struct RayInfo {
        f32 t;
        glm::vec2 normal;
        glm::vec2 point;
    };

    class Bitmap {
    public:
#pragma pack(push, 1)
        struct FileHeader {
            char type[2];
            u32 size;
            u16 reserved1;
            u16 reserved2;
            u32 offset;
        };

        struct InfoHeader {
            u32 size;
            i32 width;
            i32 height;
            u16 planes;
            u16 bitCount;
            u32 compression;
            u32 sizeImage;
            i32 xPelsPerMeter;
            i32 yPelsPerMeter;
            u32 clrUsed;
            u32 clrImportant;
        };
#pragma pack(pop)

        static bool Write(byte* pixels, u32 width, u32 height, const char* name);
    };

    class TileSheetGenerator {
    public:
        struct Tile {
            u32 width;
            u32 height;
            u32 xPos;
            u32 yPos;
            FixedList<byte, 2048> data;
            glm::vec2 uv0;
            glm::vec2 uv1;
            glm::vec2 boundingUV0;
            glm::vec2 boundingUV1;
        };

        void            AddTile(u32 width, u32 height, void* data);
        void            GenerateTiles(List<byte>& pixels, i32& width, i32& height);
        void            GetTileUV(i32 index, glm::vec2& uv0, glm::vec2& uv1);

        List<Tile>      tiles;
        u32             pixelStrideBytes = 4;
    };

    enum AssetType {
        ASSET_TYPE_INVALID = 0,
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

    typedef TypedAssetId<ASSET_TYPE_TEXTURE> TextureAssetId;
    typedef TypedAssetId<ASSET_TYPE_AUDIO>   AudioAssetId;
    typedef TypedAssetId<ASSET_TYPE_FONT>    FontAssetId;
    
    struct TextureAsset {
        u32         textureHandle;
        i32         width;
        i32         height;
        i32         channels;
        i32         wrapMode;
        bool        generateMipMaps;

        static TextureAsset CreateDefault() {
            TextureAsset textureAsset = {};
            textureAsset.wrapMode = 0x2901; // GL_REPEAT
            textureAsset.generateMipMaps = true;

            return textureAsset;
        }
    };

    struct AudioAsset {
        u32         bufferHandle;
        i32         channels;
        i32         sampleRate;
        i32         sizeBytes;
        i32         bitDepth;

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

    struct SpriteAsset {
        AssetId                 id;

        // Texture stuffies
        glm::vec2               uv0;
        glm::vec2               uv1;
        TextureAssetId          textureId;
        const TextureAsset*     texture;

        // Animation stuffies
        bool                    animated;

        glm::vec2               frameSize;
        i32                     frameCount;
        SpriteOrigin            origin;

        inline static SpriteAsset CreateDefault() {
            SpriteAsset spriteAsset = {};
            spriteAsset.uv1 = glm::vec2(1, 1);
            spriteAsset.frameCount = 1;
            return spriteAsset;
        }
    };

    struct Glyph {
        glm::ivec2      size;       // Size of glyph
        glm::ivec2      bearing;    // Offset from baseline to left/top of glyph
        glm::vec2       uv0;        // Uv0
        glm::vec2       uv1;        // Uv1
        u32             advance;    // Horizontal offset to advance to next glyph
    };

    struct FontAsset {
        u32                     textureHandle;
        i32                     width;
        i32                     height;
        
        i32                     fontSize; // TODO: Think of a good way to set these things...
        FixedList<Glyph, 128>   glyphs;

        static FontAsset CreateDefault() {
            FontAsset fontAsset = {};
            fontAsset.fontSize = 38;
            return fontAsset;
        }
    };

    struct VertexBuffer {
        u32 vao;
        u32 vbo;
        i32 size;
        i32 stride;
    };

    struct VertexBufferIndexed {
        u32 vao;
        u32 vbo;
        u32 ibo;
    };

    struct ShaderUniform {
        SmallString name;
        i32 location;
    };

    struct ShaderProgram {
        u32                                 programHandle;
        FixedList<ShaderUniform, 16>        uniforms;
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
    
    struct EngineAsset {
        AssetType               type = ASSET_TYPE_INVALID;
        AssetId                 id = {};
        LargeString             path = {};

        union {
            TextureAsset   texture;
            FontAsset      font;
            AudioAsset     audio;
        };
    };
    
    struct ShapeVertex {
        glm::vec2 position;
    };

    struct ShapeRenderingState {
        glm::vec4           color;
        ShaderProgram       program;
        VertexBuffer        vertexBuffer;
    };

    struct SpriteVertex {
        glm::vec2 position;
        glm::vec2 uv;
        glm::vec4 color;
    };

    struct SpriteRenderingState {
        glm::vec4           color;
        ShaderProgram       program;
        VertexBuffer        vertexBuffer;
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

    struct FontVertex {
        glm::vec2 position;
        glm::vec2 uv;
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
        ShaderProgram       program;
        VertexBuffer        vertexBuffer;
    };

    struct DebugLineVertex {
        glm::vec2 position;
        glm::vec4 color;
    };

    struct DebugRenderingState {
        ShaderProgram                       program;
        VertexBuffer                        vertexBufer;
        FixedList<DebugLineVertex, 2048>    lines;
        glm::vec4                           color;
    };

    enum VertexLayoutType {
        VERTEX_LAYOUT_TYPE_SHAPE,           // Vec2(POS)
        VERTEX_LAYOUT_TYPE_SPRITE,          // Vec2(POS), Vec2(UV), Vec4(COLOR)
        VERTEX_LAYOUT_TYPE_FONT,            // Vec2(POS), Vec2(UV)
        VERTEX_LAYOUT_TYPE_DEBUG_LINE,      // Vec2(POS), Vec4(COLOR)
    };

    struct GlobalRenderingState {
        ShaderProgram *                     program;
    };

    struct SpriteInstance {
        f32                     animationDuration;
        f32                     animationPlayhead;
    };

    struct EditorState {
        bool            consoleOpen;
        f32             consoleScroll;
    };
    
    struct EntitySprite {
        bool            active;
        SpriteAsset*    sprite;
        f32             animationDuration;
        f32             animationPlayhead;
    };

    struct Entity {
        glm::vec2       pos;
        glm::vec2       vel;
        f32             rotation;
        BoxBounds       boundingBox;
        EntitySprite    sprite;
    };

    class LeEngine {
    public:
        bool                                Initialize(AppState* app);
        void                                Update(AppState* app);
        void                                Render(AppState* app);
        void                                Shutdown();
        
        void                                MouseWheelCallback(f32 x, f32 y);

        f32                                 Random();
        f32                                 Random(f32 min, f32 max);
        i32                                 RandomInt(i32 min, i32 max);

        glm::vec2                           ScreenPosToNDC(glm::vec2 pos);
        glm::vec2                           GetMousePosWorldSpace();

        virtual void                        RegisterAssets() = 0;

        const void *                        LoadEngineAsset(AssetId id, AssetType type);
        
        TextureAsset*                       LoadTextureAsset(TextureAssetId id);
        void                                FreeTextureAsset(TextureAssetId id);

        FontAsset*                          LoadFontAsset(FontAssetId id);
        void                                FreeAudioAsset(AudioAssetId id);

        AudioAsset*                         LoadAudioAsset(AudioAssetId id);

        Speaker                             AudioPlay(AudioAssetId audioAssetId, bool looping = false, f32 volume = 1.0f);
        void                                AudioPause(Speaker speaker);
        void                                AudioStop(Speaker speaker);
        bool                                AudioIsSpeakerPlaying(Speaker speaker);
        bool                                AudioIsSpeakerAlive(Speaker speaker);

        SpriteAsset*                        GetSpriteAsset(AssetId id);

        void                                ShaderProgramBind(ShaderProgram* program);
        i32                                 ShaderProgramGetUniformLocation(const char* name);
        void                                ShaderProgramSetInt(const char* name, i32 value);
        void                                ShaderProgramSetSampler(const char* name, i32 value);
        void                                ShaderProgramSetTexture(i32 location, u32 textureHandle);
        void                                ShaderProgramSetFloat(const char* name, f32 value);
        void                                ShaderProgramSetVec2(const char* name, glm::vec2 value);
        void                                ShaderProgramSetVec3(const char* name, glm::vec3 value);
        void                                ShaderProgramSetVec4(const char* name, glm::vec4 value);
        void                                ShaderProgramSetMat3(const char* name, glm::mat3 value);
        void                                ShaderProgramSetMat4(const char* name, glm::mat4 value);

        void                                VertexBufferUpdate(VertexBuffer vertexBuffer, i32 offset, i32 size, const void* data);

        void                                DrawSurfaceResized(i32 w, i32 h);

        void                                DrawClearSurface(const glm::vec4& color = glm::vec4(0, 0, 0, 1));
        void                                DrawEnableAlphaBlending();
        
        void                                DrawShapeSetColor(glm::vec4 color);
        void                                DrawShapeRect(glm::vec2 bl, glm::vec2 tr);
        void                                DrawShapeRectCenterDim(glm::vec2 center, glm::vec2 dim);
        void                                DrawShapeCircle(glm::vec2 center, f32 radius);
        void                                DrawShapeRoundRect(glm::vec2 bl, glm::vec2 tr, f32 radius = 10.0f);

        void                                DrawSpriteSetColor(glm::vec4 color);
        void                                DrawSprite(const AssetId& id, glm::vec2 pos, f32 rotation, i32 frameIndex);
        void                                DrawSprite(SpriteAsset* spriteAsset, glm::vec2 pos, f32 rotation, i32 frameIndex);

        void                                DrawTextSetFont(FontAssetId id);
        void                                DrawTextSetColor(glm::vec4 color);
        void                                DrawTextSetHalign(FontHAlignment hAlignment);
        void                                DrawTextSetValign(FontVAlignment vAlignment);
        f32                                 DrawTextWidth(const char* text);
        BoxBounds                           DrawTextBounds(const char* text);
        f32                                 DrawTextWidth(FontAsset* font, const char* text);
        BoxBounds                           DrawTextBounds(FontAsset *font, const char* text);
        DrawEntryFont                       DrawTextCreate(const char* text, glm::vec2 pos);
        void                                DrawText(const char* text, glm::vec2 pos);
        void                                DrawText(SmallString text, glm::vec2 pos);

        void                                DEBUGPushLine(glm::vec2 a, glm::vec2 b);
        void                                DEBUGPushRay(Ray2D ray);
        void                                DEBUGPushCircle(glm::vec2 pos, f32 radius);
        void                                DEBUGPushCircle(Circle circle);
        void                                DEBUGPushBox(BoxBounds bounds);
        void                                DEBUGSubmit();

        void                                EditorToggleConsole();

        glm::mat4                           cameraProjection;
        glm::mat4                           cameraView;
        glm::mat4                           screenProjection;

        glm::vec2                           cameraPos;
        f32                                 cameraZoom;

        i32                                 mainSurfaceWidth;
        i32                                 mainSurfaceHeight;

    protected:
        virtual bool                        LoadTextureAsset(const char* name, TextureAsset& textureAsset) = 0;
        virtual bool                        LoadAudioAsset(const char* name, AudioAsset& audioAsset) = 0;
        virtual bool                        LoadFontAsset(const char* name, FontAsset& fontAsset) = 0;

        void                                InitializeShapeRendering();
        void                                InitializeSpriteRendering();
        void                                InitializeTextRendering();
        void                                InitializeDebugRendering();
        void                                InitializeLuaBindings();

        VertexBuffer                        SubmitVertexBuffer(i32 sizeBytes, const void* data, VertexLayoutType layoutType, bool dyanmic);
        ShaderProgram                       SubmitShaderProgram(const char* vertexSource, const char* fragmentSource);
        u32                                 SubmitTextureR8B8G8A8(i32 width, i32 height, byte* data, i32 wrapMode, bool generateMipMaps);
        u32                                 SubmitAudioClip(i32 sizeBytes, byte* data, i32 channels, i32 bitDepth, i32 sampleRate);
        
        void                                ALCheckErrors();
        u32                                 ALGetFormat(u32 numChannels, u32 bitDepth);
        bool                                GLCheckShaderCompilationErrors(u32 shader);
        bool                                GLCheckShaderLinkErrors(u32 program);

        AppState*                           app;

        LuaScript                           luaEngine;


        LargeString                         basePathSprites;
        LargeString                         basePathSounds;

        GlobalRenderingState                globalRenderingState;
        ShapeRenderingState                 shapeRenderingState;
        SpriteRenderingState                spriteRenderingState;
        TextRenderingState                  textRenderingState;
        DebugRenderingState                 debugRenderingState;
        EditorState                         editorState;

        FixedList<EngineAsset, 2048>        engineAssets;      // These never get moved, so it's safe to store a pointer to them.
        FixedList<SpriteAsset, 2048>        registeredSprites; // These never get moved, so it's safe to store a pointer to them.

        FixedList<Speaker,       64>        speakers;
        FixedList<Entity,      2048>        entities;

    private:
        
        static i32 Lua_IdFromString(lua_State* L);
        
        static i32 Lua_AudioPlay(lua_State* L);
        
        static i32 Lua_DrawShapeSetColor(lua_State *L);
        static i32 Lua_DrawShapeRect(lua_State *L);
        static i32 Lua_DrawShapeRectCenterDim(lua_State *L);
        static i32 Lua_DrawShapeCircle(lua_State *L);
        static i32 Lua_DrawShapeRoundRect(lua_State* L);

        static i32 Lua_DrawSprite(lua_State* L);

    };

    class LooseAssetLoader : public LeEngine {
    public:
        virtual void                RegisterAssets() override;

    protected:
        virtual bool                LoadTextureAsset(const char* name, TextureAsset& textureAsset) override;
        virtual bool                LoadAudioAsset(const char* name, AudioAsset& audioAsset) override;
        virtual bool                LoadFontAsset(const char* name, FontAsset& fontAsset) override;

    private:
        bool                        LoadWAV(const char* filename, AudioAsset& audioAsset);
        bool                        LoadOGG(const char* filename, AudioAsset& audioAsset);

        PackedAssetFile             texturePakFile;
        PackedAssetFile             audioPakFile;
        PackedAssetFile             fontPakFile;
    };

    //class PackedAssetLoader : public AssetLoader {
    //    virtual bool                Begin() override;
    //    virtual bool                LoadTextureAsset(const char* name, TextureAsset& textureAsset) override;
    //    virtual bool                LoadAudioAsset(const char* name, AudioAsset& audioAsset) override;
    //    virtual bool                LoadFontAsset(const char* name, FontAsset& fontAsset) override;
    //    virtual void                End() override;

    //private:
    //    PackedAssetFile             texturePakFile;
    //    PackedAssetFile             audioPakFile;
    //    PackedAssetFile             fontPakFile;
    //};

}
