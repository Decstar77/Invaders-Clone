#pragma once

#include "AttoLib.h"
#include "AttoAsset.h"
#include "AttoLua.h"

namespace atto
{
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

    class Program {
    public:
        void Load();
        void Free();

        bool Create(const char* vertexSource, const char* fragmentSource);

        bool CheckCompilationErrors(u32 shader);
        bool CheckLinkErrors(u32 program);

        void Bind();

        void SetInt(const char* name, i32 value);
        void SetVec4f(const char* name, f32 x, f32 y, f32 z, f32 w);
        void SetVec4f(const char* name, const glm::vec4& value);
        void SetSampler(const char* name, const u32 textureUnit);
        void SetMat4f(const char* name, const glm::mat4& mat);

        i32 GetUniformLocation(const char* name);

        u32                                 program;
        FixedList<ShaderUniformValue, 16>   uniforms;
    };

    class Mesh {
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

    class Texture {
    public:
        void CreateFromAsset(const TextureAsset& textureAsset);
        void CreateFontTexture(i32 width, i32 height, const void* data);
        void CreateRGBA8(i32 width, i32 height, const void* data);

        void Bind(u32 textureUnit);

        i32 width = 0;
        i32 height = 0;
        u32 texture = 0;
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

    class BoxBounds {
    public:
        glm::vec2 min;
        glm::vec2 max;

        void Translate(const glm::vec2& translation);
        void CreateFromCenterSize(const glm::vec2& center, const glm::vec2& size);
        bool Intersects(const BoxBounds& other);
    };

    class LineRenderer {
    public:
        struct LineVertex {
            glm::vec2 position;
            glm::vec4 color;
        };

        void Initialize(u32 maxLines = 1024);
        void CreateProgram();
        void CreateBuffer();

        void PushLine(const glm::vec2& start, const glm::vec2& end, const glm::vec4& color);
        void PushBox(const BoxBounds& box, const glm::vec4& color = glm::vec4(0, 0.8f, 0, 1));

        void Draw(const glm::mat4& projection);

        List<LineVertex>        lines;
        Program                 program;
        u32                     totalSizeBytes = 0;
        u32                     vao = 0;
        u32                     vbo = 0;
    };

    enum class FontPivot {
        BottomLeft = 0,
        Center,
    };

    struct FontDrawEntry {
        LargeString         text;
        const FontAsset*    font;
        FontPivot           pivot = FontPivot::BottomLeft;
        f32                 underlineThinkness = 0.0f;
        f32                 underlinePercent = 1.0f;
        glm::vec2           position;
        BoxBounds           bounds;
    };

    class FontRenderer {
    public:
        struct FontVertex {
            glm::vec2 position;
            glm::vec2 uv;
        };

        enum FontRenderingMode {
            FONT_RENDERING_MODE_TEXT = 0,
            FONT_RENDERING_MODE_GUI = 1
        };

        void                    Initialize();
        void                    CreateProgram();
        void                    CreateBuffer();

        f32                     GetTextWidth(const FontAsset* font, const char* text);
        BoxBounds               GetTextBounds(const FontAsset* font, const char* text);

        BoxBounds               Push(FontDrawEntry& drawEntry);
        void                    Push(const FontAsset* font, const glm::vec2& pos, const char* text);
        void                    Draw(const glm::mat4& projection);

        Program                 textProgram;
        List<FontDrawEntry>     entries;
        u32                     totalSizeBytes = 0;
        u32                     vao = 0;
        u32                     vbo = 0;
    };

    enum SpriteDrawKind {
        SPRITE_DRAW_KIND_SPRITE = 0,
        SPRITE_DRAW_KIND_QUAD = 1,
    };

    struct SpriteDrawEntry {
        SpriteDrawKind          drawKind = SPRITE_DRAW_KIND_SPRITE;
        SpriteAsset             sprite;
        f32                     depth = 0;
        f32                     rotation = 0;
        glm::vec2               position = glm::vec2(0, 0);
        glm::vec2               scale = glm::vec2(1, 1);
        glm::vec4               color = glm::vec4(1, 1, 1, 1);
    };

    class SpriteRenderer {
    public:
        struct SpriteVertex {
            glm::vec2 position;
            glm::vec2 uv;
            glm::vec4 color;
        };

        void                                Initialize();
        void                                CreateProgram();
        void                                CreateBuffer();

        void                                Push(SpriteDrawEntry& drawEntry);
        void                                PushQuad(const glm::vec2& pos, const glm::vec2& size, const glm::vec4& color);
        void                                Draw(const glm::mat4& projection);

        FixedList<SpriteDrawEntry, 1024>    entries;
        Program                             program;
        u32                                 totalSizeBytes = 0;
        u32                                 vao = 0;
        u32                                 vbo = 0;
    };
}


