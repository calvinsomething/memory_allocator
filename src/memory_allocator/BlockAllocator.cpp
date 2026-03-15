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

void *BlockAllocator::allocate(size_t size, size_t alignment)
{
    // find best fitting block
    size_t min_diff = INVALID_INT, min_diff_index = 0, min_diff_offset = 0, padding = 0;
    {
        size_t summed_offset = 0;
        for (size_t i = 0; i < empty_headers_start; ++i)
        {
            size_t block_size = headers[i].get_size();
            padding = (alignment - (summed_offset % alignment)) % alignment;
            size_t aligned_size = padding + size;

            if (block_size >= aligned_size && headers[i].is_free())
            {
                size_t diff = block_size - aligned_size;
                if (!diff)
                {
                    headers[i].set_free(false);
                    return static_cast<void *>(memory + summed_offset + padding);
                }
                else if (diff < min_diff)
                {
                    min_diff = diff;
                    min_diff_index = i;
                    min_diff_offset = summed_offset + padding;
                }
            }

            summed_offset +=
                block_size + (summed_offset == remainder_offset) *
                                 remainder_size; // include remainder_size if we encounter remainder's address
        }
    }

    void *mem = 0;
    if (min_diff != INVALID_INT)
    {
        Header *transfer_dest = get_lower_free(min_diff_index);
        if (!transfer_dest)
        {
            size_t index_higher = min_diff_index + 1;
            if (index_higher < empty_headers_start && headers[index_higher].is_free())
            {
                transfer_dest = headers + index_higher;
            }
            else
            {
                if (empty_headers_start == header_count)
                {
                    if (remainder_size)
                    {
                        DEBUG_OUT("BlockAllocator headers exhausted.");
                        return 0;
                    }

                    remainder_size = min_diff;
                    remainder_offset = min_diff_offset + size;
                    headers[min_diff_index].increment_size(-remainder_size);
                    // set transfer_dest to same block so increment_size calls nullify each other
                    transfer_dest = headers + min_diff_index;
                }
                else
                {
                    // split headers array after destination header
                    size_t src_index = min_diff_index + 1;
                    transfer_dest = headers + src_index;

                    // shift right side one to the right
                    memcpy(headers + src_index + 1, transfer_dest, sizeof(Header) * (empty_headers_start - src_index));

                    transfer_dest->reset();

                    ++empty_headers_start;
                }
            }
        }

        headers[min_diff_index].increment_size(-min_diff);
        transfer_dest->increment_size(min_diff);

        headers[min_diff_index].set_free(false);
        mem = static_cast<void *>(memory + min_diff_offset);
    }

    return mem;
}

void BlockAllocator::deallocate(void *mem, size_t alignment)
{
    size_t offset = static_cast<char *>(mem) - memory;

    assert(offset < memory_size && "invalid memory address");

    size_t summed_offset = 0;

    for (size_t i = 0; i < empty_headers_start; ++i)
    {
        size_t padding = (alignment - (summed_offset % alignment)) % alignment;
        size_t aligned_offset = offset - padding;

        if (summed_offset == aligned_offset)
        {
            if (remainder_size && (aligned_offset - remainder_size == remainder_offset ||
                                   aligned_offset + headers[i].get_size() == remainder_offset))
            {
                headers[i].increment_size(remainder_size);
                remainder_size = 0;
                remainder_offset = 0;
            }

            headers[i].set_free(true);

            coalesce_adjacent_blocks(i);

            return;
        }

        summed_offset += headers[i].get_size();
    }

    throw std::runtime_error("BlockAllocator::deallocate failed");
}

BlockAllocator::Header *BlockAllocator::get_lower_free(size_t i)
{
    Header *adjacent_block_header = 0;

    for (size_t adjacent_index = i - 1; adjacent_index < header_count; --adjacent_index)
    {
        if (!headers[adjacent_index].is_free())
        {
            // attempt to use empty header one address higher
            ++adjacent_index;
            if (adjacent_index != i)
            {
                adjacent_block_header = headers + adjacent_index;
            }
            break;
        }
        if (!headers[adjacent_index].is_empty())
        {
            adjacent_block_header = headers + adjacent_index;
            break;
        }
    }

    return adjacent_block_header;
}

void BlockAllocator::shift_empty_header(size_t i)
{
    size_t src_index = i + 1;

    memcpy(headers + i, headers + src_index, sizeof(Header) * (empty_headers_start - src_index));

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

    Header *lower_free_block = get_lower_free(i);
    if (lower_free_block)
    {
        *lower_free_block += headers[i];

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
