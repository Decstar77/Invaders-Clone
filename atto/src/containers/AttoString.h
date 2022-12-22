#pragma once

#include "../AttoDefines.h"

namespace atto
{
    class String
    {
    public:
        String();
        String(const char* str);
        String(const String& other);
        ~String();

		void                                SetLength(const i32& l);
        i32                                 GetLength() const;
        i32                                 GetCapcity() const;
        const char*                         GetCStr() const;
        char*                               GetCStr();
        void                                CalculateLength();
        void                                Clear();
        void                                Set(const char* str);
        void                                Resize(i32 newCapicty);
        String&                             Add(const char& c);
        String&                             Add(const char* c);
        String&                             Add(const String& c);
        i32                                 FindFirstOf(const char& c) const;
        i32                                 FindLastOf(const char& c) const;
        i32                                 NumOf(const char& c) const;
        String                              SubStr(i32 fromIndex) const;
        String                              SubStr(i32 startIndex, i32 endIndex) const;
        void                                Replace(const char& c, const char& replaceWith);
        void                                RemoveCharacter(const i32& removeIndex);
        void                                RemoveWhiteSpace();
        b32	                                Contains(const String& str);
        b32	                                StartsWith(const String& str) const;
        void                                CopyFrom(const String& src, const i32& start, const i32& end);
        void                                ToUpperCase();
        void                                StripFileExtension();
        void                                StripFilePath();
        void                                BackSlashesToSlashes();

        bool                                operator==(const char* other);
        bool                                operator==(const String& other) const;
        char                                operator[](const i32& index) const;
        bool                                operator!=(const String& other) const;

    private:
        i32 length;
        i32 capicty;
        char* data;
    };
}
