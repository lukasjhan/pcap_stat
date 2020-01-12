#pragma once

#include "header.hpp"

namespace noname_core {
	namespace network {
		inline uint16_t bswap16(uint16_t value)
		{
			return ((uint16_t)((((value) >> 8) & 0xff)
				| (((value) & 0xff) << 8)));
		}

		inline uint32_t bswap32(uint32_t value)
		{
			return ((((value) & 0xff000000u) >> 24) | (((value) & 0x00ff0000u) >> 8)
				| (((value) & 0x0000ff00u) << 8) | (((value) & 0x000000ffu) << 24));
		}

		inline uint64_t bswap64(uint64_t value)
		{
			return ((((value) & 0xff00000000000000ull) >> 56)
				| (((value) & 0x00ff000000000000ull) >> 40)
				| (((value) & 0x0000ff0000000000ull) >> 24)
				| (((value) & 0x000000ff00000000ull) >> 8)
				| (((value) & 0x00000000ff000000ull) << 8)
				| (((value) & 0x0000000000ff0000ull) << 24)
				| (((value) & 0x000000000000ff00ull) << 40)
				| (((value) & 0x00000000000000ffull) << 56));
		}
	}
}