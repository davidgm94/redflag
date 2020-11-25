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

test_first_loop = (number s32) s32
{
    while number < 10
    {
        number = number + 1;
    }

    return number;
}

test_branchy_loop = (n s32) s32
{
    while n < 10
    {
        if n < 10
        {
            n = n * 2;
        }
        n = n + 1;
    }
    return n;
}

test_nested_loops = () s32
{
    var a s32 = 0;
    var b s32 = 0;
    var c s32 = 0;

    while (b < 10000)
    {
        while (a < 100)
        {
            c = a * b;
            b = b + c;
            a = a + 1;
        }
        b = b + 1;
    }

    return b;
}

test_callee = () s32 
{
    return 5;
}

test_fn_function_caller = () s32
{
    return test_callee();
}

test_callee_args = (n s32) s32 
{
    return 5 * n;
}

test_fn_function_caller_args = (n s32) s32
{
    return test_callee_args(n);
}

test_array = (i s32) s32
{
    var a [3]s32 = [0, 1, 2];
    return a[i];
}

test_array_elem_assign = () s32
{
    var arr [2]s32;
    arr[0] = 5;
    return arr[0];
}

test_foo_struct = struct
{
    a s32;
    b s32;
}

test_first_struct = () test_foo_struct
{
    var foo test_foo_struct;
    foo.a = 5;
    foo.b = 3;
    return foo;
}

enum_test = enum
{
    Foo1;
    Foo2;
    Foo3;
}

enum_test2 = enum
{
    Foo2_0 = 0;
    Foo2_1 = 3;
    Foo2_2 = 2;
}

test_first_enum = (a enum_test) s32
{
    if a == enum_test2.Foo2_1
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

test_pointer = (n &s32)
{
    n = 3;
}
