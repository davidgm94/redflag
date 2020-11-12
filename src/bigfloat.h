//
// Created by david on 7/10/20.
//

#pragma once

#include "types.h"
#include "compiler_types.h"

void BigFloat_init_16(BigFloat* dst, f16 x);
void BigFloat_init_32(BigFloat* dst, f32 x);
void BigFloat_init_64(BigFloat* dst, f64 x);
void BigFloat_init_128(BigFloat* dst, f128 x);
void BigFloat_init_big_float(BigFloat* dst, f16 x);
void BigFloat_init_big_int(BigFloat* dst, const u8* src_buffer, size_t buffer_length);
Error BigFloat_init_buffer(BigFloat* dst, const u8* src_buffer, size_t buffer_length);

f16 BigFloat_to_f16(const BigFloat* big_float);
f32 BigFloat_to_f32(const BigFloat* big_float);
f64 BigFloat_to_f64(const BigFloat* big_float);
f128 BigFloat_to_f128(const BigFloat* big_float);

void BigFloat_add(BigFloat* dst, const BigFloat* op1, const BigFloat* op2);
void BigFloat_negate(BigFloat* dst, const BigFloat* op);
void BigFloat_sub(BigFloat* dst, const BigFloat* op1, const BigFloat* op2);
void BigFloat_mul(BigFloat* dst, const BigFloat* op1, const BigFloat* op2);
void BigFloat_div(BigFloat* dst, const BigFloat* op1, const BigFloat* op2);
void BigFloat_div_trunc(BigFloat* dst, const BigFloat* op1, const BigFloat* op2);
void BigFloat_div_floor(BigFloat* dst, const BigFloat* op1, const BigFloat* op2);
void BigFloat_rem(BigFloat* dst, const BigFloat* op1, const BigFloat* op2);
void BigFloat_mod(BigFloat* dst, const BigFloat* op1, const BigFloat* op2);
void BigFloat_sqrt(BigFloat* dst, const BigFloat* op);
void BigFloat_append_buffer(BigFloat* dst, const BigFloat* op);
void BigFloat_cmp(const BigFloat* op1, const BigFloat* op2);

bool BigFloat_is_nan(const BigFloat* op);

Cmp BigFloat_cmp_zero(const BigFloat* op);
bool BigFloat_has_fraction(const BigFloat* op);
