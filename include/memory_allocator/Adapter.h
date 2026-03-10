#pragma once

#include <cstddef>
#include <new>
#include <type_traits>

#include "memory_allocator/BlockAllocator.h"

template <typename T> struct is_allocator : std::false_type
{
};

template <> struct is_allocator<BlockAllocator> : std::true_type
{
};

template <typename T> using ValidAllocator = typename std::enable_if<is_allocator<T>::value>::type;

template <typename ValueType, typename AllocatorType, unsigned ID = 0, typename Enable = void> class Adapter
{
};

template <typename ValueType, typename AllocatorType, unsigned ID>
class Adapter<ValueType, AllocatorType, ID, ValidAllocator<AllocatorType>>
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

    Adapter &operator=(Adapter &&other)
    {
        return *this;
    }

    static ValueType *allocate(size_t n = 1)
    {
        ValueType *mem = static_cast<ValueType *>(allocator.allocate(n * sizeof(ValueType)));

        if (!mem)
        {
            throw std::bad_alloc();
        }

        return mem;
    }

    static void deallocate(ValueType *mem, size_t n)
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

    // extensions
    static ValueType *create()
    {
        ValueType *mem = allocate();

        new (mem) ValueType;

        return mem;
    }

    static void release(ValueType *value)
    {
        value->~ValueType();

        deallocate(value, 1);
    }
};

template <typename AllocatorType, unsigned ID> class AllocatorGroup
{
  public:
    static AllocatorType allocator;
};

template <typename AllocatorType, unsigned ID> AllocatorType AllocatorGroup<AllocatorType, ID>::allocator;

template <typename ValueType, typename AllocatorType, unsigned ID>
AllocatorType &Adapter<ValueType, AllocatorType, ID, ValidAllocator<AllocatorType>>::allocator =
    AllocatorGroup<AllocatorType, ID>::allocator;
