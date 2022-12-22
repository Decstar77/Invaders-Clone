#include "AttoString.h"

#include <string>

namespace atto
{
    String::String() : capicty(0), length(0), data(nullptr) {
    }

    String::String(const char* str) : capicty(0), length(0), data(nullptr) {
        Set(str);
    }

    String::String(const String& other) : capicty(0), length(0), data(nullptr) {
        Set(other.GetCStr());
    }

    String::~String() {
        if (data) {
            delete[] data;
        }
    }

    void String::SetLength(const i32& l) {
        length = l;
    }

    i32 String::GetLength() const {
        return length;
    }

    i32 String::GetCapcity() const {
        return capicty;
    }

    const char* String::GetCStr() const {
        return data;
    }

    char* String::GetCStr() {
        return data;
    }

    void String::CalculateLength() {
        length = (i32)strlen(data);
    }

    void String::Clear() {
        length = 0;
        for (i32 i = 0; i < capicty; i++) {
            data[i] = '\0';
        }
    }

    void String::Set(const char* str) {
        i32 l = (i32)strlen(str);
        Resize(AlignUp(l, 256));
        strcpy_s(data, capicty, str);
        length = l;
    }

    void String::Resize(i32 newCapicty) {
        if (capicty == 0) {
            data = new char[newCapicty];
            capicty = newCapicty;
            return;
        }

        if (capicty < newCapicty) {
            char* newData = new char[newCapicty];
            memcpy(newData, data, capicty);
            capicty = newCapicty;

            delete[] data;
            data = newData;
        }
    }

    String& String::Add(const char& c) {
        i32 index = length;
        index++;

        Assert(index < capicty, "String, to many characters");

        if (index < capicty) {
            length = index;
            data[index - 1] = c;
        }

        return *this;
    }

    String& String::Add(const char* c) {
        i32 index = length;
        Assert(index + 1 < capicty, "FixedStringBase, to many characters");

        for (i32 i = 0; index < capicty && c[i] != '\0'; i++, index++) {
            Assert(index < capicty, "FixedStringBase, to many characters");

            if (index < capicty) {
                data[index] = c[i];
                length = index + 1;
            }
        }

        return *this;
    }

    String& String::Add(const String& c) {
        return Add(c.data);
    }

    i32 String::FindFirstOf(const char& c) const {
        const i32 l = length;
        for (i32 i = 0; i < l; i++) {
            if (data[i] == c) {
                return i;
            }
        }

        return -1;
    }

    i32 String::FindLastOf(const char& c) const {
        const i32 l = length;
        for (i32 i = l; i >= 0; i--) {
            if (data[i] == c) {
                return i;
            }
        }

        return -1;
    }

    i32 String::NumOf(const char& c) const {
        i32 count = 0;
        for (i32 i = 0; i < length; i++) {
            if (data[i] == c) {
                count++;
            }
        }

        return count;
    }

    String String::SubStr(i32 fromIndex) const {
        const i32 l = length;
        Assert(fromIndex >= 0 && fromIndex < l, "SubStr range invalid");

        String result = "";
        result.Resize(l);
        for (i32 i = fromIndex; i < l; i++) {
            result.Add(data[i]);
        }

        return result;
    }

    String String::SubStr(i32 startIndex, i32 endIndex) const {
        const i32 l = length;
        Assert(startIndex >= 0 && startIndex < l, "SubStr range invalid");
        Assert(endIndex >= 0 && endIndex < l, "SubStr range invalid");
        Assert(startIndex < endIndex, "SubStr range invalid");

        String result = "";
        result.Resize(l);
        for (i32 i = startIndex; i < endIndex; i++) {
            result.Add(data[i]);
        }

        return result;
    }

    void String::Replace(const char& c, const char& replaceWith) {
        const i32 l = length;
        for (i32 i = 0; i < l; i++) {
            if (data[i] == c) {
                data[i] = replaceWith;
            }
        }
    }

    void String::RemoveCharacter(const i32& removeIndex) {
        const i32 l = length;
        Assert(removeIndex >= 0 && removeIndex < l, "FixedStringBase, invalid index");

        for (i32 i = removeIndex; i < l; i++) {
            data[i] = data[i + 1];
        }

        length = l - 1;
    }

    void String::RemoveWhiteSpace() {
        for (i32 i = 0; i < GetLength(); i++) {
            char d = data[i];
            if (d == ' ' || d == '\n' || d == '\t' || d == '\r') {
                RemoveCharacter(i);
                i--;
            }
        }
    }

    b32 String::Contains(const String& str) {
        const i32 otherLength = str.length;
        const i32 ourLength = length;
        b32 result = false;

        for (i32 i = 0; i < ourLength && !result; i++) {
            result = true;
            for (i32 j = 0; j < otherLength; j++) {
                if (data[i] != str.data[j]) {
                    result = false;
                    break;
                }
                else {
                    i++;
                }
            }
        }

        return result;
    }

    b32 String::StartsWith(const String& str) const {
        const i32 l = length;
        const i32 ll = str.length;

        if (l < ll) {
            return false;
        }

        for (i32 i = 0; i < ll; i++) {
            if (data[i] != str.data[i]) {
                return false;
            }
        }

        return true;
    }

    void String::CopyFrom(const String& src, const i32& start, const i32& end) {
        Assert(start >= 0 && start < src.length, "FixedStringBase, invalid index");
        Assert(end >= 0 && end < src.length, "FixedStringBase, invalid index");

        Resize(src.length);

        i32 writePtr = 0;
        for (i32 i = start; i <= end; i++, writePtr++) {
            data[writePtr] = src.data[i];
        }

        length = writePtr;
    }

    void String::ToUpperCase() {
        const i32 l = length;
        for (i32 i = 0; i < l; i++) {
            data[i] = (char)toupper(data[i]);
        }
    }

    void String::StripFileExtension() {
        i32 index = FindLastOf('.');
        if (index > 0) {
            for (i32 i = index; i < length; i++) {
                data[i] = '\0';
            }
        }

        length = (i32)strlen((char*)data);
    }

    void String::StripFilePath() {
        i32 index = FindLastOf('/');

        if (index > 0) {
            i32 cur = 0;
            for (i32 i = index + 1; i < length; i++) {
                data[cur] = data[i];
                cur++;
            }

            for (i32 i = cur; i < length; i++) {
                data[i] = '\0';
            }

            length = (i32)strlen((char*)data);
        }
    }

    void String::BackSlashesToSlashes() {
        for (i32 i = 0; i < length; i++) {
            if (data[i] == '\\') {
                data[i] = '/';
            }
        }
    }

    char String::operator[](const i32& index) const {
        Assert(index >= 0 && index < capicty, "String, invalid index");

        return data[index];
    }

    bool String::operator!=(const String& other) const {
        return !(*this == other);
    }

    bool String::operator==(const String& other) const {
        const i32 l1 = length;
        const i32 l2 = other.length;

        if (l1 != l2) {
            return false;
        }

        for (i32 i = 0; i < l1; i++) {
            if (data[i] != other.data[i]) {
                return false;
            }
        }

        return true;
    }

    bool String::operator==(const char* other) {
        i32 index = 0;
        const i32 l = length;
        const i32 o = static_cast<i32>(strlen(other));

        while (index < l) {
            if (index >= o || data[index] != other[index]) {
                return false;
            }
            index++;
        }

        return true;
    }

}

