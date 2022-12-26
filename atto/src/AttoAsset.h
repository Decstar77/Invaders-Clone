#pragma once

#include "AttoLib.h"

namespace atto
{
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
            FixedList<byte, 1024> data;
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

    class AssetId {
    public:
        inline static AssetId Create(const char* str) {
            AssetId id;
            id.id = StringHash::Hash(str);
            return id; 
        }

        inline b8 IsValid() const { return id != 0; }
        inline u32 GetValue() const { return id; }

        inline b8 operator ==(const AssetId& other) const { return id == other.id; }
        inline b8 operator !=(const AssetId& other) const { return id != other.id; }

    private:
        u32 id;
    };

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

    class AudioAsset {
    public:
        i32         channels = 0;
        i32         sampleRate = 0;
        i32         sizeBytes = 0;
        i32         bitDepth = 0;
        List<byte>  data;
    };

    struct SpriteAsset {
        // Texture stuffies
        glm::vec2               uv0;
        glm::vec2               uv1;
        const TextureAsset*     texture;

        // Draw params
        glm::vec2               drawScale; // If texture == null then this is the size of the quad/shape in pixels

        // Animation stuffies
        bool                    animated;
        f32                     animationDuration;
        f32                     animationPlayhead;
        glm::vec2               frameSize;
        i32                     frameCount;
        i32                     frameIndex;

        inline static SpriteAsset Create() {
            SpriteAsset spriteAsset = {};
            spriteAsset.uv1 = glm::vec2(1, 1);
            spriteAsset.drawScale = glm::vec2(1, 1);
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
        
        i32                     fontSize;
        FixedList<Glyph, 128>   glyphs;
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

    enum AssetType {
        ASSET_TYPE_INVALID = 0,
        ASSET_TYPE_TEXTURE,
        ASSET_TYPE_AUDIO,
        ASSET_TYPE_FONT,
        ASSET_TYPE_SPRITE,
        ASSET_TYPE_TILESHEET,
        ASSET_TYPE_COUNT
    };
    
    struct EngineAsset {
        AssetType               type = ASSET_TYPE_INVALID;
        AssetId                 id = {};
        LargeString             path = {};

        union {
            TextureAsset   texture;
            FontAsset      font;
            SpriteAsset    sprite;
        };
    };

    class AssetRegistry {
    public:
        virtual bool                        Initialize(AppState *app) = 0;
        virtual void                        Shutdown() = 0;
        
        const void *                        LoadEngineAsset(const AssetId& id, const AssetType &type);
        const TextureAsset*                 LoadTextureAsset(const AssetId &id);
        const AudioAsset*                   LoadAudioAsset(const AssetId& id);
        const FontAsset*                    LoadFontAsset(const AssetId& id);

        void                                FreeTextureAsset(const AssetId& id);

    protected:
        virtual bool                        LoadTextureAsset(const char* name, TextureAsset& textureAsset) = 0;
        virtual bool                        LoadAudioAsset(const char* name, AudioAsset& audioAsset) = 0;
        virtual bool                        LoadFontAsset(const char* name, FontAsset& fontAsset) = 0;

        u32                                 SubmitTextureR8B8G8A8(i32 width, i32 height, byte* data, i32 wrapMode, bool generateMipMaps);

        FixedList<EngineAsset, 2048>        engineAssets;
    };

    class LooseAssetLoader : public AssetRegistry {
    public:
        virtual bool                Initialize(AppState* app) override;
        virtual void                Shutdown() override;

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
