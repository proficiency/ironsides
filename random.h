#pragma once
#include <random>
#include "typedefs.h"

class RNG
{
    std::random_device m_rd;
    std::mt19937       m_gen;

    template <typename T>
    using Distribution = std::conditional_t<std::is_integral_v<T>, std::uniform_int_distribution<T>, std::uniform_real_distribution<T>>;

    template <typename T>
    Distribution<T> get_distribution(T min, T max)
    {
        static_assert(std::is_arithmetic_v<T>, "arithmetic types are required for rng");
        return Distribution<T>(min, max);
    }

public:
    RNG() : m_gen(m_rd()) {}

    template <typename T>
    T get_random(T min, T max)
    {
        return get_distribution<T>(min, max)(m_gen);
    }
};

inline RNG g_rng;