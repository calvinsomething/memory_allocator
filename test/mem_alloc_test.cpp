#include <gtest/gtest.h>

#include <array>

#include "memory_allocator/Adapter.h"
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

TEST(ChunkTest, OverAllocate)
{
    Chunk chunk(64);

    using bytes_32 = std::array<char, 32>;

    bytes_32 *a = static_cast<bytes_32 *>(chunk.allocate(sizeof(bytes_32)));
    ASSERT_TRUE(a);

    char *acp = reinterpret_cast<char *>(a);

    for (size_t i = 0; i < 32; ++i)
    {
        acp[i] = 'A';
    }

    bytes_32 *b = static_cast<bytes_32 *>(chunk.allocate(sizeof(bytes_32)));
    ASSERT_TRUE(b);

    ASSERT_NE(a, b);

    char *bcp = reinterpret_cast<char *>(b);

    for (size_t i = 0; i < 32; ++i)
    {
        bcp[i] = 'B';
    }

    EXPECT_EQ((*a)[0], static_cast<char>('A'));
    EXPECT_EQ((*a)[31], static_cast<char>('A'));
    EXPECT_EQ((*b)[0], static_cast<char>('B'));
    EXPECT_EQ((*b)[31], static_cast<char>('B'));

    bytes_32 *c = static_cast<bytes_32 *>(chunk.allocate(sizeof(bytes_32)));
    ASSERT_FALSE(c);

    ASSERT_TRUE(chunk.free(a, sizeof(*a)));

    char *chars[8];

    for (size_t i = 0; i < 8; ++i)
    {
        chars[i] = static_cast<char *>(chunk.allocate(sizeof(char)));

        if (i)
        {
            ASSERT_TRUE(chars[i]);
        }
        else
        {
            ASSERT_EQ(static_cast<void *>(a), static_cast<void *>(chars[i]));
        }
    }

    ASSERT_FALSE(static_cast<char *>(chunk.allocate(sizeof(char))));
}

TEST(AllocatorTest, AllocateAndFree)
{
    MappedSegmentAllocator allocator;
    allocator.add_chunk(64);

    bool alive = false;

    TestClass *tc = allocator.emplace<TestClass>(alive, 1, 2, 3);

    ASSERT_TRUE(tc);

    EXPECT_EQ(alive, true);
    EXPECT_EQ(tc->a, 1);
    EXPECT_EQ(tc->b, 2);
    EXPECT_EQ(tc->c, 3);

    int *i = allocator.allocate<int>(2);

    EXPECT_NE(static_cast<void *>(tc), static_cast<void *>(i));

    allocator.free(tc);

    ASSERT_EQ(alive, false);
}

TEST(AdapterTest, VectorAllocation)
{
    using A = Adapter<int, MappedSegmentAllocator>;
    A::allocator.add_chunk(sizeof(int) * 8);

    std::vector<int, A> v;

    v.push_back(3);

    EXPECT_EQ(v[0], 3);
}

TEST(AdapterTest, VectorLifeCycle)
{
    Adapter<int, MappedSegmentAllocator> a;
    a.allocator.add_chunk(sizeof(int) * 8);

    std::vector<int, decltype(a)> v;

    v.push_back(0);

    const int *addr = v.data();

    EXPECT_NE(addr, a.allocate());

    v.push_back(1);
    v.push_back(2);
    v.push_back(3);

    EXPECT_EQ(v[3], 3);

    EXPECT_NE(addr, v.data());

    v.pop_back();
    v.pop_back();
    v.pop_back();

    EXPECT_EQ(v.size(), 1);
}

TEST(LinearAllocatorTest, AllocateAndFree)
{
    LinearAllocator a(64);

    bool alive = false;

    TestClass *tc = a.emplace<TestClass>(alive, 1, 2, 3);

    EXPECT_TRUE(alive);

    a.free(tc);
    EXPECT_EQ(tc->a, 0);
    EXPECT_EQ(tc->b, 0);
    EXPECT_EQ(tc->c, 0);
}
