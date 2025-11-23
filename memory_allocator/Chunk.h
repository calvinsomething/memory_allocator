#include <cassert>
#include <cstdlib>
#include <cstring>

#define MIN_SEGMENT_SIZE 4

#define STRING(s) MACRO_TO_STR(#s)
#define MACRO_TO_STR(s) #s

class Chunk
{
  public:
    Chunk() = default;
    Chunk(size_t size);
    void init(size_t size);
    ~Chunk();

    void *allocate(size_t bytes_requested);

    bool free(void *segment, size_t size);

    operator bool();

  private:
    void set_bitmap_size();

    void set_free(bool free, size_t offset, size_t segment_size);

    char *memory = 0;
    size_t memory_size = 0;

    char *bitmap;
    size_t bitmap_size = 1;

    size_t max_segments_count = 1;

    size_t free_bytes_count = 0;
};
