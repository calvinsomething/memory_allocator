#include "MemoryAllocator.h"
#include <gtest/gtest.h>

#include <array>

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

TEST(AllocatorTest, OverAllocate)
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

    bytes_32 *c = static_cast<bytes_32 *>(chunk.allocate(sizeof(bytes_32)));
    ASSERT_FALSE(c);

    ASSERT_TRUE(chunk.free(a, sizeof(*a)));

    char *chars[8];

    for (size_t i = 0; i < 8; ++i)
    {
        chars[i] = static_cast<char *>(chunk.allocate(sizeof(char)));

        std::cout << "chars[i] = " << reinterpret_cast<unsigned long>(chars[i]) << "\n";

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
    Chunk chunks[1] = {Chunk(64)};

    MemoryAllocator allocator{chunks};

    bool alive = false;

    TestClass *tc = allocator.allocate<TestClass>(alive, 1, 2, 3);

    ASSERT_NE(tc, nullptr);

    EXPECT_EQ(alive, true);
    EXPECT_EQ(tc->a, 1);
    EXPECT_EQ(tc->b, 2);
    EXPECT_EQ(tc->c, 3);

    allocator.free(tc);

    ASSERT_EQ(alive, false);

    std::cout << "\nDONE 1\n";
}
