int test_nested_loops(void)
{
    int a = 0;
    int b = 0;

    while (b < 10000)
    {
        while (a < 100)
        {
            b += a * b;
            a = a + 1;
        }
        b = b + 1;
    }

    return b;
}
#include <stdio.h>
int main()
{
    return printf("%d\n", test_nested_loops());
}
