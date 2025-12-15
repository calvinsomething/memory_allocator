#pragma once

#include <cstddef>

template <typename ValueType, typename AllocatorType, unsigned ID = 0> class Adapter
{
  public:
    using value_type = ValueType;

    Adapter() = default;

    Adapter(const Adapter &other)
    {
    }

    template <typename V, typename A, unsigned I> Adapter(const Adapter<V, A, I> &other)
    {
    }

    Adapter &operator=(const Adapter &other)
    {
        return *this;
    }

    template <typename V, typename A, unsigned I> Adapter &operator=(const Adapter<V, A, I> &other)
    {
        return *this;
    }

    template <typename V> struct rebind
    {
        using other = Adapter<V, AllocatorType, ID>;
    };

    Adapter(Adapter &&other)
    {
    }

    Adapter operator=(Adapter &&other)
    {
    }

    ValueType *allocate(size_t n = 1)
    {
        return static_cast<ValueType *>(allocator.allocate(n * sizeof(ValueType)));
    }

    void deallocate(ValueType *mem, size_t n)
    {
        return allocator.deallocate(mem);
    }

    template <typename V, typename A, unsigned I> bool operator==(const Adapter<V, A, I> &other)
    {
        return &allocator == &other.allocator;
    }

    template <typename V, typename A, unsigned I> bool operator!=(const Adapter<V, A, I> &other)
    {
        return !(*this == other);
    }

    static AllocatorType &allocator;
};

template <typename AllocatorType, unsigned ID> class AllocatorGroup
{
  public:
    static AllocatorType allocator;
};

template <typename AllocatorType, unsigned ID> AllocatorType AllocatorGroup<AllocatorType, ID>::allocator;

template <typename ValueType, typename AllocatorType, unsigned ID>
AllocatorType &Adapter<ValueType, AllocatorType, ID>::allocator = AllocatorGroup<AllocatorType, ID>::allocator;
