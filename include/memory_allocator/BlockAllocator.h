#pragma once

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

    void *allocate(size_t size);

    void deallocate(void *mem);

  private:
    static constexpr size_t INVALID_INT = ~size_t(0);

    size_t memory_size = 0;
    char *memory = 0;

    size_t headers_count = 0;
    Header *headers = 0;

    size_t remainder_offset = 0, remainder_size = 0;

    bool transfer_memory(size_t dest_index, size_t src_index, size_t size);

    size_t find_empty_block_index();
};
