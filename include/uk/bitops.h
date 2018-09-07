/* SPDX-License-Identifier: BSD-2-Clause */
/*-
 * Copyright (c) 2010 Isilon Systems, Inc.
 * Copyright (c) 2010 iX Systems, Inc.
 * Copyright (c) 2010 Panasas, Inc.
 * Copyright (c) 2013-2017 Mellanox Technologies, Ltd.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD$
 */
#ifndef	_LINUX_BITOPS_H_
#define	_LINUX_BITOPS_H_

#include <sys/param.h>
#include <errno.h>
#include <uk/essentials.h>
#include <uk/bitcount.h>
#include <uk/arch/lcpu.h>
#include <uk/arch/atomic.h>

#define	BIT(nr)			(1UL << (nr))
#define	BIT_ULL(nr)		(1ULL << (nr))
#ifdef __LP64__
#define	BITS_PER_LONG		64
#else
#define	BITS_PER_LONG		32
#endif

#define	BITS_PER_LONG_LONG	64

#define	BITMAP_FIRST_WORD_MASK(start)  (~0UL << ((start) % BITS_PER_LONG))
#define	BITMAP_LAST_WORD_MASK(n)       (~0UL >> (BITS_PER_LONG - (n)))
#define	BITS_TO_LONGS(n)               howmany((n), BITS_PER_LONG)
#define	BIT_MASK(nr)                   (1UL << ((nr) & (BITS_PER_LONG - 1)))
#define BIT_WORD(nr)                   ((nr) / BITS_PER_LONG)
#define	GENMASK(h, l) \
	(((~0UL) >> (BITS_PER_LONG - (h) - 1)) & ((~0UL) << (l)))
#define	GENMASK_ULL(h, l) \
	(((~0ULL) >> (BITS_PER_LONG_LONG - (h) - 1)) & ((~0ULL) << (l)))
#define BITS_PER_BYTE  8

#define	hweight8(x)	uk_bitcount((uint8_t)(x))
#define	hweight16(x)	uk_bitcount16(x)
#define	hweight32(x)	uk_bitcount32(x)
#define	hweight64(x)	uk_bitcount64(x)
#define	hweight_long(x)	uk_bitcountl(x)

#if 0 /* TODO revisit when needed */
static inline int
fls64(__u64 mask)
{
	return flsll(mask);
}
#endif

static inline __u32
ror32(__u32 word, unsigned int shift)
{
	return ((word >> shift) | (word << (32 - shift)));
}

static inline int get_count_order(unsigned int count)
{
	int order;

	order = ukarch_fls(count);
	if (count & (count - 1))
		order++;
	return order;
}

static inline unsigned long
find_first_bit(const unsigned long *addr, unsigned long size)
{
	long mask;
	int bit;

	for (bit = 0; size >= BITS_PER_LONG;
		size -= BITS_PER_LONG, bit += BITS_PER_LONG, addr++) {
		if (*addr == 0)
			continue;
		return (bit + ukarch_ffsl(*addr));
	}
	if (size) {
		mask = (*addr) & BITMAP_LAST_WORD_MASK(size);
		if (mask)
			bit += ukarch_ffsl(mask);
		else
			bit += size;
	}
	return (bit);
}

static inline unsigned long
find_first_zero_bit(const unsigned long *addr, unsigned long size)
{
	long mask;
	int bit;

	for (bit = 0; size >= BITS_PER_LONG;
		size -= BITS_PER_LONG, bit += BITS_PER_LONG, addr++) {
		if (~(*addr) == 0)
			continue;
		return (bit + ukarch_ffsl(~(*addr)));
	}
	if (size) {
		mask = ~(*addr) & BITMAP_LAST_WORD_MASK(size);
		if (mask)
			bit += ukarch_ffsl(mask);
		else
			bit += size;
	}
	return (bit);
}

static inline unsigned long
find_last_bit(const unsigned long *addr, unsigned long size)
{
	long mask;
	int offs;
	int bit;
	int pos;

	pos = size / BITS_PER_LONG;
	offs = size % BITS_PER_LONG;
	bit = BITS_PER_LONG * pos;
	addr += pos;
	if (offs) {
		mask = (*addr) & BITMAP_LAST_WORD_MASK(offs);
		if (mask)
			return (bit + ukarch_flsl(mask));
	}
	while (pos--) {
		addr--;
		bit -= BITS_PER_LONG;
		if (*addr)
			return (bit + ukarch_flsl(*addr));
	}
	return (size);
}

static inline unsigned long
find_next_bit(const unsigned long *addr, unsigned long size,
	unsigned long offset)
{
	long mask;
	int offs;
	int bit;
	int pos;

	if (offset >= size)
		return (size);
	pos = offset / BITS_PER_LONG;
	offs = offset % BITS_PER_LONG;
	bit = BITS_PER_LONG * pos;
	addr += pos;
	if (offs) {
		mask = (*addr) & ~BITMAP_LAST_WORD_MASK(offs);
		if (mask)
			return (bit + ukarch_ffsl(mask));
		if (size - bit <= BITS_PER_LONG)
			return (size);
		bit += BITS_PER_LONG;
		addr++;
	}
	for (size -= bit; size >= BITS_PER_LONG;
		size -= BITS_PER_LONG, bit += BITS_PER_LONG, addr++) {
		if (*addr == 0)
			continue;
		return (bit + ukarch_ffsl(*addr));
	}
	if (size) {
		mask = (*addr) & BITMAP_LAST_WORD_MASK(size);
		if (mask)
			bit += ukarch_ffsl(mask);
		else
			bit += size;
	}
	return (bit);
}

static inline unsigned long
find_next_zero_bit(const unsigned long *addr, unsigned long size,
	unsigned long offset)
{
	long mask;
	int offs;
	int bit;
	int pos;

	if (offset >= size)
		return (size);
	pos = offset / BITS_PER_LONG;
	offs = offset % BITS_PER_LONG;
	bit = BITS_PER_LONG * pos;
	addr += pos;
	if (offs) {
		mask = ~(*addr) & ~BITMAP_LAST_WORD_MASK(offs);
		if (mask)
			return (bit + ukarch_ffsl(mask));
		if (size - bit <= BITS_PER_LONG)
			return (size);
		bit += BITS_PER_LONG;
		addr++;
	}
	for (size -= bit; size >= BITS_PER_LONG;
		size -= BITS_PER_LONG, bit += BITS_PER_LONG, addr++) {
		if (~(*addr) == 0)
			continue;
		return (bit + ukarch_ffsl(~(*addr)));
	}
	if (size) {
		mask = ~(*addr) & BITMAP_LAST_WORD_MASK(size);
		if (mask)
			bit += ukarch_ffsl(mask);
		else
			bit += size;
	}
	return (bit);
}

/* set_bit and clear_bit are atomic and protected against
 * reordering (do barriers), while the underscored (__*) versions of
 * them don't (not atomic).
 */
#define __set_bit(i, a)        ukarch_set_bit(i, a)
#define set_bit(i, a)          ukarch_set_bit_sync(i, a)
#define __clear_bit(i, a)      ukarch_clr_bit(i, a)
#define clear_bit(i, a)        ukarch_clr_bit_sync(i, a)

static inline int
test_and_clear_bit(long bit, volatile unsigned long *var)
{
	return ukarch_test_and_clr_bit_sync(bit, (volatile void *) var);
}

static inline int
__test_and_clear_bit(long bit, volatile unsigned long *var)
{
	return ukarch_test_and_clr_bit(bit, (volatile void *) var);
}

static inline int
test_and_set_bit(long bit, volatile unsigned long *var)
{
	return ukarch_test_and_set_bit_sync(bit, (volatile void *) var);
}

static inline int
__test_and_set_bit(long bit, volatile unsigned long *var)
{
	return ukarch_test_and_set_bit(bit, (volatile void *) var);
}

enum {
	REG_OP_ISFREE,
	REG_OP_ALLOC,
	REG_OP_RELEASE,
};

static inline int
linux_reg_op(unsigned long *bitmap, int pos, int order, int reg_op)
{
	int nbits_reg;
	int index;
	int offset;
	int nlongs_reg;
	int nbitsinlong;
	unsigned long mask;
	int i;
	int ret = 0;

	nbits_reg = 1 << order;
	index = pos / BITS_PER_LONG;
	offset = pos - (index * BITS_PER_LONG);
	nlongs_reg = BITS_TO_LONGS(nbits_reg);
	nbitsinlong = MIN(nbits_reg,  BITS_PER_LONG);

	mask = (1UL << (nbitsinlong - 1));
	mask += mask - 1;
	mask <<= offset;

	switch (reg_op) {
	case REG_OP_ISFREE:
		for (i = 0; i < nlongs_reg; i++) {
			if (bitmap[index + i] & mask)
				goto done;
		}
		ret = 1;
		break;

	case REG_OP_ALLOC:
		for (i = 0; i < nlongs_reg; i++)
			bitmap[index + i] |= mask;
		break;

	case REG_OP_RELEASE:
		for (i = 0; i < nlongs_reg; i++)
			bitmap[index + i] &= ~mask;
		break;
	}
done:
	return ret;
}

#define for_each_set_bit(bit, addr, size) \
	for ((bit) = find_first_bit((addr), (size));		\
	     (bit) < (size);					\
	     (bit) = find_next_bit((addr), (size), (bit) + 1))

#define	for_each_clear_bit(bit, addr, size) \
	for ((bit) = find_first_zero_bit((addr), (size));		\
	     (bit) < (size);						\
	     (bit) = find_next_zero_bit((addr), (size), (bit) + 1))

static inline __u64
sign_extend64(__u64 value, int index)
{
	__u8 shift = 63 - index;

	return ((__s64)(value << shift) >> shift);
}

#endif	/* _LINUX_BITOPS_H_ */