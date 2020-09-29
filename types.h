#include <stdint.h>
#define renum typedef enum
#define rstruct typedef struct
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

#define strequal(a, b) strcmp(a, b) == 0
#define COUNT_OF(x) ((sizeof(x))/(sizeof((x[0]))))