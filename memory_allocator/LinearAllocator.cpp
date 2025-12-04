#include "memory_allocator/LinearAllocator.h"

LinearAllocator::LinearAllocator(size_t size)
{
    begin = static_cast<char *>(malloc(size));
    memset(begin, 0, size);

    cursor = begin;
    end = begin + size;
}

LinearAllocator::~LinearAllocator()
{
    free(begin);
}

void LinearAllocator::free(void *mem)
{
    if (mem >= begin && mem < cursor)
    {
        memset(mem, 0, cursor - static_cast<char *>(mem));
        cursor = static_cast<char *>(mem);
    }
}
