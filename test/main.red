#import "memory"

extern putchar = (c s32) s32;

foo = () s32
{
    var a s32 = 0;
    var b s32 = 1;
    memory.memcpy(a, b, #size(s32));
    return a;
}

main = () s32
{
    var result s32 = foo();
    putchar(result + 48);
    return 0;
}
