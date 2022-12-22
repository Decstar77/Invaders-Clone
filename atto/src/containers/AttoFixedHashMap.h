#pragma once

#include "AttoFixedList.h"
#include "AttoTransientList.h"

namespace atto
{
    template<typename T>
    class FixedHashMap : public Object
    {
    public:
        inline void Put(u64 key, const T& t) {
            u64 hash = Hash(key);
            u32 index = static_cast<u32>(hash % entries.GetCapcity());

            FixedList<Entry, 25>& bucket = entries[index];

            for (u32 i = 0; i < bucket.GetCapcity(); i++) {
                Entry& entry = bucket[i];
                if (!entry.valid) {
                    entry.key = key;
                    entry.valid = true;
                    entry.t = t;
                    count++;
                    return;
                }
            }

            Assert(0, "Could not place item in hashmap, it's full");
        }

        inline T* Create(u64 key) {
            u64 hash = Hash(key);
            u32 index = static_cast<u32>(hash % entries.GetCapcity());

            FixedList<Entry, 25>* bucket = entries.Get(index);

            for (u32 i = 0; i < bucket->GetCapcity(); i++)
            {
                Entry* entry = bucket->Get(i);
                if (!entry->valid)
                {
                    entry->key = key;
                    entry->valid = true;
                    count++;
                    return &entry->t;
                }
            }

            Assert(0, "Could not create item in hashmap, it's full");
            return nullptr;
        }

        inline T* Get(u64 key) {
            u64 hash = Hash(key);
            u32 index = static_cast<u32>(hash % entries.GetCapcity());

            FixedList<Entry, 25>& bucket = entries[index];

            for (u32 i = 0; i < bucket.GetCapcity(); i++) {
                Entry* entry = bucket.Get(i);
                if (entry->valid && entry->key == key) {
                    return &entry->t;
                }
            }

            return nullptr;
        }

        inline TransientList<T*> GetTransientValueSet() {
            TransientList<T*> result = TransientList<T*>(count, memorySystem->AllocateTransient<T*>(count));

            for (u32 buckedIndex = 0; buckedIndex < entries.GetCapcity(); buckedIndex++) {
                FixedList<Entry, 25>* bucket = entries.Get(buckedIndex);

                for (u32 entryIndex = 0; entryIndex < bucket->GetCapcity(); entryIndex++) {
                    Entry* entry = bucket->Get(entryIndex);

                    if (entry->valid) {
                        result.Add(&entry->t);
                    }
                }
            }

            return result;
        }

        inline void Clear() {
            MemorySystem::ZeroStruct(&entries);
            count = 0;
        }

    private:
        struct Entry {
            u64 key = 0;
            b8 valid = false;
            T t = {};
        };

        inline u64 Hash(u64 x) {
            // @NOTE: Source https://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key
            x = (x ^ (x >> 30)) * u64(0xbf58476d1ce4e5b9);
            x = (x ^ (x >> 27)) * u64(0x94d049bb133111eb);
            x = x ^ (x >> 31);
            return x;
        }

        u32 count = 0;
        FixedList<FixedList<Entry, 25>, 250> entries;
    };
}
