#include "AttoRendering.h"

#include <string>
#include <sstream>
#include <fstream>
#include <glad/glad.h>

#include <al/alc.h>
#include <al/al.h>

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

    void Program::Load() {
        std::string vertexSource = ReadEntireFile("assets/drawShader.vs.glsl");
        std::string fragmentSource = ReadEntireFile("assets/drawShader.fs.glsl");
        Create(vertexSource.c_str(), fragmentSource.c_str());
    }

    void Program::Free() {
        glDeleteProgram(program);
    }

    bool Program::Create(const char* vertexSource, const char* fragmentSource) {
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

    bool Program::CheckCompilationErrors(u32 shader) {
        i32 success;
        char infoLog[1024];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            ATTOERROR("ERROR::SHADER_COMPILATION_ERROR of type: ");
            ATTOERROR(infoLog);
            ATTOERROR("-- --------------------------------------------------- -- ");
        }

        return success;
    }

    bool Program::CheckLinkErrors(u32 program) {
        i32 success;
        char infoLog[1024];
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(program, 1024, NULL, infoLog);
            ATTOERROR("ERROR::SHADER_LINKER_ERROR of type: ");
            ATTOERROR(infoLog);
            ATTOERROR("-- --------------------------------------------------- -- ");
        }

        return success;
    }

    void Program::Bind() {
        glUseProgram(program);
    }

    void Program::SetInt(const char* name, i32 value) {
        i32 location = GetUniformLocation(name);
        if (location >= 0) {
            glProgramUniform1i(program, location, value);
        }
    }

    void Program::SetVec4f(const char* name, f32 x, f32 y, f32 z, f32 w) {
        i32 location = GetUniformLocation(name);
        if (location >= 0) {
            glProgramUniform4f(program, location, x, y, z, w);
        }
    }

    void Program::SetVec4f(const char* name, const glm::vec4& value) {
        i32 location = GetUniformLocation(name);
        if (location >= 0) {
            glProgramUniform4f(program, location, value.x, value.y, value.z, value.w);
        }
    }

    void Program::SetSampler(const char* name, const u32 textureUnit) {
        i32 location = GetUniformLocation(name);
        if (location >= 0) {
            glProgramUniform1i(program, location, textureUnit);
        }
    }

    void Program::SetMat4f(const char* name, const glm::mat4& mat) {
        i32 location = GetUniformLocation(name);
        if (location >= 0) {
            glProgramUniformMatrix4fv(program, location, 1, false, &mat[0][0]);
        }
    }

    i32 Program::GetUniformLocation(const char* name) {
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
            ATTOERROR("Could not find uniform value %s", name);
        }

        return location;
    }

    void Mesh::CreateUnitQuad() {
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

    void Mesh::CreateMesh(const void* vertexData, u32 vertexCount, u32 vertexStride) {
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

    void Mesh::BindAndDraw() {
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    }

    void Texture::CreateFromAsset(const TextureAsset& textureAsset) {
        //this->width = textureAsset.width;
        //this->height = textureAsset.height;

        //glGenTextures(1, &texture);
        //glBindTexture(GL_TEXTURE_2D, texture);
        //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureAsset.width, textureAsset.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureAsset.data.GetData());
        //if (textureAsset.generateMipMaps) {
        //    glGenerateMipmap(GL_TEXTURE_2D);
        //}

        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, textureAsset.wrapMode);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, textureAsset.wrapMode);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, textureAsset.generateMipMaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        //glBindTexture(GL_TEXTURE_2D, 0);
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

    void BoxBounds::Translate(const glm::vec2& translation) {
        min += translation;
        max += translation;
    }

    void BoxBounds::CreateFromCenterSize(const glm::vec2& center, const glm::vec2& size) {
        max = center + size * 0.5f;
        min = center - size * 0.5f;
    }

    bool RayCast::Box(const BoxBounds& b, const Ray2D& r, f32& t) {
        f32 tx1 = (b.min.x - r.origin.x) * (1.0f / r.direction.x);
        f32 tx2 = (b.max.x - r.origin.x) * (1.0f / r.direction.x);

        f32 tmin = glm::min(tx1, tx2);
        f32 tmax = glm::max(tx1, tx2);

        f32 ty1 = (b.min.y - r.origin.y) * (1.0f / r.direction.y);
        f32 ty2 = (b.max.y - r.origin.y) * (1.0f / r.direction.y);

        tmin = glm::max(tmin, glm::min(ty1, ty2));
        tmax = glm::min(tmax, glm::max(ty1, ty2));

        t = tmin;

        return tmax >= tmin;
    }

    bool BoxBounds::Intersects(const BoxBounds& other) {
        return (max.x >= other.min.x && min.x <= other.max.x) &&
            (max.y >= other.min.y && min.y <= other.max.y);
    }

    void DebugRenderer::Initialize(u32 maxLines) {
        totalSizeBytes = maxLines * sizeof(LineVertex);
        CreateProgram();
        CreateBuffer();
        ATTOTRACE("LineRenderer Initialized");
    }

    void DebugRenderer::CreateProgram() {
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

    void DebugRenderer::CreateBuffer() {
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

    void DebugRenderer::PushLine(const glm::vec2& start, const glm::vec2& end, const glm::vec4& color) {
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

    void DebugRenderer::PushRay(const Ray2D& ray, const glm::vec4& color) {
        PushLine(ray.origin, ray.origin + ray.direction, color);
    }

    void DebugRenderer::PushBox(const BoxBounds& box, const glm::vec4& color) {
        glm::vec2 v1 = box.min;
        glm::vec2 v2 = box.max;
        glm::vec2 v3 = glm::vec2(box.min.x, box.max.y);
        glm::vec2 v4 = glm::vec2(box.max.x, box.min.y);
        PushLine(v1, v3, color);
        PushLine(v1, v4, color);
        PushLine(v2, v3, color);
        PushLine(v2, v4, color);
    }

    void DebugRenderer::Draw(const glm::mat4& projection) {
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
        ATTOTRACE("FontRenderer Initialized");
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

    f32 FontRenderer::GetTextWidth(const FontAsset* currentFont, const char* text) {
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

    BoxBounds FontRenderer::GetTextBounds(const FontAsset* currentFont, const char* text) {
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

    BoxBounds FontRenderer::Push(FontDrawEntry& entry) {
        BoxBounds bounds = GetTextBounds(entry.font, entry.text.GetCStr());
        if (entry.pivot == FontPivot::Center) {
            f32 width = bounds.max.x - bounds.min.x;
            bounds.min.x -= width / 2.0f;
            bounds.max.x -= width / 2.0f;
        }

        bounds.Translate(entry.position);
        entry.bounds = bounds;
        entries.Add(entry);

        return bounds;
    }

    void FontRenderer::Push(const FontAsset* font, const glm::vec2& pos, const char* text) {
        FontDrawEntry drawEntry = {};
        drawEntry.font = font;
        drawEntry.position = pos;
        drawEntry.text = text;

        Push(drawEntry);
    }

    void FontRenderer::Draw(const glm::mat4& projection) {
        textProgram.Bind();
        textProgram.SetMat4f("p", projection);
        textProgram.SetSampler("texture0", 0);

        glBindVertexArray(vao);

        const i32 entryCount = entries.GetNum();
        for (i32 entryIndex = 0; entryIndex < entryCount; entryIndex++) {
            FontDrawEntry& entry = entries[entryIndex];

            f32 x = entry.position.x;
            f32 y = entry.position.y;
            const char* text = entry.text.GetCStr();

            // @TODO :Cache this
            const f32 textWidth = GetTextWidth(entry.font, entry.text.GetCStr());

            if (entry.pivot == FontPivot::Center) {
                x -= textWidth / 2.0f;
            }

            if (entry.underlineThinkness > 0.0f && entry.underlinePercent > 0.0f) {
                textProgram.SetInt("mode", (i32)FontRenderingMode::FONT_RENDERING_MODE_GUI);
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

            textProgram.SetInt("mode", (i32)FontRenderingMode::FONT_RENDERING_MODE_TEXT);

            glBindTextureUnit(0, entry.font->textureHandle);

            for (i32 i = 0; text[i] != '\0'; i++) {
                i32 index = (i32)text[i];
                Glyph& ch = entry.font->glyphs[index];

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

    void SpriteRenderer::Initialize() {
        totalSizeBytes = sizeof(SpriteVertex) * 6;
        CreateProgram();
        CreateBuffer();
        ATTOTRACE("SpriteRenderer Initialized");
    }

    void SpriteRenderer::CreateProgram() {
        const char* vertexShaderSource = R"(
            #version 330 core

            layout (location = 0) in vec2 position;
            layout (location = 1) in vec2 texCoord;
            layout (location = 2) in vec4 color;

            out vec2 vertexTexCoord;
            out vec4 vertexColor;

            uniform mat4 p;

            void main() {
                vertexTexCoord = texCoord;
                vertexColor = color;
                gl_Position = p * vec4(position.x, position.y, 0.0, 1.0);
            }
        )";

        const char* fragmentShaderSource = R"(
            #version 330 core
            out vec4 FragColor;

            in vec2 vertexTexCoord;
            in vec4 vertexColor;

            uniform sampler2D texture0;
            uniform int mode;

            void main() {
                if (mode == 0) {
                    vec4 sampled = texture(texture0, vertexTexCoord);
                    FragColor = vertexColor * sampled;
                } else if (mode == 1) {
                    FragColor = vertexColor;
                }
            }
        )";

        program.Create(vertexShaderSource, fragmentShaderSource);
    }

    void SpriteRenderer::CreateBuffer() {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, totalSizeBytes, nullptr, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        const u32 vertexStride = sizeof(SpriteVertex);

        glVertexAttribPointer(0, 2, GL_FLOAT, false, vertexStride, 0);
        glVertexAttribPointer(1, 2, GL_FLOAT, false, vertexStride, (void*)(2 * sizeof(f32)));
        glVertexAttribPointer(2, 4, GL_FLOAT, false, vertexStride, (void*)((2 + 2) * sizeof(f32)));

        glBindVertexArray(0);
    }

    void SpriteRenderer::Push(SpriteDrawEntry& drawEntry) {
        entries.Add(drawEntry);
    }

    void SpriteRenderer::PushQuad(const glm::vec2& pos, const glm::vec2& size, const glm::vec4& color) {
        SpriteDrawEntry entry;
        entry.drawKind = SPRITE_DRAW_KIND_QUAD;
        entry.position = pos;
        entry.scale = size;
        entry.color = color;
        entries.Add(entry);
    }

    void SpriteRenderer::Draw(const glm::mat4& projection) {
        program.Bind();
        program.SetMat4f("p", projection);
        program.SetSampler("texture0", 0);

        glBindVertexArray(vao);

        const i32 entryCount = entries.GetCount();
        for (i32 entryIndex = 0; entryIndex < entryCount; entryIndex++) {
            SpriteDrawEntry& entry = entries[entryIndex];
            SpriteAsset& sprite = entry.sprite;

            f32 xpos = entry.position.x;
            f32 ypos = entry.position.y;

            glm::vec2 uv0 = sprite.uv0;
            glm::vec2 uv1 = sprite.uv1;

            f32 w = entry.scale.x;
            f32 h = entry.scale.y;
            
            if (entry.drawKind == SPRITE_DRAW_KIND_SPRITE) {
                if (sprite.texture == nullptr) {
                    ATTOWARN("Sprite texture is null");
                    continue;
                }
            
                w *= sprite.texture->width;
                h *= sprite.texture->height;

                if (entry.sprite.animated) {
                    w = sprite.frameSize.x * entry.scale.x;
                    h = sprite.frameSize.y * entry.scale.y;
                    //uv0.x = (sprite.frameIndex * sprite.frameSize.x) / sprite.texture->width;
                    //uv1.x = (sprite.frameIndex * sprite.frameSize.x + sprite.frameSize.x) / sprite.texture->width;
                }
            }
          

            glm::vec4 color = entry.color;

            f32 vertices[6][2 + 2 + 4] = {
                { xpos,     ypos + h,    uv0.x, uv0.y,    color.r, color.g, color.b, color.a },
                { xpos,     ypos,        uv0.x, uv1.y,    color.r, color.g, color.b, color.a },
                { xpos + w, ypos,        uv1.x, uv1.y,    color.r, color.g, color.b, color.a },

                { xpos,     ypos + h,    uv0.x, uv0.y,    color.r, color.g, color.b, color.a },
                { xpos + w, ypos,        uv1.x, uv1.y,    color.r, color.g, color.b, color.a },
                { xpos + w, ypos + h,    uv1.x, uv0.y,    color.r, color.g, color.b, color.a }
            };

            Assert(totalSizeBytes == sizeof(vertices), "SpriteVertex size mismatch");
            static_assert(sizeof(vertices) == sizeof(SpriteVertex) * 6, "SpriteVertex size mismatch");

            if (entry.drawKind == SPRITE_DRAW_KIND_SPRITE) {
                glBindTextureUnit(0, sprite.texture->textureHandle);
            }

            program.SetInt("mode", entry.drawKind);

            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        glBindVertexArray(0);

        entries.Clear();
    }



}



