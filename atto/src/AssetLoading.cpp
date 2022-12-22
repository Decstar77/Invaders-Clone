#include "SpaceInvaders.h"

#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image/std_image.h"

#define STB_VORBIS_HEADER_ONLY
#include "vendor/stb_vorbis/stb_vorbis.c"
#include <audio/AudioFile.h>


#include <ft2build.h>
#include FT_FREETYPE_H

namespace atto {

    bool LooseAssetLoader::Begin() {
        return true;
    }

    bool LooseAssetLoader::LoadTextureAsset(const char* name, TextureAsset& textureAsset) {
        void* pixelData = stbi_load(name, &textureAsset.width, &textureAsset.height, &textureAsset.channels, 4);

        if (!pixelData) {
            std::cout << "Failed to load texture " << name << std::endl;
            return false;
        }

        textureAsset.data.SetNum(textureAsset.width * textureAsset.height * 4, true);
        memcpy(textureAsset.data.GetData(), pixelData, textureAsset.width * textureAsset.height * 4);

        stbi_image_free(pixelData);

        texturePakFile.SerializeAsset(textureAsset);

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

        fontAsset.textureAsset.channels = 4;
        fontAsset.textureAsset.generateMipMaps = false;
        fontAsset.textureAsset.wrapMode = 0x2901; // GL_REPEAT;
        tileSheet.GenerateTiles(fontAsset.textureAsset.data, fontAsset.textureAsset.width, fontAsset.textureAsset.height);
        for (byte c = 0; c < 128; c++) {
            tileSheet.GetTileUV(c, fontAsset.glyphs[c].uv0, fontAsset.glyphs[c].uv1);
        }

        //Bitmap::Write(fontAsset.textureAsset.data.GetData(), fontAsset.textureAsset.width, fontAsset.textureAsset.height, "fontyboi.bmp");

        fontAsset.texture.CreateFromAsset(fontAsset.textureAsset);

        FT_Done_Face(face);
        FT_Done_FreeType(ft);

        fontPakFile.SerializeAsset(fontAsset);

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

    void LooseAssetLoader::End() {
        texturePakFile.Save("assets/pixel_bois.pak");
        audioPakFile.Save("assets/noisy_bois.pak");
        fontPakFile.Save("assets/scribly_bois.pak");

        texturePakFile.Finished();
        audioPakFile.Finished();
    }

    bool PackedAssetLoader::Begin() {
        texturePakFile.Load("assets/pixel_bois.pak");
        audioPakFile.Load("assets/noisy_bois.pak");
        fontPakFile.Load("assets/scribly_bois.pak");
        return true;
    }
    
    bool PackedAssetLoader::LoadTextureAsset(const char* name, TextureAsset& textureAsset) {
        texturePakFile.SerializeAsset(textureAsset);
        return true;
    }
    bool PackedAssetLoader::LoadAudioAsset(const char* name, AudioAsset& audioAsset) {
        audioPakFile.SerializeAsset(audioAsset);
        return true;
    }
    bool PackedAssetLoader::LoadFontAsset(const char* name, FontAsset& fontAsset) {
        fontPakFile.SerializeAsset(fontAsset);
        fontAsset.texture.CreateFromAsset(fontAsset.textureAsset);
        return true;
    }
    void PackedAssetLoader::End() {
        texturePakFile.Finished();
        audioPakFile.Finished();
        fontPakFile.Finished();
    }
}

