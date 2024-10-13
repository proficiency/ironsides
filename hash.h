#pragma once
#include <string>
#include <concepts>

#include "typedefs.h"

template <typename T>
concept Hashable = std::unsigned_integral<T> && (sizeof(T) == 4 || sizeof(T) == 8);

class Fnv_Hash
{
public:
    template <Hashable T = u32>
    static constexpr T hash(std::string_view str)
    {
        if constexpr (sizeof(T) == 4)
            return hash32<T>(str);

        else if constexpr (sizeof(T) == 8)
            return hash64<T>(str);
    }

private:
    constexpr static u32 fnv_prime_32 = 0x01000193;
    constexpr static u64 fnv_prime_64 = 0x100000001b3;

    template <Hashable T = u32>
    static constexpr T hash32(std::string_view str)
    {
        T hash = static_cast<T>(0x811c9dc5);
        for (const auto& c : str)
        {
            hash ^= static_cast<T>(c);
            hash *= fnv_prime_32;
        }

        return hash;
    }

    template <Hashable T = u64>
    static constexpr T hash64(std::string_view str)
    {
        T hash = static_cast<T>(0xcbf29ce484222325);
        for (const auto& c : str)
        {
            hash ^= static_cast<T>(c);
            hash *= fnv_prime_64;
        }

        return hash;
    }
};
