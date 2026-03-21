#pragma once

#include <cstddef>
#include <cstring>

class BlockAllocator
{
    struct Header
    {
      public:
        size_t get_size() const;

        bool is_free() const;

        void set_free(bool free);

        void reset();

        void increment_size(size_t diff);

        bool is_empty();

        Header &operator+=(Header &other);

      private:
        static constexpr size_t size_mask = ~size_t(0) >> 1;
        static constexpr size_t free_mask = ~size_mask;

        size_t size_and_free_flag = free_mask;
    };

  public:
    BlockAllocator() = default;
    BlockAllocator(size_t memory_size, size_t max_block_count);
    void init(size_t memory_size, size_t max_block_count);

    ~BlockAllocator();

    void *allocate(size_t size, size_t alignment = alignof(std::max_align_t));

    void deallocate(void *mem);

#ifdef BUILD_TESTS
    void log_headers() const;

    size_t count_free_blocks() const;

    size_t get_largest_free_block() const;

    size_t count_active_headers() const;
#endif

  private:
    static constexpr size_t INVALID_INT = ~size_t(0);

    size_t memory_size = 0;
    char *memory = 0;

    size_t header_count = 0;
    Header *headers = 0;

    size_t empty_headers_start = 0;

    Header *get_free_header(size_t i);

    bool shift_memory(size_t &i, size_t left, size_t right);

    void coalesce_adjacent_blocks(size_t i);

    void shift_empty_header(size_t i);
};
