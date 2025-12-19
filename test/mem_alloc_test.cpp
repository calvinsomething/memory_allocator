#include <cstdlib>
#include <gtest/gtest.h>

#include "./AdapterFixture.h"
#include "memory_allocator/BlockAllocator.h"
#include "memory_allocator/LinearAllocator.h"

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

using IntAdapterFixture = AdapterFixture<int>;

TEST_F(IntAdapterFixture, VectorAllocation)
{
    init(12, 5);

    std::vector<int, A> v;

    v.push_back(3);

    EXPECT_EQ(v[0], 3);
}

TEST_F(IntAdapterFixture, MultipleVectorAllocations)
{
    init(1024, 100); // Larger arena and header capacity

    std::vector<int, A> v1, v2, v3, v4;

    // Push multiple elements to trigger reallocations
    for (int i = 0; i < 10; ++i)
    {
        v1.push_back(i);
        v2.push_back(i * 10);
        v3.push_back(i * 100);
        v4.push_back(i * 1000); // 4th vector - where your crash happens
    }

    // Verify data integrity
    for (int i = 0; i < 10; ++i)
    {
        EXPECT_EQ(v1[i], i);
        EXPECT_EQ(v2[i], i * 10);
        EXPECT_EQ(v3[i], i * 100);
        EXPECT_EQ(v4[i], i * 1000);
    }
}

TEST_F(IntAdapterFixture, VectorReallocationStress)
{
    init(2048, 200);

    std::vector<int, A> v;

    // Force multiple reallocations (vector typically doubles capacity)
    // This tests allocation, deallocation, and reallocation cycles
    for (int i = 0; i < 100; ++i)
    {
        v.push_back(i);

        // Verify all elements are still correct after each reallocation
        for (int j = 0; j <= i; ++j)
        {
            EXPECT_EQ(v[j], j) << "Failed at iteration " << i << ", index " << j;
        }
    }
}

TEST_F(IntAdapterFixture, AlignmentCheck)
{
    init(512, 50);

    std::vector<int, A> v;

    // Allocate and check alignment of the underlying pointer
    v.reserve(10);

    int *ptr = v.data();
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);

    // int typically requires 4-byte alignment, but allocators often use 8 or 16
    EXPECT_EQ(addr % alignof(int), 0) << "Pointer not aligned for int";
    EXPECT_EQ(addr % 8, 0) << "Pointer not 8-byte aligned";
}

TEST_F(IntAdapterFixture, InterleavedAllocationDeallocation)
{
    init(2048, 200);

    std::vector<int, A> v1, v2, v3, v4;

    // Interleaved operations that might expose coalescing bugs
    v1.push_back(1);
    v2.push_back(2);
    v1.push_back(3);
    v3.push_back(4);
    v2.push_back(5);
    v4.push_back(6);

    // Clear some vectors (triggers deallocation)
    v2.clear();
    v2.shrink_to_fit();

    // Allocate again (might reuse freed space)
    v4.push_back(7);
    v4.push_back(8);

    // Verify
    EXPECT_EQ(v1[0], 1);
    EXPECT_EQ(v1[1], 3);
    EXPECT_EQ(v3[0], 4);
    EXPECT_EQ(v4[0], 6);
    EXPECT_EQ(v4[1], 7);
    EXPECT_EQ(v4[2], 8);
}

TEST_F(IntAdapterFixture, ExhaustHeadersAndArena)
{
    init(sizeof(int) * 2, 2);

    std::vector<int, A> vec;

    vec.push_back(0);
    EXPECT_THROW(try { vec.push_back(1); } catch (std::exception &e) { throw e; } catch (...){}, std::exception);

    vec.clear();

    vec.push_back(0);
    EXPECT_THROW(try { vec.push_back(1); } catch (std::exception &e) { throw e; } catch (...){}, std::exception);
}

TEST_F(IntAdapterFixture, ArenaExhaustion)
{
    init(1000, 100); // Very small arena

    std::vector<std::vector<int, A>, A> vectors;

    // Try to allocate until we run out of space
    bool allocation_failed = false;
    try
    {
        for (int i = 0; i < 100; ++i)
        {
            vectors.emplace_back();

            for (int j = 0; j < 50; ++j)
            {
                vectors.back().push_back(j);
            }
        }
    }
    catch (...)
    {
        allocation_failed = true;
    }

    // Should either succeed or fail gracefully
    // If it crashes with corrupted headers, this test will catch it
    EXPECT_TRUE(allocation_failed || !!vectors.size());
}

TEST_F(IntAdapterFixture, RepeatedAllocationDeallocation)
{
    init(50000, 200);

    void *first_alloc = 0;

    // Repeatedly allocate and deallocate to stress header management
    for (int cycle = 0; cycle < 10; ++cycle)
    {
        std::vector<std::vector<int, A>, A> vectors;

        // Allocate many vectors
        for (int i = 0; i < 20; ++i)
        {
            vectors.emplace_back();
            if (!first_alloc)
            {
                first_alloc = vectors.data();
            }
            for (int j = 0; j < 20; ++j)
            {
                vectors.back().push_back(cycle * 1000 + i * 10 + j);
            }
        }

        // Verify
        for (int i = 0; i < 20; ++i)
        {
            EXPECT_EQ(vectors[i].size(), 20);
            for (int j = 0; j < 20; ++j)
            {
                EXPECT_EQ(vectors[i][j], cycle * 1000 + i * 10 + j);
            }
        }

        // vectors go out of scope, deallocating everything
    }

    A::allocator.log_headers();

    A::allocator.deallocate(static_cast<char *>(first_alloc) + 284);

    A::allocator.log_headers();
}

TEST_F(IntAdapterFixture, NoCoalescingWhenNotAdjacent)
{
    init(sizeof(int) * 3, 3);

    // Allocate 3 blocks
    int *p1 = adapter.allocate(1);
    EXPECT_TRUE(p1);
    int *p2 = adapter.allocate(1);
    EXPECT_TRUE(p2);
    int *p3 = adapter.allocate(1);
    EXPECT_TRUE(p3);

    // Free p1 and p3 (not adjacent, p2 separates them)
    adapter.deallocate(p1, 1);
    adapter.deallocate(p3, 1);

    // Should have 2 separate free blocks
    EXPECT_EQ(A::allocator.count_free_blocks(), 2);

    adapter.deallocate(p2, 1);
}

TEST_F(IntAdapterFixture, CoalescingAdjacentBlocks)
{
    init(sizeof(int) * 3, 3);

    // Allocate 3 adjacent blocks
    int *p1 = adapter.allocate(1);
    EXPECT_TRUE(p1);
    int *p2 = adapter.allocate(1);
    EXPECT_TRUE(p2);
    int *p3 = adapter.allocate(1);
    EXPECT_TRUE(p3);

    // Free all 3 (they're adjacent)
    adapter.deallocate(p1, 1);
    adapter.deallocate(p2, 1);
    adapter.deallocate(p3, 1);

    // Should coalesce into 1 large free block
    EXPECT_EQ(A::allocator.count_free_blocks(), 1);
    EXPECT_GE(A::allocator.get_largest_free_block(), sizeof(int) * 3);
}

TEST_F(IntAdapterFixture, FragmentationPreventsLargeAlloc)
{
    init(sizeof(int) * 4, 4);

    // Allocate 4 blocks
    int *p1 = adapter.allocate(1);
    EXPECT_TRUE(p1);
    int *p2 = adapter.allocate(1);
    EXPECT_TRUE(p2);
    int *p3 = adapter.allocate(1);
    EXPECT_TRUE(p3);
    int *p4 = adapter.allocate(1);
    EXPECT_TRUE(p4);

    // Free alternating blocks (p1 and p3)
    adapter.deallocate(p1, 1);
    adapter.deallocate(p3, 1);

    EXPECT_THROW(adapter.allocate(2), std::bad_alloc) << "Should fail due to fragmentation";
}

TEST_F(IntAdapterFixture, HeaderReuseAfterCoalescing)
{
    init(sizeof(int) * 2, 3);

    // First cycle
    int *p1 = adapter.allocate(1);
    EXPECT_TRUE(p1);
    int *p2 = adapter.allocate(1);
    EXPECT_TRUE(p2);
    adapter.deallocate(p1, 1);
    adapter.deallocate(p2, 1);

    size_t headers_after_cycle1 = A::allocator.count_active_headers();

    // Second cycle - should reuse headers
    int *p3 = adapter.allocate(1);
    EXPECT_TRUE(p3);
    int *p4 = adapter.allocate(1);
    EXPECT_TRUE(p4);
    adapter.deallocate(p3, 1);
    adapter.deallocate(p4, 1);

    size_t headers_after_cycle2 = A::allocator.count_active_headers();

    // Headers shouldn't grow between cycles if reuse works
    EXPECT_EQ(headers_after_cycle1, headers_after_cycle2) << "Headers not being reused";
}

TEST_F(IntAdapterFixture, VectorReallocationsCreateHeaders)
{
    // Need enough space for vector growth: 1, 2, 4, 8 = 15 ints total
    init(sizeof(int) * 15, 10);

    size_t initial = A::allocator.count_active_headers();

    {
        std::vector<int, A> vec;
        vec.reserve(1); // Start with capacity 1

        // Push 8 items, causing reallocations: 1->2, 2->4, 4->8
        for (int i = 0; i < 8; ++i)
        {
            vec.push_back(i);
        }

        size_t during = A::allocator.count_active_headers();
        size_t free_blocks = A::allocator.count_free_blocks();

        // Should have accumulated headers from the reallocation cycles
        // Active headers = current allocation + freed blocks (if not coalesced)
        std::cout << "Active headers: " << during << ", Free blocks: " << free_blocks << "\n";

        // If coalescing works well, free blocks should be fewer than total reallocations (3)
        EXPECT_LE(free_blocks, 3) << "Too many free blocks suggests poor coalescing";
    }

    // After vec is destroyed, should have freed its memory
    EXPECT_GT(A::allocator.count_free_blocks(), 0);
}
