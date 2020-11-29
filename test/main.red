#import "imported_module"

extern putchar = (c s8) s32;
main = () s32
{
    const ch s8 = file_function(48);
    putchar(ch);
    return 0;
}
