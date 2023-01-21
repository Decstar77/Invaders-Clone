#include "AttoAsset.h"

#include <fstream>

namespace atto 
{
    byte* LeEngine::LoadEntireFile(const char* path, i32& fileSize) {
        // Load entire file
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            ATTOERROR("Could not open file: %s", path);
            return nullptr;
        }

        fileSize = (i32)file.tellg();
        byte* data = new byte[fileSize];
        
        file.seekg(0, std::ios::beg);
        file.read((char*)data, fileSize);
        
        file.close();
        
        return data;
    }
}