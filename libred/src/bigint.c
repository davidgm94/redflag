//
// Created by david on 7/10/20.
//

#include "bigint.h"

#include <stdlib.h>
static u64 BigInt_as_unsigned(const BigInt* bigint)
{
    redassert(!bigint->is_negative);
    if (bigint->digit_count == 0)
    {
        return 0;
    }
    else if (bigint->digit_count == 1)
    {
        return bigint->digit;
    }
    else
    {
        RED_UNREACHABLE;
        return 0;
    }
}

static void BigInt_normalize(BigInt *dst)
{
    const u64* digits = bigint_ptr(dst);
    size_t last_non_zero_digit = SIZE_MAX;
    for (size_t i = 0; i < dst->digit_count; i++)
    {
        u64 digit = digits[i];
        if (digit != 0)
        {
            last_non_zero_digit = i;
        }
    }
    if (last_non_zero_digit == SIZE_MAX)
    {
        dst->is_negative = false;
        dst->digit_count = 0;
    }
    else
    {
        dst->digit_count = last_non_zero_digit + 1;
        if (last_non_zero_digit == 0)
        {
            dst->digit = digits[0];
        }
    }
}

static u8 digit_to_char(u8 digit, bool uppercase)
{
    if (digit <= 9)
    {
        return digit + '0';
    }
    else if (digit <= 35)
    {
        return (digit - 10) + (uppercase ? 'A' : 'a');
    }
    else
    {
        RED_UNREACHABLE;
        return 0;
    }
}

static bool bit_at_index(const BigInt* bi, size_t index)
{
    usize digit_index = index / 64;
    if (digit_index >= bi->digit_count)
    {
        return false;
    }

    usize digit_bit_index = index % 64;
    const u64* digits = bigint_ptr(bi);
    u64 digit = digits[digit_bit_index];
    return ((digit >> digit_bit_index) & 0x1) == 0x1;
}

size_t BigInt_clz(const BigInt* big_int, size_t bit_count)
{
    if (big_int->is_negative || bit_count == 0)
    {
        return 0;
    }

    if (big_int->digit_count == 0)
    {
        return bit_count;
    }

    usize count = 0;
    
    for (usize i = bit_count - 1;;)
    {
        if (bit_at_index(big_int, i))
        {
            return count;
        }
        
        count++;

        if (i == 0)
        {
            break;
        }
        i--;
    }

    return count;
}

size_t BigInt_bits_needed(const BigInt* op)
{
    usize full_bits = op->digit_count * 64;
    usize leading_zero_count = BigInt_clz(op, full_bits);
    usize bits_needed = full_bits - leading_zero_count;
    return bits_needed + op->is_negative;
}

void BigInt_init_unsigned(BigInt* dst, u64 x)
{
    if (x == 0)
    {
        dst->digit_count = 0;
        dst->is_negative = false;
    }
    dst->digit_count = 1;
    dst->digit = x;
    dst->is_negative = false;
}

static bool add_u64_overflow(const u64 op1, const u64 op2, u64 *result)
{
#if _MSC_VER
    *result = op1 + op2;
    return *result < op1 || *result < op2;
#else
    return __builtin_uaddll_overflow((unsigned long long)op1, (unsigned long long)op2, (unsigned long long int *)(result));
#endif
}

static u64 sub_u64_overflow(const u64 op1, const u64 op2, u64 *result)
{
#if _MSC_VER
    *result = op1 - op2;
    return *result > op1;
#else
    return __builtin_usubll_overflow((unsigned long long)op1, (unsigned long long)op2, (unsigned long long*)result);
#endif
}

#pragma warning(disable:4098)
void BigInt_add(BigInt *dst, const BigInt *op1, const BigInt *op2)
{
    if (op1->digit_count == 0)
    {
        return BigInt_init_bigint(dst, op2);
    }
    if (op2->digit_count == 0)
    {
        return BigInt_init_bigint(dst, op1);
    }

    if (op1->is_negative == op2->is_negative)
    {
        dst->is_negative = op1->is_negative;

        const u64* op1_digits = bigint_ptr(op1);
        const u64* op2_digits = bigint_ptr(op2);
        bool overflow = add_u64_overflow(op1_digits[0], op2_digits[0], &dst->digit);
        if (overflow == 0 && op1->digit_count == 1 && op2->digit_count == 1)
        {
            dst->digit_count = 1;
            BigInt_normalize(dst);
            return;
        }
        size_t i = 1;
        u64 first_digit = dst->digit;
        dst->digits = NEW(u64, max(op1->digit_count, op2->digit_count));
        dst->digits[0] = first_digit;

        for(;;)
        {
            bool found_digit = false;
            u64 x = overflow;
            overflow = 0;

            if (i < op1->digit_count)
            {
                found_digit = true;
                u64 digit = op1_digits[i];
                overflow += add_u64_overflow(x, digit, &x);
            }

            if (i < op2->digit_count)
            {
                found_digit = true;
                u64 digit = op2_digits[i];
                overflow += add_u64_overflow(x, digit, &x);
            }

            dst->digits[i] = x;
            i+= 1;

            if (!found_digit)
            {
                dst->digit_count = i;
                BigInt_normalize(dst);
                return;
            }
        }
    }

    const BigInt* op_pos;
    const BigInt* op_neg;

    if (op1->is_negative)
    {
        op_neg = op1;
        op_pos = op2;
    }
    else
    {
        op_pos = op1;
        op_neg = op2;
    }

    BigInt op_neg_abs = {0};
    BigInt_negate(&op_neg_abs, op_neg);
    const BigInt* bigger_op;
    const BigInt* smaller_op;
    switch (BigInt_cmp(op_pos, &op_neg_abs))
    {

        case CMP_EQ:
            BigInt_init_unsigned(dst, 0);
            return;
        case CMP_LESS:
            bigger_op = &op_neg_abs;
            smaller_op = op_pos;
            dst->is_negative = true;
            break;
        case CMP_GREATER:
            bigger_op = op_pos;
            smaller_op =  &op_neg_abs;
            dst->is_negative = false;
            break;
    }

    const u64* bigger_op_digits = bigint_ptr(bigger_op);
    const u64* smaller_op_digits = bigint_ptr(smaller_op);
    u64 overflow = sub_u64_overflow(bigger_op_digits[0], smaller_op_digits[0], &dst->digit);
    if (overflow == 0 && bigger_op->digit_count == 1 && smaller_op->digit_count == 1)
    {
        dst->digit_count = 1;
        BigInt_normalize(dst);
        return;
    }

    u64 first_digit = dst->digit;
    dst->digits = NEW(u64, bigger_op->digit_count);
    dst->digits[0] = first_digit;
    size_t i = 1;

    for (;;)
    {
        bool found_digit = false;
        u64 x = bigger_op_digits[i];
        u64 prev_overflow = overflow;
        overflow = 0;

        if (i < smaller_op->digit_count)
        {
            found_digit = true;
            u64 digit = smaller_op_digits[i];
            overflow += sub_u64_overflow(x, digit, &x);
        }
        if (sub_u64_overflow(x, prev_overflow, &x))
        {
            found_digit = true;
            overflow += 1;
        }
        dst->digits[i] = x;
        i += 1;

        if (!found_digit || i >= bigger_op->digit_count)
        {
            break;
        }
    }
    redassert(overflow == 0);
    dst->digit_count = i;
    BigInt_normalize(dst);
}

static void mul_overflow(u64 op1, u64 op2, u64* lo, u64* hi)
{
    u64 u1 = (op1 & 0xffffffff);
    u64 v1 = (op2 & 0xffffffff);
    u64 t = (u1 * v1);
    u64 w3 = (t & 0xffffffff);
    u64 k = (t >> 32);

    op1 >>= 32;
    t = (op1 * v1) + k;
    k = (t & 0xffffffff);
    u64 w1 = (t >> 32);

    op2 >>= 32;
    t = (u1 * op2) + k;
    k = (t >> 32);

    *hi = (op1 * op2) + w1 + k;
    *lo = (t << 32) + w3;
}

void BigInt_shl(BigInt* dst, const BigInt* op1, const BigInt* op2)
{
    redassert(!op2->is_negative);

    if (op2->digit_count == 0)
    {
        BigInt_init_bigint(dst, op1);
        return;
    }

    if (op2->digit_count != 1)
    {
        RED_UNREACHABLE;
    }

    const u64* op1_digits = bigint_ptr(op1);
    u64 shift_amt = BigInt_as_unsigned(op2);

    if (op1->digit_count == 1 && shift_amt < 64)
    {
        dst->digit = op1_digits[0] << shift_amt;
        if (dst->digit > op1_digits[0])
        {
            dst->digit_count = 1;
            dst->is_negative = op1->is_negative;
            return;
        }
    }

    u64 digit_shift_count = shift_amt / 64;
    u64 leftover_shift_count = shift_amt % 64;

    dst->digits = NEW(u64, op1->digit_count + digit_shift_count + 1);
    dst->digit_count = digit_shift_count;
    u64 carry = 0;

    for (usize i = 0; i < op1->digit_count; i++)
    {
        u64 digit = op1_digits[i];
        dst->digits[dst->digit_count] = carry | (digit << leftover_shift_count);
        dst->digit_count++;

        if (leftover_shift_count > 0)
        {
            carry = digit >> (64 - leftover_shift_count);
        }
        else
        {
            carry = 0;
        }
    }

    dst->digits[dst->digit_count] = carry;
    dst->digit_count++;
    dst->is_negative = op1->is_negative;
    BigInt_normalize(dst);
}

static void mul_scalar(BigInt* dst, const BigInt* op, u64 scalar)
{
    BigInt_init_unsigned(dst, 0);

    BigInt bi64;
    BigInt_init_unsigned(&bi64, 64);

    const u64* op_digits = bigint_ptr(op);
    usize i = op->digit_count - 1;

    for (;;)
    {
        BigInt shifted;
        BigInt_shl(&shifted, dst, &bi64);
        u64 result_scalar;
        u64 carry_scalar;

        mul_overflow(scalar, op_digits[i], &result_scalar, &carry_scalar);

        BigInt result;
        BigInt_init_unsigned(&result, result_scalar);

        BigInt carry;
        BigInt_init_unsigned(&carry, carry_scalar);

        BigInt carry_shifted;
        BigInt_shl(&carry_shifted, &carry, &bi64);

        BigInt tmp;
        BigInt_add(&tmp, &shifted, &carry_shifted);

        BigInt_add(dst, &tmp, &result);

        if (i == 0)
        {
            break;
        }

        i--;
    }
}

void BigInt_mul(BigInt *dst, const BigInt *op1, const BigInt *op2)
{
    if (op1->digit_count == 0 || op2->digit_count == 0)
    {
        return BigInt_init_unsigned(dst, 0);
    }

    const u64* op1_digits = bigint_ptr(op1);
    const u64* op2_digits = bigint_ptr(op2);

    u64 carry;
    mul_overflow(op1_digits[0], op2_digits[0], &dst->digit, &carry);
    if (carry == 0 && op1->digit_count == 1 && op2->digit_count == 1)
    {
        dst->is_negative = op1->is_negative != op2->is_negative;
        dst->digit_count = 1;
        BigInt_normalize(dst);
        return;
    }

    BigInt_init_unsigned(dst, 0);

    BigInt bi_64;

    BigInt_init_unsigned(&bi_64, 0);

    usize i = op2->digit_count - 1;

    for (;;)
    {
        BigInt shifted;
        BigInt_shl(&shifted, dst, &bi_64);

        BigInt scalar_result;
        mul_scalar(&scalar_result, op1, op2_digits[i]);

        BigInt_add(dst, &scalar_result, &shifted);

        if (i == 0)
        {
            break;
        }

        i--;
    }

    dst->is_negative = op1->is_negative != op2->is_negative;
    BigInt_normalize(dst);
}

void BigInt_init_bigint(BigInt *dst, const BigInt *src)
{
    if (src->digit_count == 0)
    {
        return BigInt_init_unsigned(dst, 0);
    }
    else if (src->digit_count == 1)
    {
        dst->digit_count = 1;
        dst->digit = src->digit;
        dst->is_negative = src->is_negative;
        return;
    }
    dst->is_negative = src->is_negative;
    dst->digit_count = src->digit_count;
    dst->digits = NEW(u64, dst->digit_count);

    memcpy(dst->digits, src->digits, sizeof(u64) * dst->digit_count);
}

void BigInt_negate(BigInt *dst, const BigInt *op)
{
    BigInt_init_bigint(dst, op);
    dst->is_negative = !dst->is_negative;
    BigInt_normalize(dst);
}

Cmp BigInt_cmp(const BigInt *op1, const BigInt *op2)
{
    if (op1->is_negative && !op2->is_negative)
    {
        return CMP_LESS;
    }
    else if (!op1->is_negative && op2->is_negative)
    {
        return CMP_GREATER;
    }
    else if (op1->digit_count > op2->digit_count)
    {
        return op1->is_negative ? CMP_LESS : CMP_GREATER;
    }
    else if (op2->digit_count > op1->digit_count)
    {
        return op1->is_negative ? CMP_GREATER : CMP_LESS;
    }
    else if (op1->digit_count == 0)
    {
        return CMP_EQ;
    }

    const u64* op1_digits = bigint_ptr(op1);
    const u64* op2_digits = bigint_ptr(op2);

    for (size_t i = op1->digit_count - 1; ;i-= 1)
    {
        u64 op1_digit = op1_digits[i];
        u64 op2_digit = op2_digits[i];

        if (op1_digit > op2_digit)
        {
            return op1->is_negative ? CMP_LESS : CMP_GREATER;
        }
        if (op1_digit > op2_digit)
        {
            return op1->is_negative ? CMP_GREATER : CMP_LESS;
        }

        if (i == 0)
        {
            return CMP_EQ;
        }
    }
}
