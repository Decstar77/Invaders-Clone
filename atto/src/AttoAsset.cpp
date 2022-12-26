#include "SpaceInvaders.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/std_image.h>

#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis/stb_vorbis.c"

#include <audio/AudioFile.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <glad/glad.h>

#include <filesystem>

namespace atto {
    bool Bitmap::Write(byte* pixels, u32 width, u32 height, const char* name) {
        const u32 pixelSize = width * height * 4;

        FileHeader fileHeader = {};
        fileHeader.type[0] = 'B';
        fileHeader.type[1] = 'M';
        fileHeader.size = sizeof(FileHeader) + sizeof(InfoHeader) + pixelSize;
        fileHeader.offset = sizeof(FileHeader) + sizeof(InfoHeader);

        InfoHeader infoHeader = {};
        infoHeader.size = sizeof(InfoHeader);
        infoHeader.width = width;
        infoHeader.height = height;
        infoHeader.planes = 1;
        infoHeader.bitCount = 32;
        infoHeader.compression = 0;
        infoHeader.sizeImage = 0;
        infoHeader.xPelsPerMeter = 3200;
        infoHeader.yPelsPerMeter = 3200;
        infoHeader.clrUsed = 0;
        infoHeader.clrImportant = 0;

        std::ofstream file(name, std::ios::binary);
        if (!file.is_open()) {
            std::cout << "Bitmap::Write -> Could not open file " << name << std::endl;
            return false;
        }

        file.write((char*)&fileHeader, sizeof(FileHeader));
        file.write((char*)&infoHeader, sizeof(InfoHeader));
        file.write((char*)pixels, pixelSize);
        file.close();

        return true;
    }

    static void FindAllFiles(const char* path, const char* extension, List<LargeString> &files) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
            if (entry.path().extension() == extension) {
                files.Add(entry.path().string().c_str());
            }
        }
    }

    const void* AssetRegistry::LoadEngineAsset(const AssetId& id, const AssetType& type) {
        const i32 count = engineAssets.GetCount();
        for (i32 assetIndex = 0; assetIndex < count; ++assetIndex) {
            EngineAsset& asset = engineAssets[assetIndex];

            if (!asset.id.IsValid()) {
                break;
            }

            if (asset.id == id && asset.type == type) {
                switch (asset.type) {
                    case ASSET_TYPE_TEXTURE: {
                        if (asset.texture.textureHandle != 0) {
                            return &asset;
                        }
                        
                        if (LoadTextureAsset(asset.path.GetCStr(), asset.texture)) {
                            ATTOTRACE("Loaded texture asset %s", asset.path.GetCStr());
                            return &asset;
                        }
                    } break;
                    
                    default: {
                        Assert(0, "");
                    }
                }

                
            }
        }

        return nullptr;
    }

    const TextureAsset* AssetRegistry::LoadTextureAsset(const AssetId& id) {
        return (const TextureAsset*)LoadEngineAsset(id, ASSET_TYPE_TEXTURE);
    }
    
    const FontAsset* AssetRegistry::LoadFontAsset(const AssetId& id) {
        return (const FontAsset*)LoadEngineAsset(id, ASSET_TYPE_FONT);
    }

    u32 AssetRegistry::SubmitTextureR8B8G8A8(i32 width, i32 height, byte* data, i32 wrapMode, bool generateMipMaps) {
        u32 textureHandle = 0;
        glGenTextures(1, &textureHandle);
        glBindTexture(GL_TEXTURE_2D, textureHandle);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        if (generateMipMaps) {
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, generateMipMaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0);

        return textureHandle;
    }

    void AssetRegistry::FreeTextureAsset(const AssetId& id) {
        Assert(0, "TODO");
    }

    bool LooseAssetLoader::Initialize(AppState* app) {
        List<LargeString> texturePaths;
        FindAllFiles(app->looseAssetPath.GetCStr(), ".png", texturePaths);
        
        List<LargeString> audioPaths;
        FindAllFiles(app->looseAssetPath.GetCStr(), ".ogg", audioPaths);
        FindAllFiles(app->looseAssetPath.GetCStr(), ".wav", audioPaths);

        List<LargeString> fontPaths;
        FindAllFiles(app->looseAssetPath.GetCStr(), ".ttf", fontPaths);

        const i32 textureCount = texturePaths.GetNum();
        for (i32 texturePathIndex = 0; texturePathIndex < textureCount; ++texturePathIndex) {
            EngineAsset asset = {};
            asset.type = ASSET_TYPE_TEXTURE;
            
            LargeString path = texturePaths[texturePathIndex];
            path.BackSlashesToSlashes();
            asset.path = path;
            path.StripFileExtension();
            asset.id = AssetId::Create(path.GetCStr());

            asset.texture = TextureAsset::CreateDefault();
            
            engineAssets.Add(asset);

            ATTOTRACE("Found texture asset: %s", path.GetCStr());
        }

        return true;
    }

    bool LooseAssetLoader::LoadTextureAsset(const char* name, TextureAsset& textureAsset) {

        void* pixelData = stbi_load(name, &textureAsset.width, &textureAsset.height, &textureAsset.channels, 4);

        if (!pixelData) {
            ATTOTRACE("Failed to load texture asset %s", name);
            return false;
        }

        glGenTextures(1, &textureAsset.textureHandle);
        glBindTexture(GL_TEXTURE_2D, textureAsset.textureHandle);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureAsset.width, textureAsset.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
        if (textureAsset.generateMipMaps) {
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, textureAsset.wrapMode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, textureAsset.wrapMode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, textureAsset.generateMipMaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0);

        stbi_image_free(pixelData);

        return true;
    }

    bool LooseAssetLoader::LoadAudioAsset(const char* name, AudioAsset& audioAsset) {
        LargeString filename = name;
        if (filename.Contains(".ogg")) {
            if (LoadOGG(filename.GetCStr(), audioAsset)) {
                audioPakFile.SerializeAsset(audioAsset);
                return true;
            }
        }
        else if (filename.Contains(".wav")) {
            if (LoadWAV(filename.GetCStr(), audioAsset)) {
                audioPakFile.SerializeAsset(audioAsset);
                return true;
            }
        }
        else {

        }

        return false;
    }

    bool LooseAssetLoader::LoadFontAsset(const char* filename, FontAsset& fontAsset) {
        FT_Library ft = {};
        if (FT_Init_FreeType(&ft)) {
            std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
            return false;
        }

        FT_Face face;
        if (FT_New_Face(ft, filename, 0, &face)) {
            std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
            return false;
        }

        FT_Set_Pixel_Sizes(face, 0, fontAsset.fontSize);

        TileSheetGenerator tileSheet = {};

        for (unsigned char c = 0; c < 128; c++) {
            if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
                std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
                continue;
            }

            Glyph character = {};
            character.size = glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows);
            character.bearing = glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top);
            character.advance = face->glyph->advance.x;

            tileSheet.AddTile(character.size.x, character.size.y, face->glyph->bitmap.buffer);

            fontAsset.glyphs.Add(character);
        }

        List<byte> pixels;
        tileSheet.GenerateTiles(pixels, fontAsset.width, fontAsset.height);
        for (byte c = 0; c < 128; c++) {
            tileSheet.GetTileUV(c, fontAsset.glyphs[c].uv0, fontAsset.glyphs[c].uv1);
        }

        fontAsset.textureHandle = SubmitTextureR8B8G8A8(fontAsset.width, fontAsset.height, pixels.GetData(), GL_REPEAT, true);

        //Bitmap::Write(fontAsset.textureAsset.data.GetData(), fontAsset.textureAsset.width, fontAsset.textureAsset.height, "fontyboi.bmp");

        FT_Done_Face(face);
        FT_Done_FreeType(ft);

        return true;
    }

    bool LooseAssetLoader::LoadWAV(const char* filename, AudioAsset& audioAsset) {
        AudioFile<f32> audioFile;
        bool loaded = audioFile.load(filename);
        if (!loaded) {
            std::cout << "Failed to load audio file " << filename << std::endl;
            return false;
        }

        audioAsset.bitDepth = audioFile.getBitDepth();
        audioAsset.channels = audioFile.getNumChannels();
        audioAsset.sampleRate = audioFile.getSampleRate();

        std::vector<u8> loadedData;
        audioFile.savePCMToBuffer(loadedData);

        audioAsset.sizeBytes = (i32)loadedData.size();

        audioAsset.data.SetNum(audioAsset.sizeBytes, true);
        std::memcpy(audioAsset.data.GetData(), loadedData.data(), audioAsset.sizeBytes);

        return true;
    }

    bool LooseAssetLoader::LoadOGG(const char* filename, AudioAsset& audioAsset) {
        audioAsset.channels = 0;
        audioAsset.sampleRate = 0;
        audioAsset.bitDepth = 16;
        i16* loadedData = nullptr;
        i32 decoded = stb_vorbis_decode_filename(filename, &audioAsset.channels, &audioAsset.sampleRate, &loadedData);
        if (loadedData == nullptr) {
            std::cout << "Failed to load audio file " << filename << std::endl;
            return false;
        }

        audioAsset.sizeBytes = decoded * audioAsset.channels * sizeof(i16);

        audioAsset.data.SetNum(audioAsset.sizeBytes, true);
        memcpy(audioAsset.data.GetData(), loadedData, audioAsset.sizeBytes);

        return true;
    }

    void LooseAssetLoader::Shutdown() {
        texturePakFile.Save("assets/pixel_bois.pak");
        audioPakFile.Save("assets/noisy_bois.pak");
        fontPakFile.Save("assets/scribly_bois.pak");

        texturePakFile.Finished();
        audioPakFile.Finished();
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
        Serialize(textureAsset.wrapMode);
        Serialize(textureAsset.generateMipMaps);
        //Serialize(textureAsset.data);
    }

    void PackedAssetFile::SerializeAsset(AudioAsset& audioAsset) {
        Serialize(audioAsset.channels);
        Serialize(audioAsset.sampleRate);
        Serialize(audioAsset.sizeBytes);
        Serialize(audioAsset.bitDepth);
        Serialize(audioAsset.data);
    }

    void PackedAssetFile::SerializeAsset(FontAsset& fontAsset) {
        Serialize(fontAsset.fontSize);
        //SerializeAsset(fontAsset.textureAsset);
        Serialize(fontAsset.glyphs);
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

    //bool PackedAssetLoader::Begin() {
    //    texturePakFile.Load("assets/pixel_bois.pak");
    //    audioPakFile.Load("assets/noisy_bois.pak");
    //    fontPakFile.Load("assets/scribly_bois.pak");
    //    return true;
    //}
    //
    //bool PackedAssetLoader::LoadTextureAsset(const char* name, TextureAsset& textureAsset) {
    //    texturePakFile.SerializeAsset(textureAsset);
    //    return true;
    //}
    //bool PackedAssetLoader::LoadAudioAsset(const char* name, AudioAsset& audioAsset) {
    //    audioPakFile.SerializeAsset(audioAsset);
    //    return true;
    //}
    //bool PackedAssetLoader::LoadFontAsset(const char* name, FontAsset& fontAsset) {
    //    fontPakFile.SerializeAsset(fontAsset);
    //    return true;
    //}
    //void PackedAssetLoader::End() {
    //    texturePakFile.Finished();
    //    audioPakFile.Finished();
    //    fontPakFile.Finished();
    //}
    
}

