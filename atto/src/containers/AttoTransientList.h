#pragma once

#include "../AttoDefines.h"

namespace atto
{
    template<typename _type_>
    class TransientList
    {
    public:
        TransientList() : count(0), capcity(0), data(nullptr) {
        }

        TransientList(const u32& capcity, _type_* memory) : count(0), capcity(capcity), data(memory) {
        }

        inline _type_* GetData() {
            return data;
        }

        inline const _type_* GetData() const {
            return data;
        }

        inline u32 GetCapcity() const {
            return capcity;
        }

        inline u32 GetCount() const {
            return count;
        }

        inline void SetCount(u32 count) {
            this->count = count;
        }

        inline bool IsFull() const {
            return capcity == count;
        }

        inline bool IsEmpty() const {
            return count == 0;
        }

        inline void Clear() {
            count = 0;
            if (data) {
                delete[] data;
            }
        }

        inline _type_* Add(const _type_& value) {
            u32 index = count; count++;
            Assert(index >= 0 && index < capcity, "Array, add to many items");

            data[index] = value;

            return &data[index];
        }

        inline b8 AddIfPossible(const _type_& t) {
            u32 index = count;
            if (index < capcity)
            {
                data[index] = t;
                count++;

                return true;
            }

            return false;
        }

        inline void Remove(const u32& index) {
            Assert(index >= 0 && index < count, "Array invalid remove index ");
            for (u32 i = index; i < count - 1; i++)
            {
                data[i] = data[i + 1];
            }
            count--;
        }

        inline void Remove(const _type_* ptr) {
            for (u32 i = 0; i < count; i++)
            {
                if (ptr == &data[i])
                {
                    Remove(i);
                    return;
                }
            }
        }

        inline _type_* Get(const u32& index) {
            Assert(index >= 0 && index < capcity, "Array, invalid index");
            return &data[index];
        }

        inline const _type_* Get(const u32& index) const {
            Assert(index >= 0 && index < capcity, "Array, invalid index");
            return &data[index];
        }

        inline _type_& operator[](const u32& index) {
            Assert(index >= 0 && index < capcity, "Array, invalid index");
            return data[index];
        }

        inline _type_ operator[](const u32& index) const {
            Assert(index >= 0 && index < capcity, "Array, invalid index");

            return data[index];
        }

    private:
        _type_* data;
        u32 count;
        u32 capcity;

        template<u64>
        friend class FixedStringBase;
        friend class String;
    };
}
