#include "AttoRendering.h"

#include <string>
#include <sstream>
#include <fstream>

namespace atto
{
    static std::string ReadEntireFile(const char* filePath) {
        std::ifstream file(filePath, std::ios::in);
        if (!file.is_open()) {
            Assert(0, "File is not open");
            return "";
        }

        std::stringstream stream;
        stream << file.rdbuf();
        file.close();
        return stream.str();
    }

    void SoftwareRenderSurface::CreateEmpty(i32 width, i32 height) {
        this->width = width;
        this->height = height;


        pixels.Resize(width * height * 4);
        pixels.SetNum(width * height * 4);
        std::memset(pixels.GetData(), 0, pixels.GetNum());
    }

    void SoftwareRenderSurface::SetPixel(i32 x, i32 y, u8 r, u8 g, u8 b, u8 a) {
        const i32 index = (y * width + x) * 4;

        pixels[index + 0] = b;
        pixels[index + 1] = g;
        pixels[index + 2] = r;
        pixels[index + 3] = a;
    }

    void SoftwareRenderSurface::SetPixelf(i32 x, i32 y, f32 r, f32 g, f32 b, f32 a) {
        SetPixel(x, y, (u8)(r * 255.0f), (u8)(g * 255.0f), (u8)(b * 255.0f), (u8)(a * 255.0f));
    }

    void SoftwareRenderSurface::Blit(i32 x, i32 y, i32 width, i32 height, f32 all) {
        Assert(y + height <= (i32)this->height, "BitmapAsset::DrawQuad: y + height > this->height");
        Assert(x + width <= (i32)this->width, "BitmapAsset::DrawQuad: x + width > this->width");

        const u8 r = (u8)(all * 255.0f);
        for (i32 yIndex = y; yIndex < y + height; yIndex++) {
            for (i32 xIndex = x; xIndex < x + width; xIndex++) {
                SetPixel(xIndex, yIndex, r, r, r, r);
            }
        }
    }

    void SoftwareRenderSurface::Blit8(i32 x, i32 y, i32 w, i32 h, byte* data) {
        Assert(x + w <= (i32)this->width, "BitmapAsset::DrawQuad: x + width > this->width");
        Assert(y + h <= (i32)this->height, "BitmapAsset::DrawQuad: y + height > this->height");

        i32 dy = 0;
        for (i32 yIndex = y; yIndex < y + h; yIndex++, dy++) {
            i32 dx = 0;
            for (i32 xIndex = x; xIndex < x + w; xIndex++, dx++) {
                const i32 index = dy * w + dx;
                const u8 r = data[index];
                SetPixel(xIndex, yIndex, r, r, r, 255);
            }
        }
    }

    void TileSheetGenerator::AddTile(u32 width, u32 height, void* data) {
        Tile& tile = tiles.Alloc();
        tile.width = width;
        tile.height = height;
        const u32 size = width * height;
        Assert(size < (u32)tile.data.GetCapcity(), "");
        tile.data.SetCount(size);
        std::memcpy(tile.data.GetData(), data, size);
    }

    void TileSheetGenerator::GenerateTiles(List<byte>& pixels, i32& outWidth, i32& outHeight) {
        u32 tileWidth = 0;
        u32 tileHeight = 0;

        const i32 tileCount = tiles.GetNum();
        for (i32 tileIndex = 0; tileIndex < tileCount; tileIndex++) {
            Tile& tile = tiles[tileIndex];
            tileWidth = glm::max(tileWidth, tile.width);
            tileHeight = glm::max(tileHeight, tile.height);
        }

        const u32 widthInTiles = (u32)(glm::sqrt((f32)tileCount) + 1);
        const u32 heightInTiles = widthInTiles;
        const u32 widthInPixels = tileWidth * widthInTiles;
        const u32 heightInPixels = tileHeight * heightInTiles;
        const u32 totalSizeBytes = widthInPixels * heightInPixels * pixelStrideBytes;

        f32 xp = 0;
        f32 yp = 0;
        for (i32 tileIndex = 0; tileIndex < tileCount; tileIndex++) {
            Tile& tile = tiles[tileIndex];
            f32 wp = (f32)widthInPixels;
            f32 hp = (f32)heightInPixels;
            tile.uv0.x = xp / wp;
            tile.uv0.y = yp / hp;
            tile.uv1.x = (xp + tile.width) / wp;
            tile.uv1.y = (yp + tile.height) / hp;
            tile.boundingUV0.x = xp / wp;
            tile.boundingUV0.y = yp / hp;
            tile.boundingUV1.x = (xp + tileWidth) / wp;
            tile.boundingUV1.y = (yp + tileHeight) / hp;
            tile.xPos = (u32)xp;
            tile.yPos = (u32)yp;
            xp += tileWidth;
            if (xp >= wp) {
                xp = 0;
                yp += tileHeight;
            }
        }

        pixels.SetNum(totalSizeBytes, true);
        std::memset(pixels.GetData(), 0, totalSizeBytes);
        for (i32 tileIndex = 0; tileIndex < tileCount; tileIndex++) {
            Tile& tile = tiles[tileIndex];

            const i32 x = tile.xPos;
            const i32 y = tile.yPos;
            const i32 w = tile.width;
            const i32 h = tile.height;

            Assert(x + w <= (i32)widthInPixels, "x + width > this->width");
            Assert(y + h <= (i32)heightInPixels, "y + height > this->height");

            i32 dy = 0;
            for (i32 yIndex = y; yIndex < y + h; yIndex++, dy++) {
                i32 dx = 0;
                for (i32 xIndex = x; xIndex < x + w; xIndex++, dx++) {
                    const i32 index = dy * w + dx;
                    const u8 r = tile.data[index];

                    const i32 pixelIndex = (yIndex * widthInPixels + xIndex) * 4;

                    pixels[pixelIndex + 0] = r;
                    pixels[pixelIndex + 1] = r;
                    pixels[pixelIndex + 2] = r;
                    pixels[pixelIndex + 3] = 255;
                }
            }
        }

        outWidth = (i32)widthInPixels;
        outHeight = (i32)heightInPixels;
    }

    void TileSheetGenerator::GetTileUV(i32 index, glm::vec2& uv0, glm::vec2& uv1) {
        uv0 = tiles[index].uv0;
        uv1 = tiles[index].uv1;
    }

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
            ATTOERROR("Bitmap::Write -> Could not open file %s", name);
            return false;
        }

        file.write((char*)&fileHeader, sizeof(FileHeader));
        file.write((char*)&infoHeader, sizeof(InfoHeader));
        file.write((char*)pixels, pixelSize);
        file.close();

        return true;
    }

    //bool LooseAssetLoader::LoadTextureAsset(const char* name, TextureAsset& textureAsset) {

    //    textureAsset.generateMipMaps = false;

    //    void* pixelData = stbi_load(name, &textureAsset.width, &textureAsset.height, &textureAsset.channels, 4);

    //    if (!pixelData) {
    //        ATTOTRACE("Failed to load texture asset %s", name);
    //        return false;
    //    }

    //    glGenTextures(1, &textureAsset.textureHandle);
    //    glBindTexture(GL_TEXTURE_2D, textureAsset.textureHandle);
    //    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureAsset.width, textureAsset.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
    //    if (textureAsset.generateMipMaps) {
    //        glGenerateMipmap(GL_TEXTURE_2D);
    //    }

    //    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, textureAsset.wrapMode);
    //    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, textureAsset.wrapMode);
    //    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, textureAsset.generateMipMaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    //    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    //    glBindTexture(GL_TEXTURE_2D, 0);

    //    stbi_image_free(pixelData);

    //    return true;
    //}

    //bool LooseAssetLoader::LoadAudioAsset(const char* name, AudioAsset& audioAsset) {
    //    LargeString filename = LargeString::FromLiteral(name);
    //    if (filename.Contains(".ogg")) {
    //        if (LoadOGG(filename.GetCStr(), audioAsset)) {
    //            return true;
    //        }
    //    }
    //    else if (filename.Contains(".wav")) {
    //        if (LoadWAV(filename.GetCStr(), audioAsset)) {
    //            return true;
    //        }
    //    }
    //    else {
    //        ATTOTRACE("Unsupported audio file type: %s", filename.GetCStr());
    //    }

    //    return false;
    //}

    //bool LooseAssetLoader::LoadFontAsset(const char* filename, FontAsset& fontAsset) {
    //    FT_Library ft = {};
    //    if (FT_Init_FreeType(&ft)) {
    //        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
    //        return false;
    //    }

    //    FT_Face face;
    //    if (FT_New_Face(ft, filename, 0, &face)) {
    //        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
    //        return false;
    //    }

    //    FT_Set_Pixel_Sizes(face, 0, fontAsset.fontSize);

    //    TileSheetGenerator tileSheet = {};

    //    for (unsigned char c = 0; c < 128; c++) {
    //        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
    //            std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
    //            continue;
    //        }

    //        Glyph character = {};
    //        character.size = glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows);
    //        character.bearing = glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top);
    //        character.advance = face->glyph->advance.x;

    //        tileSheet.AddTile(character.size.x, character.size.y, face->glyph->bitmap.buffer);

    //        fontAsset.glyphs.Add(character);
    //    }

    //    List<byte> pixels;
    //    tileSheet.GenerateTiles(pixels, fontAsset.width, fontAsset.height);
    //    for (byte c = 0; c < 128; c++) {
    //        tileSheet.GetTileUV(c, fontAsset.glyphs[c].uv0, fontAsset.glyphs[c].uv1);
    //    }

    //    fontAsset.textureHandle = SubmitTextureR8B8G8A8(fontAsset.width, fontAsset.height, pixels.GetData(), GL_REPEAT, true);

    //    //Bitmap::Write(fontAsset.textureAsset.data.GetData(), fontAsset.textureAsset.width, fontAsset.textureAsset.height, "fontyboi.bmp");

    //    FT_Done_Face(face);
    //    FT_Done_FreeType(ft);

    //    return true;
    //}

    //bool LooseAssetLoader::LoadWAV(const char* filename, AudioAsset& audioAsset) {
    //    AudioFile<f32> audioFile;
    //    bool loaded = audioFile.load(filename);
    //    if (!loaded) {
    //        std::cout << "Failed to load audio file " << filename << std::endl;
    //        return false;
    //    }

    //    audioAsset.bitDepth = audioFile.getBitDepth();
    //    audioAsset.channels = audioFile.getNumChannels();
    //    audioAsset.sampleRate = audioFile.getSampleRate();

    //    std::vector<u8> loadedData;
    //    audioFile.savePCMToBuffer(loadedData);

    //    audioAsset.sizeBytes = (i32)loadedData.size();

    //    audioAsset.bufferHandle = SubmitAudioClip(audioAsset.sizeBytes, loadedData.data(), audioAsset.channels, audioAsset.bitDepth, audioAsset.sampleRate);
    //    
    //    return true;
    //}

    //bool LooseAssetLoader::LoadOGG(const char* filename, AudioAsset& audioAsset) {
    //    audioAsset.channels = 0;
    //    audioAsset.sampleRate = 0;
    //    audioAsset.bitDepth = 16;
    //    i16* loadedData = nullptr;
    //    i32 decoded = stb_vorbis_decode_filename(filename, &audioAsset.channels, &audioAsset.sampleRate, &loadedData);
    //    if (loadedData == nullptr) {
    //        std::cout << "Failed to load audio file " << filename << std::endl;
    //        return false;
    //    }

    //    audioAsset.sizeBytes = decoded * audioAsset.channels * sizeof(i16);

    //    audioAsset.bufferHandle = SubmitAudioClip(audioAsset.sizeBytes, (byte*)loadedData, audioAsset.channels, audioAsset.bitDepth, audioAsset.sampleRate);
    //    
    //    return true;
    //}

}



