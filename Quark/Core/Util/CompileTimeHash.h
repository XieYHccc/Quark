#pragma once

#include <stdint.h>
#include <stddef.h>

namespace quark::util{
#ifdef _MSC_VER
// MSVC generates bogus warnings here.
#pragma warning(disable: 4307)
#endif

constexpr uint64_t fnv_iterate(uint64_t hash, uint8_t c)
{
	return (hash * 0x100000001b3ull) ^ c;
}

template<size_t index>
constexpr uint64_t compile_time_fnv1_inner(uint64_t hash, const char *str)
{
	return compile_time_fnv1_inner<index - 1>(fnv_iterate(hash, uint8_t(str[index])), str);
}

template<>
constexpr uint64_t compile_time_fnv1_inner<size_t(-1)>(uint64_t hash, const char *)
{
	return hash;
}

template<size_t len>
constexpr uint64_t compile_time_fnv1(const char (&str)[len])
{
	return compile_time_fnv1_inner<len - 1>(0xcbf29ce484222325ull, str);
}

constexpr uint64_t compile_time_fnv1_merge(uint64_t a, uint64_t b)
{
	return fnv_iterate(
			fnv_iterate(
					fnv_iterate(
							fnv_iterate(
									fnv_iterate(
											fnv_iterate(
													fnv_iterate(
															fnv_iterate(a, uint8_t(b >> 0)),
															uint8_t(b >> 8)),
													uint8_t(b >> 16)),
											uint8_t(b >> 24)),
									uint8_t(b >> 32)),
							uint8_t(b >> 40)),
					uint8_t(b >> 48)),
			uint8_t(b >> 56));
}

constexpr uint64_t compile_time_fnv1_merged(uint64_t hash)
{
	return hash;
}

template <typename T, typename... Ts>
constexpr uint64_t compile_time_fnv1_merged(T hash, T hash2, Ts... hashes)
{
	return compile_time_fnv1_merged(compile_time_fnv1_merge(hash, hash2), hashes...);
}
}