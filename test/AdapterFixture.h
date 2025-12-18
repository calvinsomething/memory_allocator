#pragma once

#include "memory_allocator/Adapter.h"
#include "memory_allocator/BlockAllocator.h"
#include <gtest/gtest.h>

class AdapterFixture : public testing::Test
{
  protected:
    using A = Adapter<int, BlockAllocator>;

    ~AdapterFixture()
    {
        if (initialized)
        {
            A::allocator.~BlockAllocator();
        }
    }

    void init(size_t memory_size, size_t max_block_count)
    {
        A::allocator.init(memory_size, max_block_count);
        initialized = true;
    }

    bool initialized = false;
};
