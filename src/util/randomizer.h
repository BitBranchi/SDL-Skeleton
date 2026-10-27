#pragma once

#include "vector2.hpp"

#include <SDL.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <limits>
#include <random>
#include <type_traits>
#include <utility>

// Header-only RNG following project style. No rand()/srand(), no std::shuffle,
// no standard distributions. Deterministic given the same seed across compilers.

namespace detail_randomizer {

inline constexpr uint64_t splitmix64_constant = 0x9e3779b97f4a7c15ULL;

inline uint64_t rotl64(const uint64_t x, int k)
{
    return (x << k) | (x >> ((64 - k) & 63));
}

// SplitMix64 for expanding seeds (used to initialize xoshiro256**)
inline uint64_t splitmix64_next(uint64_t &x)
{
    uint64_t z = (x += splitmix64_constant);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

struct Xoshiro256ss {
    uint64_t s[4];

    void seed_from_u64(uint64_t seed)
    {
        // Expand seed to 256 bits
        uint64_t sm = seed;
        s[0] = splitmix64_next(sm);
        s[1] = splitmix64_next(sm);
        s[2] = splitmix64_next(sm);
        s[3] = splitmix64_next(sm);
        // If all zero (extremely unlikely), set a default
        if ((s[0] | s[1] | s[2] | s[3]) == 0ULL)
        {
            s[0] = 0x123456789abcdef0ULL;
            s[1] = 0x0fedcba987654321ULL;
            s[2] = 0xabcdef0123456789ULL;
            s[3] = 0x89abcdef01234567ULL;
        }
    }

    uint64_t next()
    {
        const uint64_t result = rotl64(s[1] * 5ULL, 7) * 9ULL;
        const uint64_t t = s[1] << 17;

        s[2] ^= s[0];
        s[3] ^= s[1];
        s[1] ^= s[2];
        s[0] ^= s[3];

        s[2] ^= t;
        s[3] = rotl64(s[3], 45);

        return result;
    }

    // Create an independent child state
    Xoshiro256ss split() const
    {
        Xoshiro256ss child;
        // Derive from current state with splitmix
        uint64_t sm[4];
        sm[0] = s[0];
        sm[1] = s[1];
        sm[2] = s[2];
        sm[3] = s[3];
        child.s[0] = splitmix64_next(sm[0]);
        child.s[1] = splitmix64_next(sm[1]);
        child.s[2] = splitmix64_next(sm[2]);
        child.s[3] = splitmix64_next(sm[3]);
        if ((child.s[0] | child.s[1] | child.s[2] | child.s[3]) == 0ULL)
        {
            child.s[0] = s[3];
            child.s[1] = rotl64(s[2], 13);
            child.s[2] = s[1] ^ s[0];
            child.s[3] = rotl64(s[0], 17);
        }
        return child;
    }
};

// Generate unbiased integer in [0, limit) for limit > 0
inline uint64_t unbiased_below(Xoshiro256ss &gen, uint64_t limit)
{
    if (limit == 0ULL)
        return 0ULL;
    // Find threshold = limit * k >= 2^64 largest such
    const uint64_t threshold = (~limit + 1ULL) % limit;
    for (;;)
    {
        uint64_t r = gen.next();
        if (r >= threshold)
            return r % limit;
    }
}

inline uint64_t default_seed_u64()
{
    // Try random_device first
    try
    {
        std::random_device rd;
        // Mix multiple values
        uint64_t s = (uint64_t)rd();
        s = (s << 32) ^ (uint64_t)rd();
        if (s == 0ULL)
        {
            // fallback to clock
            auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
            s = (uint64_t)now.count();
        }
        return s;
    }
    catch (...)
    {
        auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
        uint64_t s = (uint64_t)now.count();
        // Mix
        s ^= (s << 13);
        s ^= (s >> 7);
        s ^= (s << 17);
        return s;
    }
}

} // namespace detail_randomizer

class Random {
public:
    Random() : m_seed(detail_randomizer::default_seed_u64())
    {
        m_gen.seed_from_u64(m_seed);
    }

    explicit Random(uint64_t seed) : m_seed(seed)
    {
        m_gen.seed_from_u64(m_seed);
    }

    // Reseed the generator
    void seed(uint64_t s)
    {
        m_seed = s;
        m_gen.seed_from_u64(m_seed);
    }

    // Read the seed back
    uint64_t seedValue() const
    {
        return m_seed;
    }

    // Raw uint64_t output
    uint64_t next()
    {
        return m_gen.next();
    }

    // Derive an independent child generator
    Random split() const
    {
        Random child;
        child.m_seed = detail_randomizer::splitmix64_next(const_cast<uint64_t &>(m_seed)); // derive different seed
        // better derive from state
        detail_randomizer::Xoshiro256ss cgen = m_gen.split();
        child.m_gen = cgen;
        // keep a derived seed for seedValue
        uint64_t sm = m_seed;
        child.m_seed = detail_randomizer::splitmix64_next(sm) ^ detail_randomizer::rotl64(m_seed, 17);
        return child;
    }

    // ---------------- Integers ----------------
    // Integer in [0, n)
    uint64_t below(uint64_t n)
    {
        if (n == 0ULL)
            return 0ULL;
        return detail_randomizer::unbiased_below(m_gen, n);
    }

    // Integer between lo and hi. Inclusive of lo, inclusive of hi (documented). Works when lo == hi.
    template <typename Int>
    Int range(Int lo, Int hi)
    {
        static_assert(std::is_integral<Int>::value, "range(lo,hi) for integers requires integral types");
        if (lo == hi)
            return lo;
        if (lo > hi)
            std::swap(lo, hi);
        // inclusive range [lo, hi]
        uint64_t count = (uint64_t)(hi - lo) + 1ULL;
        uint64_t r = below(count);
        return (Int)(lo + (Int)r);
    }

    // ---------------- Floats ----------------
    // float in [0, 1), never exactly 1.0
    float real()
    {
        // Use upper 24 bits for float mantissa
        uint64_t r = m_gen.next();
        // Take top 24 bits
        uint32_t bits = (uint32_t)(r >> 40);
        // [0,1) as float
        const float inv = 1.0f / 16777216.0f; // 2^24
        return (float)bits * inv;
    }

    // double in [0,1)
    double real_d()
    {
        uint64_t r = m_gen.next();
        // Use top 53 bits for double mantissa
        uint64_t bits53 = r >> 11;
        const double inv = 1.0 / ((double)(1ULL << 53));
        return (double)bits53 * inv;
    }

    // float between lo and hi
    float range(float lo, float hi)
    {
        if (lo == hi)
            return lo;
        if (lo > hi)
            std::swap(lo, hi);
        float t = real();
        return lo + (hi - lo) * t;
    }

    double range(double lo, double hi)
    {
        if (lo == hi)
            return lo;
        if (lo > hi)
            std::swap(lo, hi);
        double t = real_d();
        return lo + (hi - lo) * t;
    }

    // ---------------- Booleans ----------------
    bool chance(double p)
    {
        if (p <= 0.0)
            return false;
        if (p >= 1.0)
            return true;
        return real_d() < p;
    }

    bool coin()
    {
        // Use raw bit
        return (m_gen.next() & 1ULL) == 1ULL;
    }

    // ---------------- Collections ----------------
    template <typename Container>
    auto pick(Container &c) -> decltype(*std::begin(c))
    {
        if (std::begin(c) == std::end(c))
        {
            // Defined behavior on empty container - return default constructed
            static typename std::remove_reference<decltype(*std::begin(c))>::type empty_default{};
            return empty_default;
        }
        auto it = std::begin(c);
        size_t sz = 0;
        for (auto tmp = std::begin(c); tmp != std::end(c); ++tmp)
            ++sz;
        uint64_t idx = below(sz);
        std::advance(it, (size_t)idx);
        return *it;
    }

    template <typename Container>
    auto pick(const Container &c) -> decltype(*std::begin(c))
    {
        if (std::begin(c) == std::end(c))
        {
            static typename std::remove_reference<decltype(*std::begin(c))>::type empty_default{};
            return empty_default;
        }
        size_t sz = 0;
        for (auto tmp = std::begin(c); tmp != std::end(c); ++tmp)
            ++sz;
        uint64_t idx = below(sz);
        auto it = std::begin(c);
        std::advance(it, (size_t)idx);
        return *it;
    }

    // Fisher-Yates shuffle
    template <typename RandomIt>
    void shuffle(RandomIt first, RandomIt last)
    {
        if (first == last)
            return;
        auto n = std::distance(first, last);
        for (auto i = n - 1; i > 0; --i)
        {
            uint64_t j = below((uint64_t)i + 1ULL);
            std::iter_swap(first + i, first + (ptrdiff_t)j);
        }
    }

    // weighted: index chosen by weight (handles all-zero and negative weights)
    template <typename Weights>
    size_t weighted(const Weights &weights)
    {
        double sum = 0.0;
        // First pass
        for (const auto &w : weights)
        {
            double val = (double)w;
            if (val > 0.0)
                sum += val;
        }
        if (sum <= 0.0)
        {
            // all zero or negative - pick uniformly among elements
            size_t sz = 0;
            for (const auto &w : weights)
                (void)w, ++sz;
            if (sz == 0)
                return 0;
            return (size_t)below(sz);
        }
        double r = real_d() * sum;
        double acc = 0.0;
        size_t idx = 0;
        for (const auto &w : weights)
        {
            double val = (double)w;
            if (val > 0.0)
            {
                acc += val;
                if (r < acc)
                    return idx;
            }
            ++idx;
        }
        // fallback
        if (idx == 0)
            return 0;
        return idx - 1;
    }

    // ---------------- Geometry (Vector2f) ----------------
    Vector2f unitVector()
    {
        // Uniform angle
        double ang = real_d() * 2.0 * 3.14159265358979323846;
        float c = (float)std::cos(ang);
        float s = (float)std::sin(ang);
        return Vector2f(c, s);
    }

    Vector2f inCircle(float r)
    {
        if (r <= 0.0f)
            return Vector2f(0.0f, 0.0f);
        // Uniform in disk: sqrt(random) * random direction
        float t = std::sqrt(real());
        Vector2f d = unitVector();
        return d * (t * r);
    }

    Vector2f onCircle(float r)
    {
        Vector2f d = unitVector();
        if (r == 0.0f)
            return Vector2f(0.0f, 0.0f);
        return d * r;
    }

    Vector2f inRect(const SDL_Rect &rect)
    {
        float x = (float)rect.x + range(0.0f, (float)rect.w);
        float y = (float)rect.y + range(0.0f, (float)rect.h);
        return Vector2f(x, y);
    }

    // ---------------- Optional extras ----------------
    // Box-Muller
    double normal(double mean = 0.0, double stddev = 1.0)
    {
        // Use cached value if available
        if (m_has_spare_normal)
        {
            m_has_spare_normal = false;
            return mean + stddev * m_spare_normal;
        }
        double u1 = 0.0;
        double u2 = 0.0;
        do
        {
            u1 = real_d();
        } while (u1 <= 0.0); // avoid log(0)
        u2 = real_d();
        double mag = stddev * std::sqrt(-2.0 * std::log(u1));
        double z0 = mag * std::cos(2.0 * 3.14159265358979323846 * u2) + mean;
        double z1 = mag * std::sin(2.0 * 3.14159265358979323846 * u2) + mean;
        m_spare_normal = z1;
        m_has_spare_normal = true;
        return z0;
    }

    // Dice roll
    long long roll(long long count, long long sides)
    {
        if (count <= 0 || sides <= 0)
            return 0LL;
        long long total = 0LL;
        for (long long i = 0; i < count; ++i)
        {
            // 1-based like dice
            total += range(1LL, sides);
        }
        return total;
    }

    // Deterministic hash for per-tile randomness
    uint64_t hash(int x, int y, uint64_t seed)
    {
        // Mix coordinates with seed
        uint64_t h = seed;
        h ^= (uint64_t)(x * 131542391);
        h ^= (uint64_t)(y * 1000000009ULL);
        h = detail_randomizer::rotl64(h, 13);
        h *= 1099511628211ULL; // FNV prime
        h ^= h >> 33;
        h *= 0xff51afd7ed558ccdULL;
        h ^= h >> 33;
        h *= 0xc4ceb9fe1a85ec53ULL;
        h ^= h >> 33;
        return h;
    }

private:
    uint64_t m_seed;
    detail_randomizer::Xoshiro256ss m_gen;
    double m_spare_normal = 0.0;
    bool m_has_spare_normal = false;
};

// Default instance
inline Random rng;