#include "Chunk.h"

Chunk::Chunk(size_t size)
{
    init(size);
}

void Chunk::init(size_t size)
{
    assert("Chunk size cannot be less than " STRING(MIN_SEGMENT_SIZE) " bytes" && size >= MIN_SEGMENT_SIZE);

    memory_size = size;

    set_bitmap_size();

    memory = static_cast<char *>(malloc(memory_size + bitmap_size));

    bitmap = memory + memory_size;
    memset(bitmap, ~0, bitmap_size);

    free_bytes_count = memory_size;
}

Chunk::~Chunk()
{
    if (memory)
    {
        ::free(memory);
    }
}

void *Chunk::allocate(size_t bytes_requested)
{
    if (bytes_requested > free_bytes_count)
    {
        return 0;
    }

    size_t segment_size = MIN_SEGMENT_SIZE, bitmap_index = 0, segments_count = max_segments_count;

    while (segment_size < bytes_requested)
    {
        bitmap_index += segments_count;

        segments_count /= 2;

        segment_size *= 2;
    }

    int bit_offset = 7 - bitmap_index % 8;

    size_t byte_index = bitmap_index / 8;

    for (size_t i = 0; i < segments_count; ++byte_index)
    {
        for (; bit_offset >= 0; --bit_offset)
        {
            if (bitmap[byte_index] & (1 << bit_offset))
            {
                set_free(false, i, segment_size);

                return static_cast<void *>(&memory[i * segment_size]);
            }

            ++i;
        }

        bit_offset = 7;
    }

    return 0;
}

bool Chunk::free(void *segment, size_t size)
{
    int offset = static_cast<char *>(segment) - memory;

    if (offset >= 0 && offset < memory_size)
    {
        size_t segment_size = MIN_SEGMENT_SIZE;
        while (segment_size < size)
        {
            segment_size *= 2;
        }

        // convert offset from bytes to segments
        set_free(true, offset / segment_size, segment_size);

        return true;
    }

    return false;
}

Chunk::operator bool()
{
    return !!memory;
}

void Chunk::set_bitmap_size()
{
    max_segments_count = memory_size / MIN_SEGMENT_SIZE;

    size_t segments_count = max_segments_count;
    do
    {
        bitmap_size += segments_count;
    } while (segments_count /= 2);

    // convert bit size to byte size
    bitmap_size = (bitmap_size + 7) / 8;
}

void Chunk::set_free(bool free, size_t offset, size_t segment_size)
{
    size_t subsegments_count = segment_size / MIN_SEGMENT_SIZE, segments_count = max_segments_count;

    size_t segments_bit_index = 0;

    while (subsegments_count)
    {
        size_t bit_index = segments_bit_index + offset * subsegments_count;
        size_t byte_index = bit_index / 8;

        size_t remaining_bits_to_set = subsegments_count, bit_offset = bit_index % 8,
               max_focused_bits_count = 8 - bit_offset;

        while (remaining_bits_to_set)
        {
            size_t focused_bits_count =
                remaining_bits_to_set > max_focused_bits_count ? max_focused_bits_count : remaining_bits_to_set;
            remaining_bits_to_set -= focused_bits_count;

            unsigned char focused_bits = ~0u << (8 - focused_bits_count);
            focused_bits >>= bit_offset;

            bitmap[byte_index] = (bitmap[byte_index] & ~focused_bits) | (focused_bits * free);

            ++byte_index;
            max_focused_bits_count = 8;
            bit_offset = 0;
        }

        segments_bit_index += segments_count;

        segments_count /= 2;
        subsegments_count = (subsegments_count + (!!segments_count * !free)) / 2;
    }

    free_bytes_count += (2 * free - 1) * segment_size;
}
