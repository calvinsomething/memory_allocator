#pragma once

#include <cstddef>
#include <cstring>
#include <utility>

#include "memory_allocator/Chunk.h"

class MappedSegmentAllocator
{
  public:
    MappedSegmentAllocator();
    ~MappedSegmentAllocator();

    bool add_chunk(size_t size);

    template <typename T> T *allocate(size_t n = 1)
    {
        T *mem = 0;
        size_t size = n * sizeof(T);

        for (size_t i = 0; i < chunk_count; ++i)
        {
            Chunk *c = chunks + i;
            mem = static_cast<T *>(c->allocate(size));

            if (mem)
            {
                break;
            }
        }

        return mem;
    }

    template <typename T, typename... Args> void construct(T *mem, Args &&...args)
    {
        new (mem) T(std::forward<Args>(args)...);
    }

    template <typename T, typename... Args> T *emplace(Args &&...args)
    {
        T *mem = allocate<T>();

        if (mem)
        {
            construct(mem, std::forward<Args>(args)...);
        }

        return mem;
    }

    template <typename T> void deallocate(T *mem, size_t n = 1)
    {
        for (size_t i = 0; i < chunk_count; ++i)
        {
            if ((chunks + i)->free(mem, sizeof(T)))
            {
                memset(mem, 0, sizeof(T));

                return;
            }
        }
    }

    template <typename T> void free(T *mem)
    {
        for (size_t i = 0; i < chunk_count; ++i)
        {
            if ((chunks + i)->free(mem, sizeof(T)))
            {
                mem->~T();

                memset(mem, 0, sizeof(T));

                return;
            }
        }
    }

  private:
    Chunk *chunks = 0;

    size_t chunk_count = 0, max_chunks = 20;
};
