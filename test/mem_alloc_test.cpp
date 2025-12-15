#include <cstdlib>
#include <gtest/gtest.h>

#include <array>
#include <random>

#include "memory_allocator/Adapter.h"
#include "memory_allocator/BlockAllocator.h"
#include "memory_allocator/LinearAllocator.h"
#include "memory_allocator/MappedSegmentAllocator.h"

class TestClass
{
  public:
    TestClass(bool &alive, int a, int b, int c) : alive(alive), a(a), b(b), c(c)
    {
        alive = true;
    }

    ~TestClass()
    {
        alive = false;
    }

    int a, b, c;

    bool &alive;
};

TEST(BlockAllocatorTest, Allocate)
{
    constexpr size_t size = sizeof(int);

    BlockAllocator allocator(size * 2, 2);

    int *a = static_cast<int *>(allocator.allocate(size));
    ASSERT_TRUE(a);

    int *b = static_cast<int *>(allocator.allocate(size));
    ASSERT_TRUE(b);

    ASSERT_NE(a, b);
}

TEST(BlockAllocatorTest, OverAllocate)
{
    constexpr size_t size = sizeof(int);

    BlockAllocator allocator(size, 1);

    int *a = static_cast<int *>(allocator.allocate(size));
    ASSERT_TRUE(a);

    int *b = static_cast<int *>(allocator.allocate(size));
    ASSERT_FALSE(b);
}

TEST(BlockAllocatorTest, Deallocate)
{
    constexpr size_t size = sizeof(int);

    BlockAllocator allocator(size * 2, 2);

    int *a = static_cast<int *>(allocator.allocate(size));
    ASSERT_TRUE(a);

    int *b = static_cast<int *>(allocator.allocate(size));
    ASSERT_TRUE(b);

    allocator.deallocate(b);
    allocator.deallocate(a);

    int *c = static_cast<int *>(allocator.allocate(size));
    ASSERT_EQ(a, c);

    int *d = static_cast<int *>(allocator.allocate(size));
    ASSERT_EQ(b, d);
}

TEST(BlockAllocatorTest, Remainder)
{
    constexpr size_t size = sizeof(int);

    BlockAllocator allocator(size * 3, 2);

    int *a = static_cast<int *>(allocator.allocate(size));
    ASSERT_TRUE(a);

    int *b = static_cast<int *>(allocator.allocate(size));
    ASSERT_TRUE(b);
}

TEST(AdapterTest, VectorAllocation)
{
    using A = Adapter<int, BlockAllocator>;
    A::allocator.init(12, 5);

    std::vector<int, A> v;

    v.push_back(3);

    EXPECT_EQ(v[0], 3);
}

// TEST(AdapterTest, VectorLifeCycle)
//{
//     Adapter<int, MappedSegmentAllocator> a;
//
//     Adapter<char, MappedSegmentAllocator> b;
//     b.allocator.add_chunk(sizeof(int) * 8);
//
//     std::vector<int, decltype(a)> v;
//
//     v.push_back(0);
//
//     const int *addr = v.data();
//
//     EXPECT_NE(addr, a.allocate());
//
//     v.push_back(1);
//     v.push_back(2);
//     v.push_back(3);
//
//     EXPECT_EQ(v[3], 3);
//
//     EXPECT_NE(addr, v.data());
//
//     v.pop_back();
//     v.pop_back();
//     v.pop_back();
//
//     EXPECT_EQ(v.size(), 1);
// }
//
// TEST(AdapterTest, NestedVectors)
//{
//     Adapter<int, MappedSegmentAllocator> a;
//     a.allocator.add_chunk(10'000'000);
//
//     constexpr size_t element_count = 64000;
//
//     struct Parent
//     {
//         std::vector<Parent *, Adapter<Parent *, MappedSegmentAllocator>> parents;
//         size_t i = 0;
//     };
//
//     std::vector<Parent, Adapter<Parent, MappedSegmentAllocator>> parents;
//
//     std::unordered_map<int, Parent *> parents_by_int;
//
//     parents.reserve(element_count);
//
//     for (size_t i = 0; i < element_count; ++i)
//     {
//         parents.push_back({{}, i});
//
//         parents_by_int.insert({i, &parents.back()});
//     }
//
//     std::random_device rd;
//     std::uniform_int_distribution<int> dist(0, element_count);
//
//     for (Parent &p : parents)
//     {
//         for (size_t i = 0; i < 2; ++i)
//         {
//             int index = dist(rd);
//
//             if (index != p.i)
//             {
//                 auto parent = parents_by_int.find(index);
//
//                 if (parent != parents_by_int.end())
//                 {
//                     p.parents.push_back(parent->second);
//                 }
//             }
//         }
//     }
// }
//
// TEST(LinearAllocatorTest, AllocateAndFree)
//{
//     LinearAllocator a(64);
//
//     bool alive = false;
//
//     TestClass *tc = a.emplace<TestClass>(alive, 1, 2, 3);
//
//     EXPECT_TRUE(alive);
//
//     a.free(tc);
//     EXPECT_EQ(tc->a, 0);
//     EXPECT_EQ(tc->b, 0);
//     EXPECT_EQ(tc->c, 0);
// }
