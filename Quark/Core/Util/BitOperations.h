#pragma once

namespace quark::util {

#ifdef __GNUC__
#define leading_zeroes(x) ((x) == 0 ? 32 : __builtin_clz(x))
#define trailing_zeroes(x) ((x) == 0 ? 32 : __builtin_ctz(x))
#define trailing_ones(x) __builtin_ctz(~uint32_t(x))
#define leading_zeroes64(x) ((x) == 0 ? 64 : __builtin_clzll(x))
#define trailing_zeroes64(x) ((x) == 0 ? 64 : __builtin_ctzll(x))
#define trailing_ones64(x) __builtin_ctzll(~uint64_t(x))
#define popcount32(x) __builtin_popcount(x)
#elif defined(_MSC_VER)
namespace Internal {
static inline uint32_t popcount32(uint32_t x)
{
	return __popcnt(x);
}

static inline uint32_t clz(uint32_t x)
{
	unsigned long result;
	if (_BitScanReverse(&result, x))
		return 31 - result;
	else
		return 32;
}

static inline uint32_t ctz(uint32_t x)
{
	unsigned long result;
	if (_BitScanForward(&result, x))
		return result;
	else
		return 32;
}

static inline uint32_t clz64(uint64_t x)
{
	unsigned long result;
	if (_BitScanReverse64(&result, x))
		return 63 - result;
	else
		return 64;
}

static inline uint32_t ctz64(uint64_t x)
{
	unsigned long result;
	if (_BitScanForward64(&result, x))
		return result;
	else
		return 64;
}
}

#define popcount32(x) ::quark::util::Internal::popcount32(x)
#define leading_zeroes(x) ::quark::util::Internal::clz(x)
#define trailing_zeroes(x) ::quark::util::Internal::ctz(x)
#define trailing_ones(x) ::quark::util::Internal::ctz(~uint32_t(x))
#define leading_zeroes64(x) ::quark::util::Internal::clz64(x)
#define trailing_zeroes64(x) ::quark::util::Internal::ctz64(x)
#define trailing_ones64(x) ::quark::util::Internal::ctz64(~uint64_t(x))
#else
#error "Implement me."
#endif

template <typename T>
inline void for_each_bit64(uint64_t value, const T &func)
{
	while (value)
	{
		uint32_t bit = trailing_zeroes64(value);
		func(bit);
		value &= ~(1ull << bit);
	}
}

template <typename T>
inline void for_each_bit(uint32_t value, const T &func)
{
	while (value)
	{
		uint32_t bit = trailing_zeroes(value);
		func(bit);
		value &= ~(1u << bit);
	}
}

template <typename T>
inline void for_each_bit_range(uint32_t value, const T &func)
{
	if (value == ~0u)
	{
		func(0, 32);
		return;
	}

	uint32_t bit_offset = 0;
	while (value)
	{
		uint32_t bit = trailing_zeroes(value);
		bit_offset += bit;
		value >>= bit;
		uint32_t range = trailing_ones(value);
		func(bit_offset, range);
		value &= ~((1u << range) - 1);
	}
}

}