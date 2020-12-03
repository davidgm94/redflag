//
// Created by david on 7/10/20.
//

#pragma once

#include "compiler_types.h"
void BigInt_init_unsigned(BigInt* dst, u64 x);

void BigInt_init_signed(BigInt* dst, s64 x);
void BigInt_init_bigint(BigInt* dst, const BigInt* src);
void BigInt_init_bigfloat(BigInt* dst, const BigFloat* op);
// Not used
void BigInt_init_data(BigInt* dst, const u64* digits, size_t digit_count, bool is_negative);
void BigInt_deinit(BigInt* i);

u64 BigInt_as_u64(const BigInt* big_int);
u32 BigInt_as_u32(const BigInt* big_int);
size_t BigInt_as_usize(const BigInt* big_int);
s64 BigInt_as_signed(const BigInt* big_int);

static inline const u64* bigint_ptr(const BigInt* big_int)
{
    if (big_int->digit_count == 1)
    {
        return &big_int->digit;
    }
    else
    {
        return big_int->digits;
    }
}

bool BigInt_fits_in_bits(const BigInt* bn, size_t bit_count, bool is_signed);
void BigInt_write_twos_complement(const BigInt* big_int, u8* src_buffer, size_t bit_count, bool is_big_endian);
void BigInt_read_twos_complement(BigInt* dst, const u8* src_buffer, size_t big_count, bool is_big_endian, bool is_signed);
void BigInt_add(BigInt* dst, const BigInt* op1, const BigInt* op2);
void BigInt_add_wrap(BigInt* dst, const BigInt* op1, const BigInt* op2, size_t bit_count, bool is_signed);
void BigInt_sub(BigInt* dst, const BigInt* op1, const BigInt* op2);
void BigInt_sub_wrap(BigInt* dst, const BigInt* op1, const BigInt* op2, size_t bit_count, bool is_signed);
void BigInt_mul(BigInt* dst, const BigInt* op1, const BigInt* op2);
void BigInt_mul_wrap(BigInt* dst, const BigInt* op1, const BigInt* op2, size_t bit_count, bool is_signed);
void BigInt_div_trunc(BigInt* dst, const BigInt* op1, const BigInt* op2);
void BigInt_div_floor(BigInt* dst, const BigInt* op1, const BigInt* op2);
void BigInt_rem(BigInt* dst, const BigInt* op1, const BigInt* op2);
void BigInt_mod(BigInt* dst, const BigInt* op1, const BigInt* op2);

void BigInt_or(BigInt* dst, const BigInt* op1, const BigInt* op2);
void BigInt_and(BigInt* dst, const BigInt* op1, const BigInt* op2);
void BigInt_xor(BigInt* dst, const BigInt* op1, const BigInt* op2);

void BigInt_shl(BigInt* dst, const BigInt* op1, const BigInt* op2);
void BigInt_shl_trunc(BigInt* dst, const BigInt* op1, const BigInt* op2, size_t bit_count, bool is_signed);
void BigInt_shr(BigInt* dst, const BigInt* op1, const BigInt* op2);

void BigInt_negate(BigInt* dst, const BigInt* op);
void BigInt_negate_wrap(BigInt* dst, const BigInt* op, size_t bit_count);
void BigInt_not(BigInt* dst, const BigInt* op, size_t bit_count, bool is_signed);
void BigInt_truncate(BigInt* dst, const BigInt* op, size_t bit_count, bool is_signed);

Cmp BigInt_cmp(const BigInt* op1, const BigInt* op2);

void BigInt_append_buffer(SB* src_buffer, const BigInt* op, u64 base);

size_t BigInt_ctz(const BigInt* big_int, size_t bit_count);
size_t BigInt_clz(const BigInt* big_int, size_t bit_count);
size_t BigInt_popcount_signed(const BigInt* big_int, size_t bit_count);
size_t BigInt_popcount_unsigned(const BigInt* big_int);

size_t BigInt_bits_needed(const BigInt* op);

Cmp BigInt_cmp_zero(const BigInt* op);

void BigInt_incr(BigInt* fn_handle);
void BigInt_decr(BigInt* fn_handle);

bool mul_u64_overflow(u64 op1, u64 op2, u64* result);

u32 BigInt_hash(BigInt n);
bool BigInt_eql(BigInt a, BigInt b);

