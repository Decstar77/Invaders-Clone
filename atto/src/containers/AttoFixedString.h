#pragma once

#include "../AttoDefines.h"
#include "AttoTransientList.h"

namespace atto
{
    class StringHash
    {
    public:
        inline static constexpr u64 ConstStrLen(const char* str) {
            u64 len = 0;
            while (str[len] != '\0') {
                len++;
            }

            return len;
        }

        //https://www.partow.net/programming/hashfunctions/index.html
        inline static constexpr u32 DEKHash(const char* str, unsigned int length) {
            unsigned int hash = length;
            unsigned int i = 0;

            for (i = 0; i < length; ++str, ++i)
            {
                hash = ((hash << 5) ^ (hash >> 27)) ^ (*str);
            }

            return hash;
        }

        inline static constexpr u32 Hash(const char* str) {
            return DEKHash(str, (u32)ConstStrLen(str));
        }
    };

    template<u64 SizeBytes>
    class FixedStringBase
    {
    public:
        inline static const i32 MAX_NUMBER_SIZE = 22;
        inline static const i32 CAPCITY         = static_cast<i32>(SizeBytes - sizeof(i32));

        FixedStringBase();
        FixedStringBase(const char* str);
        FixedStringBase(const FixedStringBase& other);

        void                                SetLength(const i32 &l);
        i32                                 GetLength() const;
        const char*                         GetCStr() const;
        char*                               GetCStr();
        void                                CalculateLength();
        void                                Clear();
        FixedStringBase<SizeBytes>&         Add(const char& c);
        FixedStringBase<SizeBytes>&         Add(const char* c);
        FixedStringBase<SizeBytes>&         Add(const FixedStringBase<SizeBytes>& c);
        i32                                 FindFirstOf(const char& c) const;
        i32                                 FindLastOf(const char& c) const;
        i32                                 NumOf(const char& c) const;
        FixedStringBase<SizeBytes>          SubStr(i32 fromIndex) const;
        FixedStringBase<SizeBytes>          SubStr(i32 startIndex, i32 endIndex) const;
        void                                Replace(const char& c, const char& replaceWith);
        void                                RemoveCharacter(const i32& removeIndex);
        void                                RemoveWhiteSpace();
        b32	                                Contains(const FixedStringBase& str);
        b32	                                StartsWith(const FixedStringBase& str) const;
        void                                CopyFrom(const FixedStringBase& src, const i32& start, const i32& end);
        void                                ToUpperCase();
        void                                StripFileExtension();
        void                                StripFilePath();
        void                                BackSlashesToSlashes();
        TransientList<FixedStringBase>      Split(char delim) const;

        bool                                operator==(const char* other);
        bool                                operator==(const FixedStringBase<SizeBytes>& other) const;
        char&                               operator[](const i32& index);
        char                                operator[](const i32& index) const;
        bool                                operator!=(const FixedStringBase<SizeBytes>& other) const;

    private:
        i32 length;
        char data[CAPCITY];
    };

    template<u64 SizeBytes>
    FixedStringBase<SizeBytes>::FixedStringBase() {
        Clear();
        SetLength(0);
    }

    template<u64 SizeBytes>
    FixedStringBase<SizeBytes>::FixedStringBase(const char* str) {
        const i32 stringLength = static_cast<i32>(StringHash::ConstStrLen(str));

        Assert(stringLength < CAPCITY, "FixedStringBase is too large");

        length = stringLength;

        for (i32 i = 0; i < stringLength; i++) {
            data[i] = str[i];
        }

        for (i32 i = stringLength; i < CAPCITY; i++) {
            data[i] = '\0';
        }
    }

    template<u64 SizeBytes>
    FixedStringBase<SizeBytes>::FixedStringBase(const FixedStringBase& other) {
        Clear();

        length = other.length;
        for (i32 i = 0; i < length; i++) {
            data[i] = other.data[i];
        }
    }

    template<u64 SizeBytes>
    void FixedStringBase<SizeBytes>::SetLength(const i32& l) {
        length = l;
    }

    template<u64 SizeBytes>
    i32 FixedStringBase<SizeBytes>::GetLength() const {
        return length;
    }

    template<u64 SizeBytes>
    const char* FixedStringBase<SizeBytes>::GetCStr() const {
        return data;
    }

    template<u64 SizeBytes>
    char* FixedStringBase<SizeBytes>::GetCStr() {
        return data;
    }

    template<u64 SizeBytes>
    void FixedStringBase<SizeBytes>::CalculateLength() {
        length = static_cast<i32>(StringHash::ConstStrLen(data));
    }

    template<u64 SizeBytes>
    void FixedStringBase<SizeBytes>::Clear() {
        length = 0;
        for (i32 i = 0; i < CAPCITY; i++) {
            data[i] = '\0';
        }
    }

    template<u64 SizeBytes>
    FixedStringBase<SizeBytes>& FixedStringBase<SizeBytes>::Add(const char& c) {
        i32 index = length;
        index++;

        Assert(index < CAPCITY, "FixedStringBase, to many characters");

        if (index < CAPCITY) {
            length = index;
            data[index - 1] = c;
        }

        return *this;
    }

    template<u64 SizeBytes>
    FixedStringBase<SizeBytes>& FixedStringBase<SizeBytes>::Add(const char* c) {
        i32 index = length;
        Assert(index + 1 < CAPCITY, "FixedStringBase, to many characters");

        for (i32 i = 0; index < CAPCITY && c[i] != '\0'; i++, index++) {
            Assert(index < CAPCITY, "FixedStringBase, to many characters");

            if (index < CAPCITY) {
                data[index] = c[i];
                length = index + 1;
            }
        }

        return *this;
    }

    template<u64 SizeBytes>
    FixedStringBase<SizeBytes>& FixedStringBase<SizeBytes>::Add(const FixedStringBase<SizeBytes>& c) {
        return Add(c.data);
    }

    template<u64 SizeBytes>
    i32 FixedStringBase<SizeBytes>::FindFirstOf(const char& c) const {
        const i32 l = length;
        for (i32 i = 0; i < l; i++) {
            if (data[i] == c) {
                return i;
            }
        }

        return -1;
    }

    template<u64 SizeBytes>
    i32 FixedStringBase<SizeBytes>::FindLastOf(const char& c) const {
        const i32 l = length;
        for (i32 i = l; i >= 0; i--) {
            if (data[i] == c) {
                return i;
            }
        }

        return -1;
    }

    template<u64 SizeBytes>
    i32 FixedStringBase<SizeBytes>::NumOf(const char& c) const {
        i32 count = 0;
        for (i32 i = 0; i < length; i++) {
            if (data[i] == c) {
                count++;
            }
        }

        return count;
    }

    template<u64 SizeBytes>
    FixedStringBase<SizeBytes> FixedStringBase<SizeBytes>::SubStr(i32 fromIndex) const {
        const i32 l = length;
        Assert(fromIndex >= 0 && fromIndex < l, "SubStr range invalid");

        FixedStringBase<SizeBytes> result = "";
        for (i32 i = fromIndex; i < l; i++) {
            result.Add(data[i]);
        }

        return result;
    }

    template<u64 SizeBytes>
    FixedStringBase<SizeBytes> FixedStringBase<SizeBytes>::SubStr(i32 startIndex, i32 endIndex) const {
        const i32 l = length;
        Assert(startIndex >= 0 && startIndex < l, "SubStr range invalid");
        Assert(endIndex >= 0 && endIndex < l, "SubStr range invalid");
        Assert(startIndex < endIndex, "SubStr range invalid");

        FixedStringBase<SizeBytes> result = "";
        for (i32 i = startIndex; i < endIndex; i++) {
            result.Add(data[i]);
        }

        return result;
    }


    template<u64 SizeBytes>
    void FixedStringBase<SizeBytes>::Replace(const char& c, const char& replaceWith) {
        const i32 l = length;
        for (i32 i = 0; i < l; i++) {
            if (data[i] == c) {
                data[i] = replaceWith;
            }
        }
    }

    template<u64 SizeBytes>
    void FixedStringBase<SizeBytes>::RemoveCharacter(const i32& removeIndex) {
        const i32 l = length;
        Assert(removeIndex >= 0 && removeIndex < l, "FixedStringBase, invalid index");

        for (i32 i = removeIndex; i < l; i++) {
            data[i] = data[i + 1];
        }

        length = l - 1;
    }

    template<u64 SizeBytes>
    void FixedStringBase<SizeBytes>::RemoveWhiteSpace() {
        for (i32 i = 0; i < GetLength(); i++) {
            char d = data[i];
            if (d == ' ' || d == '\n' || d == '\t' || d == '\r') {
                RemoveCharacter(i);
                i--;
            }
        }
    }

    template<u64 SizeBytes>
    b32 FixedStringBase<SizeBytes>::Contains(const FixedStringBase& str) {
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

    template<u64 SizeBytes>
    b32 FixedStringBase<SizeBytes>::StartsWith(const FixedStringBase& str) const {
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

    template<u64 SizeBytes>
    void FixedStringBase<SizeBytes>::CopyFrom(const FixedStringBase& src, const i32& start, const i32& end) {
        Assert(start >= 0 && start < src.length, "FixedStringBase, invalid index");
        Assert(end >= 0 && end < src.length, "FixedStringBase, invalid index");

        i32 writePtr = 0;
        for (i32 i = start; i <= end; i++, writePtr++) {
            data[writePtr] = src.data[i];
        }

        length = writePtr;
    }

    template<u64 SizeBytes>
    void FixedStringBase<SizeBytes>::ToUpperCase() {
        const i32 l = length;
        for (i32 i = 0; i < l; i++) {
            data[i] = (char)toupper(data[i]);
        }
    }

    template<u64 SizeBytes>
    void FixedStringBase<SizeBytes>::StripFileExtension() {
        i32 index = FindLastOf('.');
        if (index > 0) {
            for (i32 i = index; i < length; i++) {
                data[i] = '\0';
            }
        }

        length = (i32)StringHash::ConstStrLen((char*)data);
    }

    template<u64 SizeBytes>
    void FixedStringBase<SizeBytes>::StripFilePath() {
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

            length = (i32)StringHash::ConstStrLen((char*)data);
        }
    }

    template<u64 SizeBytes>
    void FixedStringBase<SizeBytes>::BackSlashesToSlashes() {
        for (i32 i = 0; i < length; i++) {
            if (data[i] == '\\') {
                data[i] = '/';
            }
        }
    }

    template<u64 SizeBytes>
    TransientList<FixedStringBase<SizeBytes>> FixedStringBase<SizeBytes>::Split(char delim) const {
        const i32 num = NumOf(delim) + 1;
        TransientList<FixedStringBase<SizeBytes>> result(
            num, 
            Memory::AllocateTransientStruct<FixedStringBase<SizeBytes>>(num)
        );

        i32 start = 0;
        i32 end = 0;
        const i32 len = GetLength();
        for (; end < len; end++) {
            if (data[end] == delim) {
                if (start != end) {
                    result[result.count].CopyFrom(*this, start, end - 1);
                    result.count++;
                    start = end + 1;
                }
                else {
                    start++;
                }
            }
        }

        if (end != start) {
            result[result.count].CopyFrom(*this, start, end - 1);
            result.count++;
        }

        return result;
    }

    template<u64 SizeBytes>
    bool FixedStringBase<SizeBytes>::operator==(const char* other) {
        i32 index = 0;
        const i32 l = length;
        const i32 o = static_cast<i32>(StringHash::ConstStrLen(other));

        while (index < l) {
            if (index >= o || data[index] != other[index]) {
                return false;
            }
            index++;
        }

        return true;
    }

    template<u64 SizeBytes>
    bool FixedStringBase<SizeBytes>::operator==(const FixedStringBase<SizeBytes>& other) const {
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

    template<u64 SizeBytes>
    char& FixedStringBase<SizeBytes>::operator[](const i32& index) {
        Assert(index >= 0 && index < length, "FixedString, invalid index");

        return data[index];
    }

    template<u64 SizeBytes>
    char FixedStringBase<SizeBytes>::operator[](const i32& index) const {
        Assert(index >= 0 && index < length, "FixedString, invalid index");
        return data[index];
    }

    template<u64 SizeBytes>
    bool FixedStringBase<SizeBytes>::operator!=(const FixedStringBase<SizeBytes>& other) const {
        return !(*this == other);
    }

    typedef FixedStringBase<64>           SmallString;
    typedef FixedStringBase<256>          LargeString;
    typedef FixedStringBase<Megabytes(4)> VeryLargeString;
}
