#pragma once

#include <cstddef>
#include <iostream>

inline void log_bits(const char *mem, size_t n)
{
#ifndef NDEBUG
    for (size_t i = 0; i < n; ++i)
    {
        for (size_t j = 0; j < 8; ++j)
        {
            std::cout << !!(mem[i] & (1 << (7 - j)));
        }
        std::cout << "\n";
    }
    std::cout << "\n";
#endif
}
