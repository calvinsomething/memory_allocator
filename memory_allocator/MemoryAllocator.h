#include <cstddef>
#include <cstring>
#include <utility>

#include "Chunk.h"

class MemoryAllocator
{
  public:
    MemoryAllocator(size_t max_chunks);
    ~MemoryAllocator();

    bool add_chunk(size_t size);

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

                break;
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

    size_t chunk_count = 0, max_chunks = 0;
};
