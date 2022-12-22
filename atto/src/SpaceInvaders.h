#pragma once

#include "containers/AttoFixedList.h"
#include "containers/AttoFixedString.h"
#include "containers/AttoList.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace atto
{
    class StringBuilder {
    public:
        static SmallString FormatSmall(const char* format, ...);
        static LargeString FormatLarge(const char* format, ...);
    };

    struct FrameInput {
        glm::vec2 mousePosition;
        glm::vec2 mouseDelta;
        glm::vec2 mouseWheel;
        bool mouseLeft;
        bool mouseRight;
        bool mouseMiddle;
        bool escape;
        bool enter;
        bool space;
        bool tab;
        bool backspace;
        bool a;
        bool d;
        bool w;
        bool s;
        bool e;
        FrameInput* lastInput = nullptr;
    };

#define IsKeyJustDown(key) (input->key && !input->lastInput->key)

    struct WindowData {
        SmallString title;
        u32 width;
        u32 height;
        f32 aspect;
        bool shouldClose;
    };

    struct ShaderUniformValue {
        SmallString name;
        i32 location = -1;
    };

    class SoftwareRenderSurface {
    public:
        void CreateEmpty(i32 width, i32 height);
        void SetPixel(i32 x, i32 y, u8 r, u8 g, u8 b, u8 a);
        void SetPixelf(i32 x, i32 y, f32 r, f32 g, f32 b, f32 a);
        void Blit(i32 x, i32 y, i32 width, i32 height, f32 all);
        void Blit8(i32 x, i32 y, i32 width, i32 height, byte* data);

        List<byte> pixels;
        u32 width;
        u32 height;
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

    class ProgramAsset {
    public:
        void Load();
        void Free();
        
        bool Create(const char * vertexSource, const char *fragmentSource);

        bool CheckCompilationErrors(u32 shader);
        bool CheckLinkErrors(u32 program);

        void Bind();

        void SetInt(const char* name, i32 value);
        void SetVec4f(const char* name, f32 x, f32 y, f32 z, f32 w);
        void SetVec4f(const char* name, const glm::vec4 &value);
        void SetSampler(const char* name, const u32 textureUnit);
        void SetMat4f(const char* name, const glm::mat4& mat);

        i32 GetUniformLocation(const char* name);

        u32                                 program;
        FixedList<ShaderUniformValue, 16>   uniforms;
    };

    class MeshAsset {
    public:
        static constexpr u32 PNT_STRIDE = (3 + 3 + 2) * sizeof(f32);

        void CreateUnitQuad();
        void CreateMesh(const void* vertexData, u32 vertexCount, u32 vertexStride);
        
        void BindAndDraw();

        u32 vao = 0;
        u32 vbo = 0;
        u32 ibo = 0;
        u32 vertexCount = 0;
        u32 vertexStride = 0;
        u32 indexCount = 0;
    };

    class TextureAsset {
    public:
        i32         width = 0;
        i32         height = 0;
        i32         channels = 4;
        i32         wrapMode = 0x2901; // GL_REPEAT;
        bool        generateMipMaps = false;
        List<byte>  data;
        LargeString path;
    };

    class Texture {
    public:
        void CreateFromAsset(const TextureAsset& textureAsset);
        void CreateFontTexture(i32 width, i32 height, const void* data);
        void CreateRGBA8(i32 width, i32 height, const void* data);

        void Bind(u32 textureUnit);

        i32 width;
        i32 height;
        u32 texture;
    };

    class AudioAsset {
    public:
        i32         channels = 0;
        i32         sampleRate = 0;
        i32         sizeBytes = 0;
        i32         bitDepth = 0;
        List<byte>  data;
    };

    class AudioClip {
    public:
        void CreateFromAsset(const AudioAsset& asset);

        void Play();
        bool IsPlaying();

        void CheckALErrors();

        u32 GetALFormat(u32 numChannels, u32 bitDepth);
        u32 buffer;
        u32 source;
    };

    class TileSheetGenerator {
    public:
        struct Tile {
            u32 width;
            u32 height;
            u32 xPos;
            u32 yPos;
            FixedList<byte, 1024> data;
            glm::vec2 uv0;
            glm::vec2 uv1;
            glm::vec2 boundingUV0;
            glm::vec2 boundingUV1;
        };

        void            AddTile(u32 width, u32 height, void* data);
        void            GenerateTiles(List<byte>& pixels, i32 &width, i32 &height);
        void            GetTileUV(i32 index, glm::vec2& uv0, glm::vec2& uv1);

        List<Tile>      tiles;
        u32             pixelStrideBytes = 4;
    };

    struct Glyph {
        glm::ivec2      size;       // Size of glyph
        glm::ivec2      bearing;    // Offset from baseline to left/top of glyph
        glm::vec2       uv0;
        glm::vec2       uv1;
        u32             advance;    // Horizontal offset to advance to next glyph
    };

    class FontAsset {
    public:
        Texture                 texture;
        TextureAsset            textureAsset;
        FixedList<Glyph, 128>   glyphs;
        i32                     fontSize;
    };

    class BoxBounds {
    public:
        glm::vec2 min;
        glm::vec2 max;

        void Translate(const glm::vec2& translation);
        void CreateFromCenterSize(const glm::vec2& center, const glm::vec2& size);
        bool Intersects(const BoxBounds& other);
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
        void Put(const _type_ &data) {
            const i32 s = storedData.GetNum();
            const i32 size = sizeof(_type_);
            storedData.SetNum(s + size, true);
            std::memcpy(storedData.GetData() + s, (const void *)&data, size);
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

    class AssetLoader {
    public:
        virtual bool Begin() = 0;
        virtual bool LoadTextureAsset(const char * name, TextureAsset& textureAsset) = 0;
        virtual bool LoadAudioAsset(const char* name, AudioAsset& audioAsset) = 0;
        virtual bool LoadFontAsset(const char* name, FontAsset& fontAsset) = 0;
        virtual void End() = 0;
    };

    class LooseAssetLoader : public AssetLoader {
    public:
        virtual bool                Begin() override;
        virtual bool                LoadTextureAsset(const char* name, TextureAsset& textureAsset) override;
        virtual bool                LoadAudioAsset(const char* name, AudioAsset& audioAsset) override;
        virtual bool                LoadFontAsset(const char* name, FontAsset& fontAsset) override;
        virtual void                End() override;

        bool                        LoadWAV(const char* filename, AudioAsset& audioAsset);
        bool                        LoadOGG(const char* filename, AudioAsset& audioAsset);

    private:
        PackedAssetFile             texturePakFile;
        PackedAssetFile             audioPakFile;
        PackedAssetFile             fontPakFile;
    };

    class PackedAssetLoader : public AssetLoader {
        virtual bool                Begin() override;
        virtual bool                LoadTextureAsset(const char* name, TextureAsset& textureAsset) override;
        virtual bool                LoadAudioAsset(const char* name, AudioAsset& audioAsset) override;
        virtual bool                LoadFontAsset(const char* name, FontAsset& fontAsset) override;
        virtual void                End() override;
        
    private:
        PackedAssetFile             texturePakFile;
        PackedAssetFile             audioPakFile;
        PackedAssetFile             fontPakFile;
    };
    
    class LineRenderer {
    public:
        struct LineVertex {
            glm::vec2 position;
            glm::vec4 color;
        };

        void Initialize(u32 maxLines);
        void CreateProgram();
        void CreateBuffer();

        void PushLine(const glm::vec2& start, const glm::vec2& end, const glm::vec4& color);
        void PushBox(const BoxBounds& box, const glm::vec4& color = glm::vec4(0, 0.8f, 0, 1));

        void Draw(const glm::mat4 &projection);

        List<LineVertex>        lines;
        ProgramAsset            program;
        u32                     totalSizeBytes = 0;
        u32                     vao = 0;
        u32                     vbo = 0;
    };
    
    enum class FontPivot {
        BottomLeft,
        Center,
    };

    class FontRenderer {
    public:
        struct FontVertex {
            glm::vec2 position;
            glm::vec2 uv;
        };
        
        struct DrawEntry {
            SmallString text;
            FontPivot pivot;
            BoxBounds bounds;
            glm::vec2 position;
            f32 underlineThinkness;
            f32 underlinePercent;
        };

        enum FontRenderingMode {
            FONT_RENDERING_MODE_TEXT = 0,
            FONT_RENDERING_MODE_GUI = 1
        };

        void                    Initialize();
        void                    CreateProgram();
        void                    CreateBuffer();

        void                    SetFont(FontAsset* fontAsset);
        void                    SetPivot(FontPivot pivot);
        void                    SetUnderLine(f32 thinkness, f32 percent = 1.0f);

        f32                     GetTextWidth(const char* text);
        BoxBounds               GetTextBounds(const char* text);

        BoxBounds               PushText(const char* text, const glm::vec2& position);
        BoxBounds               PushText(const SmallString &text, const glm::vec2& position);
        void                    Draw(u32 screenWidth, u32 screenHeight);

        ProgramAsset            textProgram;
        FontAsset*              currentFont = nullptr;
        FontPivot               currentPivot = FontPivot::BottomLeft;
        f32                     currentUnderlineThinkness = 0.0f;
        f32                     currentUnderlinePercent = 1.0f;
        List<DrawEntry>         entries;
        u32                     totalSizeBytes = 0;
        u32                     vao = 0;
        u32                     vbo = 0;
    };

    class PlayerShip {
    public:
        glm::vec2               position = glm::vec2(0, 0);
        glm::vec2               velocity = glm::vec2(0,0);
        glm::vec2               size = glm::vec2(25);
        f32                     rotation = 0.0f;
        f32                     shootCooldown = 0.0f;
    };

    class Meteor {
    public:
        bool                    isBig = false;
        glm::vec2               position = glm::vec2(0, 0);
        glm::vec2               velocity = glm::vec2(0, 0);
        glm::vec2               size = glm::vec2(25);
        f32                     rotation = 0.0f;
    };

    class Laser {
    public:
        glm::vec2               position = glm::vec2(0, 0);
        glm::vec2               size = glm::vec2(9, 37) * 0.3f;
        f32                     rotation = 0.0f;
        f32                     direction;
    };

    class Star {
    public:
        glm::vec2               position = glm::vec2(0, 0);
        f32                     rotation = 0.0f;
        Texture*                texture = nullptr;
    };

    class SpaceInvaders {
    public:
        void Initialize(WindowData* windowData);
        void UpdateAndRender(WindowData *windowData, FrameInput *input, f32 dt);

        void LoadTexture(const char* filename, Texture* texture, bool generateMipMaps, i32 wrapMode);
        void LoadAudioClip(const char* filename, AudioClip* audioClip);
        void LoadFont(const char* fileaname, FontAsset* fontAsset, u32 fontSize);

        void StartNewGame();

        void SpawnMeteor(bool isBig);
        void SpawnLaser(const glm::vec2& position, f32 direction);

        void SpawnMeteorWave();

        void DrawSprite(glm::vec2 position, glm::vec2 size, f32 rotation, Texture* texture);

        LooseAssetLoader        looseAssetLoader;
        PackedAssetLoader       packedAssetLoader;
        AssetLoader*            assetLoader = nullptr;

        LineRenderer            lineRenderer;
        FontRenderer            fontRenderer;

        PlayerShip              playerShip;
        FixedList<Meteor, 32>   meteors;
        FixedList<Laser, 512>   lasers;
        FixedList<Star, 7>      stars;

        bool                    useRawAssets;

        f32                     worldWidth;
        f32                     worldHeight;
        f32                     waveTimer;
        u32                     difficulty;
        u32                     score;
        bool                    gameOver;
        f32                     menuSelectionUnderlinePercent;
        i32                     menuSelection;

        ProgramAsset            drawShader;
        ProgramAsset            fontShader;
        MeshAsset               unitQuad;
        Texture                 texturePlayerShip;
        Texture                 textureSmallMeteor;
        Texture                 textureLargeMeteor;
        Texture                 textureGreenLaser;
        Texture                 textureBackground;
        Texture                 textureStar1;
        Texture                 textureStar2;
        Texture                 textureStar3;
        
        AudioClip              audioMainMenuMusic;
        AudioClip              audioLaser1;
        AudioClip              audioExplosion1;
        
        FontAsset               fontMain;

        
    };

}
