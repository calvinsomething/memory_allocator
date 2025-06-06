#include <cstddef>
#include <cstring>
#include <utility>

#include "Chunk.h"

class MemoryAllocator
{
  public:
    template <size_t N> MemoryAllocator(Chunk (&chunks)[N]) : chunks(chunks), chunk_count(N)
    {
    }

    template <typename T, typename... Args> T *allocate(Args &&...args)
    {
        T *mem = 0;

        for (size_t i = 0; i < chunk_count; ++i)
        {
            Chunk *c = chunks + i;
            mem = static_cast<T *>(c->allocate(sizeof(T)));

            if (mem)
            {
                new (mem) T(std::forward<Args>(args)...);
            }
        }

        return mem;
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
    size_t chunk_count = 0;
};
