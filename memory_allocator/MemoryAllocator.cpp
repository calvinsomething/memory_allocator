#include "MemoryAllocator.h"

MemoryAllocator::MemoryAllocator(size_t max_chunks) : max_chunks(max_chunks)
{
    chunks = static_cast<Chunk *>(malloc(sizeof(Chunk) * max_chunks));
}

MemoryAllocator::~MemoryAllocator()
{
    for (size_t i = 0; i < chunk_count; ++i)
    {
        chunks[i].~Chunk();
    }

    free(chunks);
}

bool MemoryAllocator::add_chunk(size_t size)
{
    if (chunk_count == max_chunks)
    {
        return false;
    }

    chunks[chunk_count].init(size);
    ++chunk_count;

    return true;
}
