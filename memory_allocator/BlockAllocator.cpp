#include "memory_allocator/BlockAllocator.h"

#include <cassert>
#include <cstdlib>
#include <new>
#include <stdexcept>

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
#ifndef NDEBUG
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

    BlockAllocator::memory_size = memory_size;
    headers_count = max_block_count;

    memory = static_cast<char *>(malloc(memory_size + max_block_count * sizeof(Header)));
    headers = reinterpret_cast<Header *>(memory + memory_size);

    for (size_t i = 0; i < max_block_count; ++i)
    {
        new (headers + i) Header();
    }

    headers[0].increment_size(memory_size);
}

BlockAllocator::~BlockAllocator()
{
    if (memory)
    {
        free(memory);
        memory = 0;
    }
}

void *BlockAllocator::allocate(size_t size)
{
    // find best fitting block
    size_t min_diff = INVALID_INT, min_diff_index = 0, min_diff_offset = 0;
    {
        size_t summed_offset = 0;
        for (size_t i = 0; i < headers_count; ++i)
        {
            size_t block_size = headers[i].get_size();
            if (block_size >= size && headers[i].is_free())
            {
                size_t diff = block_size - size;
                if (!diff)
                {
                    headers[i].set_free(false);
                    return static_cast<void *>(memory + summed_offset);
                }
                else if (diff < min_diff)
                {
                    min_diff = diff;
                    min_diff_index = i;
                    min_diff_offset = summed_offset;
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
            if (index_higher < headers_count && headers[index_higher].is_free())
            {
                transfer_dest = headers + index_higher;
            }
            else
            {
                size_t i = find_empty_block_index();

                if (i == INVALID_INT)
                {
                    if (!remainder_size)
                    {
                        remainder_size = min_diff;
                        remainder_offset = min_diff_offset + size;
                        headers[min_diff_index].increment_size(-remainder_size);
                        // set transfer_dest to same block so increment_size calls nullify each other
                        transfer_dest = headers + min_diff_index;
                    }
                }
                else
                {
                    if (i < min_diff_index)
                    {
                        size_t src_index = i + 1;
                        memcpy(headers + i, headers + src_index, sizeof(Header) * (min_diff_index + 1 - src_index));
                        i = min_diff_index;
                        min_diff_index -= 1;
                    }
                    else
                    {
                        size_t src_index = min_diff_index + 1;
                        memcpy(headers + src_index + 1, headers + src_index, sizeof(Header) * (i - src_index));
                        i = src_index;
                    }

                    headers[i].reset();
                    transfer_dest = headers + i;
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

void BlockAllocator::deallocate(void *mem)
{
    size_t offset = static_cast<char *>(mem) - memory;

    assert(offset < memory_size && "invalid memory address");

    size_t summed_offset = 0;

    for (size_t i = 0; i < headers_count; ++i)
    {
        if (summed_offset == offset && !headers[i].is_empty())
        {
            if (offset + headers[i].get_size() == remainder_offset)
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

    for (size_t adjacent_index = i - 1; adjacent_index < headers_count; --adjacent_index)
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

void BlockAllocator::coalesce_adjacent_blocks(size_t i)
{
    size_t adjacent_index = i + 1;
    if (adjacent_index < headers_count && headers[adjacent_index].is_free())
    {
        headers[i] += headers[adjacent_index];
    }

    Header *lower_free_block = get_lower_free(i);
    if (lower_free_block)
    {
        *lower_free_block += headers[i];
    }
}

size_t BlockAllocator::find_empty_block_index()
{
    for (size_t i = 0; i < headers_count; ++i)
    {
        if (headers[i].is_empty())
        {
            return i;
        }
    }

    return INVALID_INT;
}

#ifndef NDEBUG
#include <iostream>

void BlockAllocator::log_headers() const
{
    for (size_t i = 0; i < headers_count;)
    {
        for (size_t j = 0; j < 10 && headers_count; ++j)
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
    for (size_t i = 0; i < headers_count; ++i)
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
    for (size_t i = 0; i < headers_count; ++i)
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

    for (size_t i = 0; i < headers_count; ++i)
    {
        size_t size = headers[i].get_size();
        if (headers[i].is_free() && size)
            largest = largest > size ? largest : size;
    }

    return largest;
}
#endif
