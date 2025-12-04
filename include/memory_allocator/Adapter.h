#pragma once

#include <cstddef>

template <typename ValueType, typename AllocatorType, unsigned char ID = 0> class Adapter
{
  public:
    using value_type = ValueType;

    Adapter() = default;

    Adapter(const Adapter &other)
    {
    }

    template <typename V, typename A, unsigned char I> Adapter(const Adapter<V, A, I> &other)
    {
    }

    Adapter &operator=(const Adapter &other)
    {
        return *this;
    }

    template <typename V, typename A, unsigned char I> Adapter &operator=(const Adapter<V, A, I> &other)
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
        return allocator.template allocate<ValueType>(n);
    }

    void deallocate(ValueType *mem, size_t n)
    {
        return allocator.template deallocate<ValueType>(mem, n);
    }

    template <typename V, typename A, unsigned char I> bool operator==(const Adapter<V, A, I> &other)
    {
        return &allocator == &other.allocator;
    }

    template <typename V, typename A, unsigned char I> bool operator!=(const Adapter<V, A, I> &other)
    {
        return !(*this == other);
    }

    static AllocatorType &allocator;
};

template <typename AllocatorType, unsigned char ID> class AllocatorGroup
{
  public:
    static AllocatorType allocator;
};

template <typename AllocatorType, unsigned char ID> AllocatorType AllocatorGroup<AllocatorType, ID>::allocator;

template <typename ValueType, typename AllocatorType, unsigned char ID>
AllocatorType &Adapter<ValueType, AllocatorType, ID>::allocator = AllocatorGroup<AllocatorType, ID>::allocator;
