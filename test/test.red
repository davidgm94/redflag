test_return_variable = (v s32) s32
{
    return v;
}

test_increment = (n s32) s32
{
    return n - 1;
}

test_decrement = (n s32) s32
{
    return n - 1;
}

test_multiplication = (n s32) s32
{
    return n * 2;
}

test_division = (n s32) s32
{
    return n / 2;
}

test_increment_reverse = (n s32) s32
{
    return 1 + n;
}

test_decrement_reverse = (n s32) s32
{
    return 1 - n;
}

test_mul_reverse = (n s32) s32
{
    return 1 * n;
}

test_div_reverse = (n s32) s32
{
    return 1 / n;
}

test_sum_two_args = (a s32, b s32) s32
{
    return a + b;
}

test_sum_two_args_plus_lit = (a s32, b s32) s32
{
    return a + b + 2;
}

test_sum_three_args = (a s32, b s32, c s32) s32
{
    return a + b + c;
}

test_first_variable = (a s32, b s32) s32
{
    var c s32 = a + b;
    return c;
}
