# memory_allocator

A custom memory allocator library written in C++.

## Overview

Currently includes `BlockAllocator`, which allocates blocks of memory with corresponding headers; and `LinearAllocator`, which allocates and deallocates contiguous ranges of memory.

The `Adapter` class template can be used with `BlockAllocator`, adding an API layer meeting the [C++ named requirements: Allocater](https://en.cppreference.com/w/cpp/named_req/Allocator.html).

## Building

Requires CMake 3.x and a C++17-compatible compiler.

Build from within the project using CMake, or copy the repository into your consuming CMake project, use `add_subdirectory` to add the **memory_allocator** sub-project directory, and add **memory_allocator** to a call to `target_link_libraries`.

## Usage
```cpp
#include <memory_allocator/Adapter.h>
#include <memory_allocator/BlockAllocator.h>

template <typename T> using Allocator = Adapter<T, BlockAllocator>;

// Vector
template <typename T> using StdVector = std::vector<T, Allocator<T>>;

// Map
template <typename K, typename V>
using StdUnorderedMap = std::unordered_map<K, V, std::hash<K>, std::equal_to<K>, Allocator<std::pair<K const, V>>>;

// String
using StdString = std::basic_string<Allocator<char>>;

// Set
template <typename T> using StdSet = std::set<T, std::less<T>, Allocator<T>>;

// etc.
```

## Running Tests

From the project root (replace Ninja with your prefered build system):
```bash
mkdir debug && cd debug
cmake -GNinja ..
ninja
test/mem_alloc_test
```
