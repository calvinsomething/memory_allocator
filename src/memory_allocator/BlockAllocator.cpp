#include "memory_allocator/BlockAllocator.h"

#include <cassert>
#include <cstdlib>
#include <new>
#include <stdexcept>

#include "memory_allocator/Debug.h"

// Header
size_t BlockAllocator::Header::get_size() const
{
    return size_and_free_flag & size_mask;
}

bool BlockAllocator::Header::is_free() const
{
    return size_and_free_flag & free_mask;
}

void BlockAllocator::Header::set_free(bool free)
{
    size_and_free_flag = free ? size_and_free_flag | free_mask : size_and_free_flag & size_mask;
}

void BlockAllocator::Header::reset()
{
    size_and_free_flag = free_mask;
}

void BlockAllocator::Header::increment_size(size_t diff)
{
    size_and_free_flag += diff;
}

bool BlockAllocator::Header::is_empty()
{
    return size_and_free_flag == free_mask;
}

// Allocator
BlockAllocator::Header &BlockAllocator::Header::operator+=(BlockAllocator::Header &other)
{
#ifdef BUILD_TESTS
    assert(this != &other && "BlockAllocator::Header::operator+= expects 'this' != 'other'");
    assert(this->is_free() && "BlockAllocator::Header::operator+= expects 'this' to be flagged free");
    assert(other.is_free() && "BlockAllocator::Header::operator+= expects 'other' to be flagged free");
#endif
    size_and_free_flag += other.get_size();
    other.reset();

    return *this;
}

BlockAllocator::BlockAllocator(size_t memory_size, size_t max_block_count)
{
    init(memory_size, max_block_count);
}

void BlockAllocator::init(size_t memory_size, size_t max_block_count)
{
    assert(max_block_count && "max_block_count must be non-zero");

    if (memory)
    {
        free(memory);
        memory = 0;
    }

    BlockAllocator::memory_size = memory_size;
    header_count = max_block_count;

    memory = static_cast<char *>(malloc(memory_size + max_block_count * sizeof(Header)));
    headers = reinterpret_cast<Header *>(memory + memory_size);

    for (size_t i = 0; i < max_block_count; ++i)
    {
        new (headers + i) Header();
    }

    headers[0].increment_size(memory_size);

    empty_headers_start = 1;
}

BlockAllocator::~BlockAllocator()
{
    if (memory)
    {
        free(memory);
        memory = 0;
    }
}

BlockAllocator::Header *BlockAllocator::get_free_header(size_t i)
{
    if (i < empty_headers_start && headers[i].is_free())
    {
        return headers + i;
    }

    return 0;
}

bool BlockAllocator::shift_memory(size_t &i, size_t left, size_t right)
{
    bool do_shift_left = !!left, do_shift_right = !!right;

    if (do_shift_left)
    {
        Header *dest_left = get_free_header(i - 1);
        if (dest_left)
        {
            dest_left->increment_size(left);
            headers[i].increment_size(-left);
            do_shift_left = 0;
        }
    }

    if (do_shift_right)
    {
        Header *dest_right = get_free_header(i + 1);
        if (dest_right)
        {
            dest_right->increment_size(right);
            headers[i].increment_size(-right);
            do_shift_right = 0;
        }
    }

    size_t insert_count = do_shift_left + do_shift_right;

    if (insert_count)
    {
        size_t empty_count = header_count - empty_headers_start;
        if (insert_count > empty_count)
        {
            return false;
        }

        size_t dest_index = i + insert_count;

        memmove(headers + dest_index, headers + i, sizeof(Header) * (empty_headers_start - i));

        i = dest_index - do_shift_right;

        if (do_shift_right)
        {
            headers[i].reset();
            headers[i].increment_size(headers[dest_index].get_size() - right);

            headers[dest_index].reset();
            headers[dest_index].increment_size(right);

            ++empty_headers_start;
        }

        if (do_shift_left)
        {
            headers[i].increment_size(-left);

            headers[i - 1].reset();
            headers[i - 1].increment_size(left);

            ++empty_headers_start;
        }
    }

    return true;
}

void *BlockAllocator::allocate(size_t size, size_t alignment)
{
    // find first fitting block
    size_t diff = INVALID_INT, block_index = 0, block_offset = 0, padding = 0;
    {
        size_t summed_offset = 0;
        for (size_t i = 0; i < empty_headers_start; ++i)
        {
            size_t block_size = headers[i].get_size();
            padding = (alignment - (summed_offset % alignment)) % alignment;
            size_t aligned_size = padding + size;

            if (block_size >= aligned_size && headers[i].is_free())
            {
                diff = block_size - aligned_size;
                block_index = i;
                block_offset = summed_offset;

                break;
            }

            summed_offset += block_size;
        }
    }

    void *mem = 0;
    if (diff != INVALID_INT)
    {
        if (!(padding || diff) || shift_memory(block_index, padding, diff))
        {
            headers[block_index].set_free(false);
            mem = static_cast<void *>(memory + block_offset + padding);
        }
    }

    return mem;
}

void BlockAllocator::deallocate(void *mem)
{
    size_t offset = static_cast<char *>(mem) - memory;

    assert(offset < memory_size && "invalid memory address");

    size_t summed_offset = 0;

    for (size_t i = 0; i < empty_headers_start; ++i)
    {
        if (summed_offset == offset)
        {
            headers[i].set_free(true);

            coalesce_adjacent_blocks(i);

            return;
        }

        summed_offset += headers[i].get_size();
    }

    throw std::runtime_error("BlockAllocator::deallocate failed");
}

void BlockAllocator::shift_empty_header(size_t i)
{
    size_t src_index = i + 1;

    memmove(headers + i, headers + src_index, sizeof(Header) * (empty_headers_start - src_index));
    // memcpy(headers + i, headers + src_index, sizeof(Header) * (empty_headers_start - src_index));

    --empty_headers_start;

    headers[empty_headers_start].reset();
}

void BlockAllocator::coalesce_adjacent_blocks(size_t i)
{
    size_t adjacent_index = i + 1;
    if (adjacent_index < empty_headers_start && headers[adjacent_index].is_free())
    {
        headers[i] += headers[adjacent_index];

        shift_empty_header(adjacent_index);
    }

    adjacent_index -= 2;
    if (i && headers[adjacent_index].is_free())
    {
        headers[adjacent_index] += headers[i];

        shift_empty_header(i);
    }
}

#ifdef BUILD_TESTS
#include <iostream>

void BlockAllocator::log_headers() const
{
    for (size_t i = 0; i < header_count;)
    {
        for (size_t j = 0; j < 10 && header_count; ++j)
        {
            std::cout << headers[i].is_free() << ":" << headers[i].get_size() << " ";
            ++i;
        }
        std::cout << "\n";
    }
}

size_t BlockAllocator::count_active_headers() const
{
    size_t count = 0;
    for (size_t i = 0; i < header_count; ++i)
    {
        if (!headers[i].is_empty())
        {
            count++;
        }
    }
    return count;
}

size_t BlockAllocator::count_free_blocks() const
{
    size_t count = 0;
    for (size_t i = 0; i < header_count; ++i)
    {
        if (headers[i].is_free() && headers[i].get_size())
        {
            count++;
        }
    }
    return count;
}

size_t BlockAllocator::get_largest_free_block() const
{
    size_t largest = 0;

    for (size_t i = 0; i < header_count; ++i)
    {
        size_t size = headers[i].get_size();
        if (headers[i].is_free() && size)
            largest = largest > size ? largest : size;
    }

    return largest;
}
#endif
