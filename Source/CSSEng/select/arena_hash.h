/*
 * This file is part of CSSEng
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 *
 * Copyright 2015 Michael Drake <tlsa@netsurf-browser.org>
 */

#ifndef css_select_arena_hash_h_
#define css_select_arena_hash_h_

#include <stdint.h>

/**
 * Currently 32-bit MurmurHash2.
 *
 * Created by Austin Appleby, and placed in the public domain.
 *   https://sites.google.com/site/murmurhash/
 *
 * Refactored and adapted a little for libcss.
 */
static inline uint32_t css__arena_hash(const uint8_t *data, size_t len)
{
	/* Hashing constants */
	const uint32_t m = 0x5bd1e995;
	const int r = 24;

	/* Start with length */
	uint32_t h = len;

	/* Hash four bytes at a time */
	while (len >= 4) {
		/* If we could ensure 4-byte alignment of the input, this
		 * could be faster. */
		uint32_t k =
				(((uint32_t) data[0])      ) |
				(((uint32_t) data[1]) <<  8) |
				(((uint32_t) data[2]) << 16) |
				(((uint32_t) data[3]) << 24);

		k *= m;
		k ^= k >> r;
		k *= m;
		h *= m;
		h ^= k;
		data += 4;
		len -= 4;
	}

	/* Hash any left over bytes */
	switch (len) {
	case 3: h ^= data[2] << 16; /* Fall through */
	case 2: h ^= data[1] << 8;  /* Fall through */
	case 1: h ^= data[0];
		h *= m;
	}

	/* Finalise */
	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
}

#endif

