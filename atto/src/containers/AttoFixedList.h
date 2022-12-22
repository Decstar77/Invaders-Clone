#pragma once

#include "../AttoDefines.h"

namespace atto
{
    template<typename T, u32 capcity>
    class FixedList
    {
    public:
        FixedList();

        T*             GetData();
        const T*       GetData() const;
        u32            GetCapcity() const;
        u32            GetCount() const;
        void           SetCount(u32 count);
        bool           IsFull() const;
        bool           IsEmpty() const;
        void           Clear();

        T*             Add(const T& value);
        b8             AddIfPossible(const T& t);
        void           Remove(const u32& index);
        void           Remove(const T* ptr);

        T*             Get(const u32& index);
        const T*       Get(const u32& index) const;

        T& operator[](const u32& index);
        T operator[](const u32& index) const;

    private:
        T data[capcity];
        u32 count;
    };

    template<typename T, u32 capcity>
    FixedList<T, capcity>::FixedList() {
        count = 0;
        for (u32 i = 0; i < capcity; i++) {
            data[i] = {};
        }
    }

    template<typename T, u32 capcity>
    const T* FixedList<T, capcity>::Get(const u32& index) const {
        Assert(index >= 0 && index < capcity, "Array, invalid index");
        return &data[index];
    }


    template<typename T, u32 capcity>
    T* FixedList<T, capcity>::Get(const u32& index) {
        Assert(index >= 0 && index < capcity, "Array, invalid index");

        return &data[index];
    }

    template<typename T, u32 capcity>
    void FixedList<T, capcity>::Clear() {
        count = 0;
    }

    template<typename T, u32 capcity>
    bool FixedList<T, capcity>::IsEmpty() const {
        return count == 0;
    }

    template<typename T, u32 capcity>
    bool FixedList<T, capcity>::IsFull() const {
        return capcity == count;
    }

    template<typename T, u32 capcity>
    void FixedList<T, capcity>::SetCount(u32 count) {
        this->count = count;
    }

    template<typename T, u32 capcity>
    u32 FixedList<T, capcity>::GetCount() const {
        return count;
    }

    template<typename T, u32 capcity>
    u32 FixedList<T, capcity>::GetCapcity() const {
        return capcity;
    }

    template<typename T, u32 capcity>
    T* FixedList<T, capcity>::GetData() {
        return data;
    }

    template<typename T, u32 capcity>
    const T* FixedList<T, capcity>::GetData() const {
        return data;
    }

    template<typename T, u32 capcity>
    T* FixedList<T, capcity>::Add(const T& value) {
        u32 index = count; count++;
        Assert(index >= 0 && index < capcity, "Array, add to many items");

        data[index] = value;

        return &data[index];
    }

    template<typename T, u32 capcity>
    b8 FixedList<T, capcity>::AddIfPossible(const T& t) {
        u32 index = count;
        if (index < capcity)
        {
            data[index] = t;
            count++;

            return true;
        }

        return false;
    }

    template<typename T, u32 capcity>
    void FixedList<T, capcity>::Remove(const u32& index) {
        Assert(index >= 0 && index < count, "Array invalid remove index ");
        for (u32 i = index; i < count - 1; i++)
        {
            data[i] = data[i + 1];
        }
        count--;
    }

    template<typename T, u32 capcity>
    void FixedList<T, capcity>::Remove(const T* ptr) {
        for (u32 i = 0; i < count; i++)
        {
            if (ptr == &data[i])
            {
                Remove(i);
                return;
            }
        }
    }

    template<typename T, u32 capcity>
    T FixedList<T, capcity>::operator[](const u32& index) const {
        Assert(index >= 0 && index < capcity, "Array, invalid index");

        return data[index];
    }

    template<typename T, u32 capcity>
    T& FixedList<T, capcity>::operator[](const u32& index) {
        Assert(index >= 0 && index < capcity, "Array, invalid index");

        return data[index];
    }
}
