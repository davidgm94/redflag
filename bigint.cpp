//
// Created by david on 7/10/20.
//

#include "bigint.h"

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
            dst->data.digit = digits[0];
        }
    }
}

void BigInt_init_unsigned(BigInt* dst, u64 x)
{
    if (x == 0)
    {
        dst->digit_count = 0;
        dst->is_negative = false;
    }
    dst->digit_count = 1;
    dst->data.digit = x;
    dst->is_negative = false;
}

static bool add_u64_overflow(const u64 op1, const u64 op2, u64 *result)
{
    return __builtin_uaddll_overflow((unsigned long long)op1, (unsigned long long)op2, reinterpret_cast<unsigned long long int *>(result));
}

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
        bool overflow = add_u64_overflow(op1_digits[0], op2_digits[0], &dst->data.digit);
        if (overflow == 0 && op1->digit_count == 1 && op2->digit_count == 1)
        {
            dst->digit_count = 1;
            BigInt_normalize(dst);
            return;
        }
        size_t i = 1;
        u64 first_digit = dst->data.digit;
        //dst->data.digits =
        /* TODO: here*/
    }
}


