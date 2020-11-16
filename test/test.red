test_return_variable = (v s32) s32
{
    return v;
}

test_increment = (n s32) s32
{
    return n + 1;
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

test_var_concat = (a s32, b s32) s32
{
    var c s32 = a + b;
    var d s32 = c + 5;
    return d;
}

test_var_concat_harder = (a s32, b s32) s32
{
    var c s32 = a + b;
    var d s32 = c + a + 5;
    return d;
}

test_var_concat_empty_decl = (a s32, b s32) s32
{
    var c s32 = a + b;
    var d s32;
    var e s32 = c + 5;
    d = e;
    return d;
}

test_basic_branch = (a s32, b s32) s32
{
    var c s32;
    if a < b
    {
        c = 1;
    }
    else
    {
        c = 0;
    }

    return c;
}

test_branch_early_return = (a s32, b s32) s32
{
    if a < b
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

test_nested_branch_early_return = (a s32, b s32) s32
{
    if a < b
    {
        if a == 6
        {
            return 1;
        }
        else
        {
            return 2;
        }
    }
    else
    {
        return 0;
    }
}

test_unbalanced_branch = (a s32, b s32) s32
{
    if a < b
    {
        return 1;
    }

    return 0;
}

test_branch_return_make_compiler_fail = (a s32, b s32) s32
{
    var c s32 = 0;
    if a < b
    {
        return 1;
    }
    else
    {
        c = 1;
    }

    if c == 1
    {
        return 5;
    }

    return 1;
}

test_heavy_branch_truly_make_compiler_fail = (a s32, b s32) s32
{
    var c s32;
    if a < b
    {
        c = 3;
    }
    else
    {
        if (b > a)
        {
            c = 5;
            return c;
        }
        else
        {
            if a == 5
            {
                c = 15123;
            }
            else
            {
                c = a + b;
                c = c + a + b;
                return 1 + c;
            }
        }
    }

    return a + c;
}
