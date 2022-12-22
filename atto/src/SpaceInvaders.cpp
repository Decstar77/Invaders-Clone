#include "SpaceInvaders.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <random>
#include <string>
#include <stdarg.h> 

#include <al/alc.h>
#include <al/al.h>

#include <glad/glad.h>

#if ATTO_DEBUG
#define ATTOINFO(msg) std::cout << "INFO: " << msg << std::endl
#else 
#define ATTOINFO(msg)
#endif
namespace atto {

    template<typename T>
    T PickRandom(T&& arg) {
        return arg;
    }

    template<typename _type_, typename... Args>
    auto PickRandom(_type_&& arg, Args&&... args) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, sizeof...(args));
        int random_index = dis(gen);
        if (random_index == 0) {
            return arg;
        }
        else {
            return PickRandom(std::forward<Args>(args)...);
        }
    }

    f32 Lerp(const f32 a, const f32 b, const f32 t) {
        return a + (b - a) * t;
    }

    void StringFormatV(char* dest, size_t size, const char* format, va_list va_listp) {
        vsnprintf(dest, size, format, va_listp);
    }

    SmallString StringBuilder::FormatSmall(const char* format, ...) {
        SmallString result;

        va_list arg_ptr;
        va_start(arg_ptr, format);
        StringFormatV(result.GetCStr(), result.CAPCITY, format, arg_ptr);
        va_end(arg_ptr);

        result.CalculateLength();

        return result;
    }

    LargeString StringBuilder::FormatLarge(const char* format, ...) {
        LargeString result;

        va_list arg_ptr;
        va_start(arg_ptr, format);
        StringFormatV(result.GetCStr(), result.CAPCITY, format, arg_ptr);
        va_end(arg_ptr);

        result.CalculateLength();

        return result;
    }

    f32 RandomFloat(f32 min, f32 max) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<f32> dis(min, max);
        return dis(gen);
    }

    std::string ReadEntireFile(const char* filePath) {
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

    void ProgramAsset::Load() {
        std::string vertexSource = ReadEntireFile("assets/drawShader.vs.glsl");
        std::string fragmentSource = ReadEntireFile("assets/drawShader.fs.glsl");
        Create(vertexSource.c_str(), fragmentSource.c_str());
    }

    void ProgramAsset::Free() {
        glDeleteProgram(program);
    }

    bool ProgramAsset::Create(const char* vertexSource, const char* fragmentSource) {
        u32 vertexShader;
        vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexSource, NULL);
        glCompileShader(vertexShader);
        if (!CheckCompilationErrors(vertexShader)) {
            return false;
        }

        u32 fragmentShader;
        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
        glCompileShader(fragmentShader);
        if (!CheckCompilationErrors(fragmentShader)) {
            return false;
        }

        program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);
        if (!CheckLinkErrors(program)) {
            return false;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return true;
    }

    bool ProgramAsset::CheckCompilationErrors(u32 shader) {
        i32 success;
        char infoLog[1024];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }

        return success;
    }

    bool ProgramAsset::CheckLinkErrors(u32 program) {
        i32 success;
        char infoLog[1024];
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(program, 1024, NULL, infoLog);
            std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }

        return success;
    }

    void ProgramAsset::Bind() {
        glUseProgram(program);
    }

    void ProgramAsset::SetInt(const char* name, i32 value) {
        i32 location = GetUniformLocation(name);
        if (location >= 0) {
            glProgramUniform1i(program, location, value);
        }
    }

    void ProgramAsset::SetVec4f(const char* name, f32 x, f32 y, f32 z, f32 w) {
        i32 location = GetUniformLocation(name);
        if (location >= 0) {
            glProgramUniform4f(program, location, x, y, z, w);
        }
    }

    void ProgramAsset::SetVec4f(const char* name, const glm::vec4& value) {
        i32 location = GetUniformLocation(name);
        if (location >= 0) {
            glProgramUniform4f(program, location, value.x, value.y, value.z, value.w);
        }
    }

    void ProgramAsset::SetSampler(const char* name, const u32 textureUnit) {
        i32 location = GetUniformLocation(name);
        if (location >= 0) {
            glProgramUniform1i(program, location, textureUnit);
        }
    }

    void ProgramAsset::SetMat4f(const char* name, const glm::mat4& mat) {
        i32 location = GetUniformLocation(name);
        if (location >= 0) {
            glProgramUniformMatrix4fv(program, location, 1, false, &mat[0][0]);
        }
    }

    i32 ProgramAsset::GetUniformLocation(const char* name) {
        const u32 uniformCount = uniforms.GetCount();
        for (u32 uniformIndex = 0; uniformIndex < uniformCount; uniformIndex++) {
            ShaderUniformValue& uniform = uniforms[uniformIndex];
            if (uniform.name == name) {
                return uniform.location;
            }
        }

        i32 location = glGetUniformLocation(program, name);
        if (location >= 0) {
            ShaderUniformValue newUniform = {};
            newUniform.location = location;
            newUniform.name = name;

            uniforms.Add(newUniform);
        }
        else {
            std::cout << "Could not find uniform value " << name << std::endl;
        }

        return location;
    }

    void MeshAsset::CreateUnitQuad() {
        const f32 quadVertices[] = {
           -1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
           -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
            1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
           -1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
            1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
            1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f
        };

        CreateMesh(quadVertices, 6, PNT_STRIDE);
    }

    void MeshAsset::CreateMesh(const void* vertexData, u32 vertexCount, u32 vertexStride) {
        const u32 vertexSizeBytes = vertexCount * vertexStride;

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertexSizeBytes, vertexData, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        glVertexAttribPointer(0, 3, GL_FLOAT, false, vertexStride, 0);
        glVertexAttribPointer(1, 3, GL_FLOAT, false, vertexStride, (void*)(3 * sizeof(f32)));
        glVertexAttribPointer(2, 2, GL_FLOAT, false, vertexStride, (void*)((3 + 3) * sizeof(f32)));

        glBindVertexArray(0);

        this->vertexStride = vertexStride;
        this->vertexCount = vertexCount;
    }

    void MeshAsset::BindAndDraw() {
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    }

    void Texture::CreateFromAsset(const TextureAsset& textureAsset) {
        this->width = textureAsset.width;
        this->height = textureAsset.height;

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureAsset.width, textureAsset.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureAsset.data.GetData());
        if (textureAsset.generateMipMaps) {
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, textureAsset.wrapMode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, textureAsset.wrapMode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, textureAsset.generateMipMaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void Texture::CreateFontTexture(i32 width, i32 height, const void* data) {
        this->width = width;
        this->height = height;

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);

        // set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void Texture::CreateRGBA8(i32 width, i32 height, const void* data) {
        this->width = width;
        this->height = height;

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void Texture::Bind(u32 textureUnit) {
        glBindTextureUnit(textureUnit, texture);
    }

    void AudioClip::CreateFromAsset(const AudioAsset& asset) {
        u32 alFormat = GetALFormat(asset.channels, asset.bitDepth);

        alGenBuffers(1, &buffer);
        alBufferData(buffer, alFormat, asset.data.GetData(), (ALsizei)asset.sizeBytes, (ALsizei)asset.sampleRate);

        alGenSources(1, &source);
        alSourcei(source, AL_BUFFER, buffer);
    }

    void AudioClip::Play() {
        alSourcePlay(source);
    }

    bool AudioClip::IsPlaying() {
        ALenum state = {};
        alGetSourcei(source, AL_SOURCE_STATE, &state);
        return state == AL_PLAYING;
    }

    void AudioClip::CheckALErrors() {
        ALCenum error = alGetError();
        if (error != AL_NO_ERROR) {
            switch (error)
            {
            case AL_INVALID_NAME:
                std::cerr << "AL_INVALID_NAME: a bad name (ID) was passed to an OpenAL function";
                break;
            case AL_INVALID_ENUM:
                std::cerr << "AL_INVALID_ENUM: an invalid enum value was passed to an OpenAL function";
                break;
            case AL_INVALID_VALUE:
                std::cerr << "AL_INVALID_VALUE: an invalid value was passed to an OpenAL function";
                break;
            case AL_INVALID_OPERATION:
                std::cerr << "AL_INVALID_OPERATION: the requested operation is not valid";
                break;
            case AL_OUT_OF_MEMORY:
                std::cerr << "AL_OUT_OF_MEMORY: the requested operation resulted in OpenAL running out of memory";
                break;
            default:
                std::cerr << "UNKNOWN AL ERROR: " << error;
            }
            std::cerr << std::endl;
        }
    }

    u32 AudioClip::GetALFormat(u32 numChannels, u32 bitDepth) {
        b8 sterio = numChannels > 1;
        if (sterio) {
            if (bitDepth == 8) {
                return AL_FORMAT_STEREO8;
            }
            else if (bitDepth == 16) {
                return AL_FORMAT_STEREO16;
            }
        }
        else {
            if (bitDepth == 8) {
                return AL_FORMAT_MONO8;
            }
            else if (bitDepth == 16) {
                return AL_FORMAT_MONO16;
            }
        }

        std::cout << "Unsupported audio format" << std::endl;
        return 0;
    }

    void TileSheetGenerator::AddTile(u32 width, u32 height, void* data) {
        Tile& tile = tiles.Alloc();
        tile.width = width;
        tile.height = height;
        const u32 size = width * height;
        Assert(size < tile.data.GetCapcity(), "");
        tile.data.SetCount(size);
        std::memcpy(tile.data.GetData(), data, size);
    }

    void TileSheetGenerator::GenerateTiles(List<byte> &pixels, i32& outWidth, i32& outHeight) {
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

    void BoxBounds::Translate(const glm::vec2& translation) {
        min += translation;
        max += translation;
    }

    void BoxBounds::CreateFromCenterSize(const glm::vec2& center, const glm::vec2& size) {
        max = center + size * 0.5f;
        min = center - size * 0.5f;
    }

    bool BoxBounds::Intersects(const BoxBounds& other) {
        return (max.x >= other.min.x && min.x <= other.max.x) &&
            (max.y >= other.min.y && min.y <= other.max.y);
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
        Serialize(textureAsset.data);
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
        SerializeAsset(fontAsset.textureAsset);
        Serialize(fontAsset.glyphs);
    }

    void PackedAssetFile::Reset() {
        currentOffset = 0;
    }

    bool PackedAssetFile::Save(const char* name) {
        std::ofstream file(name, std::ios::binary);
        if (!file.is_open()) {
            std::cout << "PackedAssetFile::Save -> Could not open file " << name << std::endl;
            return false;
        }

        file.write((char*)storedData.GetData(), storedData.GetNum());
        file.close();

        return true;
    }

    bool PackedAssetFile::Load(const char* name) {
        std::ifstream file(name, std::ios::binary);
        if (!file.is_open()) {
            std::cout << "PackedAssetFile::Load -> Could not open file " << name << std::endl;
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

    void LineRenderer::Initialize(u32 maxLines) {
        totalSizeBytes = maxLines * sizeof(LineVertex);
        CreateProgram();
        CreateBuffer();
    }

    void LineRenderer::CreateProgram() {
        const char* vertexShaderSource = R"(
            #version 330 core

            layout (location = 0) in vec2 position;
            layout (location = 1) in vec4 color;

            out vec4 vertexColor;

            uniform mat4 p;

            void main() {
                gl_Position = p * vec4(position.x, position.y, 0.0, 1.0);
                vertexColor = color;
            }
        )";

        const char* fragmentShaderSource = R"(
            #version 330 core
            out vec4 FragColor;

            in vec4 vertexColor;

            void main() {
                FragColor = vertexColor;
            }
        )";

        program.Create(vertexShaderSource, fragmentShaderSource);
    }

    void LineRenderer::CreateBuffer() {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, totalSizeBytes, nullptr, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        const u32 vertexStride = sizeof(LineVertex);

        glVertexAttribPointer(0, 2, GL_FLOAT, false, vertexStride, 0);
        glVertexAttribPointer(1, 4, GL_FLOAT, false, vertexStride, (void*)(2 * sizeof(f32)));

        glBindVertexArray(0);
    }

    void LineRenderer::PushLine(const glm::vec2& start, const glm::vec2& end, const glm::vec4& color) {
        const i32 newCount = lines.GetNum() + 2;
        if (newCount * sizeof(LineVertex) > totalSizeBytes) {
            ATTOINFO("To many debug lines!!");
            return;
        }

        LineVertex v1;
        v1.position = start;
        v1.color = color;

        LineVertex v2;
        v2.position = end;
        v2.color = color;

        lines.Add(v1);
        lines.Add(v2);
    }

    void LineRenderer::PushBox(const BoxBounds& box, const glm::vec4& color) {
        glm::vec2 v1 = box.min;
        glm::vec2 v2 = box.max;
        glm::vec2 v3 = glm::vec2(box.min.x, box.max.y);
        glm::vec2 v4 = glm::vec2(box.max.x, box.min.y);
        PushLine(v1, v3, color);
        PushLine(v1, v4, color);
        PushLine(v2, v3, color);
        PushLine(v2, v4, color);
    }

    void LineRenderer::Draw(const glm::mat4& projection) {
        const u32 vertexCount = lines.GetNum();
        const u32 vertexSize = vertexCount * sizeof(LineVertex);

        if (vertexCount == 0) {
            return;
        }

        glNamedBufferSubData(vbo, 0, vertexSize, lines.GetData());

        program.Bind();
        program.SetMat4f("p", projection);
        glBindVertexArray(vao);
        glDrawArrays(GL_LINES, 0, vertexCount);

        lines.Clear();
    }

    void FontRenderer::Initialize() {
        totalSizeBytes = 6 * 4 * sizeof(f32);
        CreateProgram();
        CreateBuffer();
    }

    void FontRenderer::CreateProgram() {
        const char* vertexShaderSource = R"(
            #version 330 core

            layout (location = 0) in vec2 position;
            layout (location = 1) in vec2 texCoord;

            out vec2 vertexTexCoord;

            uniform mat4 p;

            void main() {
                vertexTexCoord = texCoord;
                gl_Position = p * vec4(position.x, position.y, 0.0, 1.0);
            }
        )";

        const char* fragmentShaderSource = R"(
            #version 330 core
            out vec4 FragColor;

            in vec2 vertexTexCoord;

            uniform int mode;
            uniform vec4 color;
            uniform sampler2D texture0;

            void main() {
                if (mode == 0) { // Text rendering
                    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(texture0, vertexTexCoord).r);
                    FragColor = vec4(1.0) * sampled;
                } else if (mode == 1 ) {
                    FragColor = color;
                }
            }
        )";

        textProgram.Create(vertexShaderSource, fragmentShaderSource);
    }

    void FontRenderer::CreateBuffer() {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, totalSizeBytes, nullptr, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        const u32 vertexStride = sizeof(FontVertex);

        glVertexAttribPointer(0, 2, GL_FLOAT, false, vertexStride, 0);
        glVertexAttribPointer(1, 2, GL_FLOAT, false, vertexStride, (void*)(2 * sizeof(f32)));

        glBindVertexArray(0);
    }

    void FontRenderer::SetFont(FontAsset* fontAsset) {
        this->currentFont = fontAsset;
    }

    void FontRenderer::SetPivot(FontPivot pivot) {
        this->currentPivot = pivot;
    }

    void FontRenderer::SetUnderLine(f32 thickness, f32 percent) {
        this->currentUnderlineThinkness = thickness;
        this->currentUnderlinePercent = percent;
    }

    f32 FontRenderer::GetTextWidth(const char* text) {
        Assert(currentFont, "No font set");

        f32 width = 0;
        for (i32 i = 0; text[i] != '\0'; i++) {
            i32 index = (i32)text[i];
            Glyph& ch = currentFont->glyphs[index];
            // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
            // bit shift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
            width += (ch.advance >> 6);
        }

        return width;
    }

    BoxBounds FontRenderer::GetTextBounds(const char* text) {
        Assert(currentFont, "No font set");

        BoxBounds bounds = {};

        f32 x = 0.0f;
        for (i32 i = 0; text[i] != '\0'; i++) {
            i32 index = (i32)text[i];
            Glyph& ch = currentFont->glyphs[index];

            f32 xpos = (f32)ch.bearing.x;
            f32 ypos = (f32)(-(ch.size.y - ch.bearing.y));
            f32 w = (f32)ch.size.x;
            f32 h = (f32)ch.size.y;


            bounds.min.y = glm::min(bounds.min.y, ypos);
            bounds.max.y = glm::max(bounds.max.y, ypos + h);

            x += (ch.advance >> 6);
        }

        bounds.max.x = x;

        return bounds;
    }

    BoxBounds FontRenderer::PushText(const char* text, const glm::vec2& position) {
        DrawEntry entry = {};
        entry.text = text;
        entry.position = position;
        entry.pivot = currentPivot;
        entry.underlineThinkness = currentUnderlineThinkness;
        entry.underlinePercent = currentUnderlinePercent;

        BoxBounds bounds = GetTextBounds(text);
        if (currentPivot == FontPivot::Center) {
            f32 width = bounds.max.x - bounds.min.x;
            bounds.min.x -= width / 2.0f;
            bounds.max.x -= width / 2.0f;
        }

        bounds.Translate(position);
        entry.bounds = bounds;

        entries.Add(entry);

        currentUnderlineThinkness = 0.0f;
        currentUnderlinePercent = 1.0f;

        return bounds;
    }

    BoxBounds FontRenderer::PushText(const SmallString& text, const glm::vec2& position) {
        return PushText(text.GetCStr(), position);
    }

    void FontRenderer::Draw(u32 screenWidth, u32 screenHeight) {
        if (currentFont == nullptr) {
            return;
        }

        glm::mat4 projection = glm::ortho(0.0f, (f32)screenWidth, 0.0f, (f32)screenHeight, -1.0f, 1.0f);

        textProgram.Bind();
        textProgram.SetMat4f("p", projection);
        textProgram.SetSampler("texture0", 0);

        glBindVertexArray(vao);

        const i32 entryCount = entries.GetNum();
        for (i32 entryIndex = 0; entryIndex < entryCount; entryIndex++) {
            DrawEntry& entry = entries[entryIndex];

            f32 x = entry.position.x;
            f32 y = entry.position.y;
            const char* text = entry.text.GetCStr();

            const f32 textWidth = GetTextWidth(text);

            if (entry.pivot == FontPivot::Center) {
                x -= textWidth / 2.0f;
            }

            if (entry.underlineThinkness > 0.0f && entry.underlinePercent > 0.0f) {
                textProgram.SetInt("mode", FONT_RENDERING_MODE_GUI);
                textProgram.SetVec4f("color", 1.0, 1.0, 1.0, 1.0);

                f32 xpos = x;
                f32 ypos = y - entry.underlineThinkness - 1.0f;
                f32 w = textWidth * entry.underlinePercent;
                f32 h = entry.underlineThinkness;

                if (entry.pivot == FontPivot::Center) {
                    xpos = x + textWidth / 2.0f - w / 2.0f;
                }

                f32 vertices[6][4] = {
                    { xpos,     ypos + h,   0.0f, 0.0f },
                    { xpos,     ypos,       0.0f, 1.0f },
                    { xpos + w, ypos,       1.0f, 1.0f },

                    { xpos,     ypos + h,   0.0f, 0.0f },
                    { xpos + w, ypos,       1.0f, 1.0f },
                    { xpos + w, ypos + h,   1.0f, 0.0f }
                };

                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
                glBindBuffer(GL_ARRAY_BUFFER, 0);

                glDrawArrays(GL_TRIANGLES, 0, 6);
            }

            textProgram.SetInt("mode", FONT_RENDERING_MODE_TEXT);

            currentFont->texture.Bind(0);

            for (i32 i = 0; text[i] != '\0'; i++) {
                i32 index = (i32)text[i];
                Glyph& ch = currentFont->glyphs[index];

                f32 xpos = x + ch.bearing.x;
                f32 ypos = y - (ch.size.y - ch.bearing.y);
                f32 w = (f32)ch.size.x;
                f32 h = (f32)ch.size.y;

                glm::vec2 uv0 = ch.uv0;
                glm::vec2 uv1 = ch.uv1;
                
                f32 vertices[6][4] = {
                    { xpos,     ypos + h,   uv0.x, uv0.y },
                    { xpos,     ypos,       uv0.x, uv1.y },
                    { xpos + w, ypos,       uv1.x, uv1.y },

                    { xpos,     ypos + h,   uv0.x, uv0.y },
                    { xpos + w, ypos,       uv1.x, uv1.y },
                    { xpos + w, ypos + h,   uv1.x, uv0.y }
                };

                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
                glBindBuffer(GL_ARRAY_BUFFER, 0);

                //ch.texture.Bind(0);

                glDrawArrays(GL_TRIANGLES, 0, 6);

                // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
                // bit shift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
                x += (ch.advance >> 6);
            }
        }

        glBindVertexArray(0);

        entries.Clear();
    }

    void SpaceInvaders::Initialize(WindowData* windowData) {
        drawShader.Load();
        unitQuad.CreateUnitQuad();

        useRawAssets = false;
        if (useRawAssets) {
            assetLoader = &looseAssetLoader;
        }
        else {
            assetLoader = &packedAssetLoader;
        }

        assetLoader->Begin();
        LoadTexture("assets/sprites/ship_A.png", &texturePlayerShip, true, GL_REPEAT);
        LoadTexture("assets/sprites/meteor_detailedSmall.png", &textureSmallMeteor, true, GL_REPEAT);
        LoadTexture("assets/sprites/meteor_detailedLarge.png", &textureLargeMeteor, true, GL_REPEAT);
        LoadTexture("assets/sprites_01/PNG/Lasers/laserGreen13.png", &textureGreenLaser, true, GL_REPEAT);
        LoadTexture("assets/sprites_01/Backgrounds/blue.png", &textureBackground, true, GL_REPEAT);
        LoadTexture("assets/sprites_01/PNG/Effects/star1.png", &textureStar1, true, GL_REPEAT);
        LoadTexture("assets/sprites_01/PNG/Effects/star2.png", &textureStar2, true, GL_REPEAT);
        LoadTexture("assets/sprites_01/PNG/Effects/star3.png", &textureStar3, true, GL_REPEAT);
        
        LoadAudioClip("assets/music/peace_1.ogg", &audioMainMenuMusic);
        LoadAudioClip("assets/sounds/laserSmall_000.ogg", &audioLaser1);
        LoadAudioClip("assets/sounds/explosionCrunch_000.ogg", &audioExplosion1);

        LoadFont("assets/fonts/arial.ttf", &fontMain, 28);
        assetLoader->End();

        worldWidth = (f32)windowData->width;
        worldHeight = (f32)windowData->height;

        lineRenderer.Initialize(2048);
        fontRenderer.Initialize();

        for (u32 i = 0; i < stars.GetCapcity(); i++) {
            Star star = {};
            star.position = glm::vec2(RandomFloat(0, worldWidth), RandomFloat(0, worldHeight));
            star.rotation = RandomFloat(0, 360);
            star.texture = PickRandom(&textureStar1, &textureStar2, &textureStar3);
            stars.Add(star);
        }

        //audioMainMenuMusic.Play();
        gameOver = true;
    }

    void SpaceInvaders::UpdateAndRender(WindowData* windowData, FrameInput* input, f32 dt) {
        worldWidth = (f32)windowData->width;
        worldHeight = (f32)windowData->height;
        glm::vec2 worldDims = glm::vec2(worldWidth, worldHeight);

        glm::mat4 projection = glm::ortho(0.0f, (f32)windowData->width, 0.0f, (f32)windowData->height, -1.0f, 1.0f);
        drawShader.SetMat4f("projection", projection);

        //glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        //BoxCollider box;
        //box.CreateFromCenterSize(glm::vec2(400, 400), glm::vec2(100, 100));
        //lineRenderer.PushLine(glm::vec2(0), glm::vec2(worldWidth, worldHeight), glm::vec4(0, 1, 0, 1));
        //lineRenderer.PushBox(box);
        //lineRenderer.Draw(projection);

        // Draw the background
        for (f32 x = 0; x < 5; x++) {
            for (f32 y = 0; y < 3; y++) {
                glm::vec2 pos = glm::vec2(x * textureBackground.width, y * textureBackground.height);
                DrawSprite(pos, glm::vec2(textureBackground.width, textureBackground.height), 0.0f, &textureBackground);
            }
        }

        for (u32 starIndex = 0; starIndex < stars.GetCount(); starIndex++) {
            Star& star = stars[starIndex];
            DrawSprite(star.position, glm::vec2(star.texture->width * 0.25f, star.texture->height * 0.25f), star.rotation, star.texture);
        }

        if (gameOver) {
            if (input->w && menuSelection != 0) {
                menuSelection = 0;
                menuSelectionUnderlinePercent = 0.0f;
            }
            if (input->s && menuSelection != 1) {
                menuSelection = 1;
                menuSelectionUnderlinePercent = 0.0f;
            }

            if ((IsKeyJustDown(e) || IsKeyJustDown(enter))) {
                if (menuSelection == 0) {
                    StartNewGame();
                    return;
                }
                else if (menuSelection == 1) {
                    windowData->shouldClose = true;
                }
            }

            menuSelectionUnderlinePercent = Lerp(menuSelectionUnderlinePercent, 1.0f, dt * 5.0f);
            menuSelectionUnderlinePercent = glm::clamp(menuSelectionUnderlinePercent, 0.0f, 1.0f);

            fontRenderer.SetFont(&fontMain);
            fontRenderer.SetPivot(FontPivot::Center);
            fontRenderer.PushText(StringBuilder::FormatSmall("Score %d", score), glm::vec2(worldWidth / 2.0f, worldHeight * 0.95f));

            if (menuSelection == 0) { fontRenderer.SetUnderLine(2.0f, menuSelectionUnderlinePercent); }
            BoxBounds playBounds = fontRenderer.PushText(StringBuilder::FormatSmall("Play", score), glm::vec2(worldWidth / 2.0f, worldHeight / 2.0f + 50));

            if (menuSelection == 1) { fontRenderer.SetUnderLine(2.0f, menuSelectionUnderlinePercent); }
            BoxBounds quitBounds = fontRenderer.PushText(StringBuilder::FormatSmall("Quit", score), glm::vec2(worldWidth / 2.0f, worldHeight / 2.0f - 50));

            fontRenderer.Draw(windowData->width, windowData->height);
            lineRenderer.Draw(projection);

            return;
        }

        if (input->a) {
            playerShip.position.x -= 300.0f * dt;
        }

        if (input->d) {
            playerShip.position.x += 300.0f * dt;
        }

        playerShip.position = glm::clamp(playerShip.position, glm::vec2(0, 0) + playerShip.size, glm::vec2(worldWidth, worldHeight) - playerShip.size);

        playerShip.shootCooldown -= dt;
        if (input->space) {
            if (playerShip.shootCooldown <= 0.0f) {
                playerShip.shootCooldown = 0.5f;
                SpawnLaser(playerShip.position - glm::vec2(0, 20), 1.0f);
            }
        }

        BoxBounds playerBox;
        playerBox.CreateFromCenterSize(playerShip.position, playerShip.size);
        //lineRenderer.PushBox(playerBox);

        DrawSprite(playerShip.position, playerShip.size, playerShip.rotation, &texturePlayerShip);

        for (u32 meteorIndex = 0; meteorIndex < meteors.GetCount(); meteorIndex++) {
            Meteor& meteor = meteors[meteorIndex];
            meteor.position.y += 100 * dt;
            meteor.rotation += 25 * dt;

            BoxBounds meteorBox;
            meteorBox.CreateFromCenterSize(meteor.position, meteor.size);
            //lineRenderer.PushBox(meteorBox);

            bool shouldRemove = meteor.position.y > worldHeight + 100;

            if (playerBox.Intersects(meteorBox)) {
                gameOver = true;
                audioExplosion1.Play();
                break;
            }

            for (u32 laserIndex = 0; laserIndex < lasers.GetCount(); laserIndex++) {
                Laser& laser = lasers[laserIndex];
                BoxBounds laserBox;
                laserBox.CreateFromCenterSize(laser.position, laser.size);

                if (laserBox.Intersects(meteorBox)) {
                    lasers.Remove(laserIndex);
                    laserIndex--;
                    shouldRemove = true;
                    audioExplosion1.Play();
                    score++;
                    break;
                }
            }

            if (shouldRemove) {
                meteors.Remove(meteorIndex);
                meteorIndex--;
            }

            Texture* texture = meteor.isBig ? &textureLargeMeteor : &textureSmallMeteor;
            DrawSprite(meteor.position, meteor.size, meteor.rotation, texture);
        }

        for (u32 laserIndex = 0; laserIndex < lasers.GetCount(); laserIndex++) {
            Laser& laser = lasers[laserIndex];
            laser.position.y -= 300 * dt * laser.direction;
            DrawSprite(laser.position, laser.size, laser.rotation, &textureGreenLaser);

            if (laser.position.y > worldHeight - 100) {
                lasers.Remove(laserIndex);
                laserIndex--;
            }
            else if (laser.position.y < -50) {
                lasers.Remove(laserIndex);
                laserIndex--;
            }
        }

        waveTimer -= dt;
        if (waveTimer <= 0) {
            SpawnMeteorWave();
        }

        //for (i32 i = 0; i < fontMain.tileSheet.tiles.GetNum(); i++) {
        //    BoxBounds b;
        //    b.min = fontMain.tileSheet.tiles[i].uv0 * worldDims;
        //    b.max = fontMain.tileSheet.tiles[i].uv1 * worldDims;
        //    lineRenderer.PushBox(b);
        //}

        fontRenderer.SetFont(&fontMain);
        fontRenderer.SetPivot(FontPivot::BottomLeft);
        fontRenderer.PushText(StringBuilder::FormatSmall("Score %d", score), glm::vec2(worldWidth * 0.01f, worldHeight * 0.95f));
        fontRenderer.Draw(windowData->width, windowData->height);

        lineRenderer.Draw(projection);
    }

    void SpaceInvaders::LoadTexture(const char* filename, Texture* texture, bool generateMipMaps, i32 wrapMode) {
        TextureAsset textureAsset = {};
        textureAsset.generateMipMaps = generateMipMaps;
        textureAsset.wrapMode = wrapMode;
        assetLoader->LoadTextureAsset(filename, textureAsset);
        texture->CreateFromAsset(textureAsset);
    }

    void SpaceInvaders::LoadAudioClip(const char* filename, AudioClip* audioClip) {
        AudioAsset audioAsset = {};
        assetLoader->LoadAudioAsset(filename, audioAsset);
        audioClip->CreateFromAsset(audioAsset);
    }

    void SpaceInvaders::LoadFont(const char* fileaname, FontAsset* fontAsset, u32 fontSize) {
        fontAsset->fontSize = fontSize;
        assetLoader->LoadFontAsset(fileaname, *fontAsset);
    }

    void SpaceInvaders::StartNewGame() {
        waveTimer = 0;
        difficulty = 10;
        gameOver = false;
        menuSelection = 0;
        score = 0;
        menuSelectionUnderlinePercent = 0.0f;

        // Create the player ship
        playerShip.position = glm::vec2(worldWidth / 2.0f, worldHeight * 0.87f);

        // Clear the meteors and lasers
        meteors.Clear();
        lasers.Clear();

        // Create the meteors
        SpawnMeteorWave();
    }

    void SpaceInvaders::DrawSprite(glm::vec2 position, glm::vec2 size, f32 rotation, Texture* texture) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(position, 0.0f));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(size, 1.0f));

        drawShader.SetMat4f("model", model);
        drawShader.SetSampler("ourTexture", 0);
        drawShader.Bind();
        texture->Bind(0);
        unitQuad.BindAndDraw();
    }

    void SpaceInvaders::SpawnMeteor(bool isBig) {
        f32 x = RandomFloat(0, worldWidth);
        f32 y = -RandomFloat(100.0f, worldHeight);

        Meteor meteor;
        meteor.position = glm::vec2(x, y);
        meteor.isBig = isBig;

#if 0
        ATTOINFO("Meteor spawned at " << x << ' ' << y);
#endif

        meteors.AddIfPossible(meteor);
    }

    void SpaceInvaders::SpawnLaser(const glm::vec2& position, f32 direction) {
        Laser laser;
        laser.direction = direction;
        laser.position = position;

        if (lasers.AddIfPossible(laser)) {
            audioLaser1.Play();
        }
    }

    void SpaceInvaders::SpawnMeteorWave() {
        ATTOINFO("Spawning meteor wave, new difficulty = " << difficulty + 2);
        for (u32 i = 0; i < difficulty; i++) {
            f32 x = RandomFloat(0.0f, 1.0f);
            SpawnMeteor(x < 0.2f);
        }

        waveTimer = 10.0f;
        difficulty += 2;
    }




}
