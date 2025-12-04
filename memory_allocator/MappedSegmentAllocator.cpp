#include "memory_allocator/MappedSegmentAllocator.h"

MappedSegmentAllocator::MappedSegmentAllocator()
{
    chunks = static_cast<Chunk *>(malloc(sizeof(Chunk) * max_chunks));
}

MappedSegmentAllocator::~MappedSegmentAllocator()
{
    for (size_t i = 0; i < chunk_count; ++i)
    {
        chunks[i].~Chunk();
    }

    free(chunks);
}

bool MappedSegmentAllocator::add_chunk(size_t size)
{
    if (chunk_count == max_chunks)
    {
        return false;
    }

    chunks[chunk_count].init(size);
    ++chunk_count;

    return true;
}
