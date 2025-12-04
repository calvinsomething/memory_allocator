#include <cstddef>
#include <cstdlib>
#include <cstring>

#include <utility>

class LinearAllocator
{
  public:
    LinearAllocator(size_t size);

    ~LinearAllocator();

    template <typename T> T *allocate(size_t n = 1)
    {
        T *p = 0;

        char *next = cursor + n * sizeof(T);
        if (next <= end)
        {
            p = reinterpret_cast<T *>(cursor);
            cursor = next;
        }

        return p;
    }

    template <typename T, typename... Args> T *emplace(Args &&...args)
    {
        T *mem = allocate<T>();

        if (mem)
        {
            new (mem) T(std::forward<Args>(args)...);
        }

        return mem;
    }

    void free(void *mem);

  private:
    char *begin, *cursor, *end;
};
