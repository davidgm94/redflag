//
// Created by david on 8/10/20.
//
#pragma once

#include "types.h"
#include <string.h>


typedef struct StringBuffer
{
    char* ptr;
    u32 len; /* length */
    u32 cap; /* capacity */
} SB, StringBuffer;

typedef enum Error
{
    ERROR_NONE,
    ERROR_NO_MEM,
    ERROR_INVALID_FORMAT,
    ERROR_SEMANTIC_ANALYZEFAIL,
    ERROR_ACCESS,
    ERROR_INTERRUPTED,
    ERROR_SYSTEM_RESOURCES,
    ERROR_FILE_NOT_FOUND,
    ERROR_FILE_SYSTEM,
    ERROR_FILE_TOO_BIG,
    ERROR_DIV_BY_ZERO,
    ERROR_OVERFLOW,
    ERROR_PATH_ALREADY_EXISTS,
    ERROR_UNEXPECTED,
    ERROR_EXACT_DIV_REMAINDER,
    ERROR_NEGATIVE_DENOMINATOR,
    ERROR_SHIFTED_OUT_ONEBITS,
    ERROR_C_COMPILE_ERRORS,
    ERROR_END_OF_FILE,
    ERROR_IS_DIR,
    ERROR_NOT_DIR,
    ERROR_UNSUPPORTED_OPERATING_SYSTEM,
    ERROR_SHARING_VIOLATION,
    ERROR_PIPE_BUSY,
    ERROR_PRIMITIVE_TYPE_NOTFOUND,
    ERROR_CACHE_UNAVAILABLE,
    ERROR_PATH_TOO_LONG,
    ERROR_C_COMPILER__CANNOT_FIND_FILE,
    ERROR_NO_C_COMPILER_INSTALLED,
    ERROR_READING_DEP_FILE,
    ERROR_INVALID_DEP_FILE,
    ERROR_MISSING_ARCHITECTURE,
    ERROR_MISSING_OPERATING_SYSTEM,
    ERROR_UNKNOWN_ARCHITECTURE,
    ERROR_UNKNOWN_OPERATING_SYSTEM,
    ERROR_UNKNOWN_ABI,
    ERROR_INVALID_FILENAME,
    ERROR_DISK_QUOTA,
    ERROR_DISK_SPACE,
    ERROR_UNEXPECTED_WRITE_FAILURE,
    ERROR_UNEXPECTED_SEEK_FAILURE,
    ERROR_UNEXPECTED_FILE_TRUNCATION_FAILURE,
    ERROR_UNIMPLEMENTED,
    ERROR_OPERATION_ABORTED,
    ERROR_BROKEN_PIPE,
    ERROR_NO_SPACE_LEFT,
    ERROR_NOT_LAZY,
    ERROR_IS_ASYNC,
    ERROR_IMPORT_OUTSIDE_PKG_PATH,
    ERROR_UNKNOWN_CPU,
    ERROR_UNKNOWN_CPU_FEATURE,
    ERROR_INVALID_CPU_FEATURES,
    ERROR_INVALID_LLVM_CPU_FEATURES_FORMAT,
    ERROR_UNKNOWN_APPLICATION_BINARY_INTERFACE,
    ERROR_AST_UNIT_FAILURE,
    ERROR_BAD_PATH_NAME,
    ERROR_SYM_LINK_LOOP,
    ERROR_PROCESS_FD_QUOTA_EXCEEDED,
    ERROR_SYSTEM_FD_QUOTA_EXCEEDED,
    ERROR_NO_DEVICE,
    ERROR_DEVICE_BUSY,
    ERROR_UNABLE_TO_SPAWN_C_COMPILER,
    ERROR_C_COMPILER_EXIT_CODE,
    ERROR_C_COMPILER_CRASHED,
    ERROR_C_COMPILER_CANNOT_FIND_HEADERS,
    ERROR_LIB_C_RUNTIME_NOT_FOUND,
    ERROR_LIB_C_STD_LIB_HEADER_NOT_FOUND,
    ERROR_LIB_C_KERNEL32_LIB_NOT_FOUND,
    ERROR_UNSUPPORTED_ARCHITECTURE,
    ERROR_WINDOWS_SDK_NOT_FOUND,
    ERROR_UNKNOWN_DYNAMIC_LINKER_PATH,
    ERROR_TARGET_HAS_NO_DYNAMIC_LINKER,
    ERROR_INVALID_ABI_VERSION,
    ERROR_INVALID_OPERATING_SYSTEM_VERSION,
    ERROR_UNKNOWN_CLANG_OPTION,
    ERROR_NESTED_RESPONSE_FILE,
    ERROR_ZIG_IS_THEC_COMPILER,
    ERROR_FILE_BUSY,
    ERROR_LOCKED,
} Error;

typedef enum Cmp
{
    CMP_LESS,
    CMP_GREATER,
    CMP_EQ
} Cmp;

typedef enum TokenID
{
    TOKEN_ID_AMPERSAND = '&',
    TOKEN_ID_AT = '@',
    TOKEN_ID_BANG = '!',
    TOKEN_ID_BAR = '|',
    TOKEN_ID_CARET = '^',
    TOKEN_ID_CMP_GREATER = '>',
    TOKEN_ID_CMP_LESS = '<',
    TOKEN_ID_COLON = ':',
    TOKEN_ID_COMMA = ',',
    TOKEN_ID_DASH = '-',
    TOKEN_ID_HASH = '#',
    TOKEN_ID_EQ = '=',
    TOKEN_ID_DOT = '.',
    TOKEN_ID_LEFT_BRACE = '{',
    TOKEN_ID_LEFT_BRACKET = '[',
    TOKEN_ID_LEFT_PARENTHESIS = '(',
    TOKEN_ID_QUESTION = '?',
    TOKEN_ID_PERCENT = '%',
    TOKEN_ID_PLUS = '+',
    TOKEN_ID_RIGHT_BRACE = '}',
    TOKEN_ID_RIGHT_BRACKET = ']',
    TOKEN_ID_RIGHT_PARENTHESIS = ')',
    TOKEN_ID_SEMICOLON = ';',
    TOKEN_ID_SLASH = '/',
    TOKEN_ID_STAR = '*',
    TOKEN_ID_TILDE = '~',
    TOKEN_ID_ARROW = 256,
    TOKEN_ID_BIT_OR_EQ,
    TOKEN_ID_BIT_XOR_EQ,
    TOKEN_ID_BIT_AND_EQ,
    TOKEN_ID_BIT_SHL,
    TOKEN_ID_BIT_SHL_EQ,
    TOKEN_ID_BIT_SHR,
    TOKEN_ID_BIT_SHR_EQ,
    TOKEN_ID_CHAR_LIT,
    TOKEN_ID_CMP_EQ,
    TOKEN_ID_CMP_GREATER_OR_EQ,
    TOKEN_ID_CMP_LESS_OR_EQ,
    TOKEN_ID_CMP_NOT_EQ,
    TOKEN_ID_DIV_EQ,
    TOKEN_ID_END_OF_FILE,
    TOKEN_ID_FAT_ARROW,
    TOKEN_ID_FLOAT_LIT,
    TOKEN_ID_INT_LIT,
    TOKEN_ID_KEYWORD_ALIGN,
    TOKEN_ID_KEYWORD_ALLOW_ZERO,
    TOKEN_ID_KEYWORD_AND,
    TOKEN_ID_KEYWORD_ANY,
    TOKEN_ID_KEYWORD_ANY_FRAME,
    TOKEN_ID_KEYWORD_CALL_CONV,
    TOKEN_ID_KEYWORD_COMPTIME,
    TOKEN_ID_KEYWORD_CONST,
    TOKEN_ID_KEYWORD_DEFER,
    TOKEN_ID_KEYWORD_ELSE,
    TOKEN_ID_KEYWORD_ENUM,
    TOKEN_ID_KEYWORD_ERROR,
    TOKEN_ID_KEYWORD_ERROR_DEFER,
    TOKEN_ID_KEYWORD_EXPORT,
    TOKEN_ID_KEYWORD_EXTERN,
    TOKEN_ID_KEYWORD_FALSE,
    TOKEN_ID_KEYWORD_FN,
    TOKEN_ID_KEYWORD_FOR,
    TOKEN_ID_KEYWORD_IF,
    TOKEN_ID_KEYWORD_INLINE,
    TOKEN_ID_KEYWORD_NO_ALIAS,
    TOKEN_ID_KEYWORD_NO_INLINE,
    TOKEN_ID_KEYWORD_NULL,
    TOKEN_ID_KEYWORD_OR,
    TOKEN_ID_KEYWORD_PACKED,
    TOKEN_ID_KEYWORD_PUB,
    TOKEN_ID_KEYWORD_RETURN,
    TOKEN_ID_KEYWORD_SECTION,
    TOKEN_ID_KEYWORD_STRUCT,
    TOKEN_ID_KEYWORD_SWITCH,
    TOKEN_ID_KEYWORD_TEST,
    TOKEN_ID_KEYWORD_THREAD_LOCAL,
    TOKEN_ID_KEYWORD_TRUE,
    TOKEN_ID_KEYWORD_UNDEFINED,
    TOKEN_ID_KEYWORD_UNION,
    TOKEN_ID_KEYWORD_UNREACHABLE,
    TOKEN_ID_KEYWORD_VAR,
    TOKEN_ID_KEYWORD_VOID,
    TOKEN_ID_KEYWORD_VOLATILE,
    TOKEN_ID_KEYWORD_WHILE,
    // ...
    TOKEN_ID_MINUS_EQ,
    TOKEN_ID_MOD_EQ,
    TOKEN_ID_PLUS_EQ,
    TOKEN_ID_SYMBOL,
    TOKEN_ID_TIMES_EQ,
    TOKEN_ID_STRING_LIT,
    TOKEN_ID_MULTILINE_STRING_LIT,
} TokenID;

typedef enum RedTypeID
{
    RED_TYPE_INVALID,
    RED_TYPE_VOID,
    RED_TYPE_BOOL,
    RED_TYPE_UNREACHABLE,
    RED_TYPE_INT,
    RED_TYPE_FLOAT,
    RED_TYPE_POINTER,
    RED_TYPE_ARRAY,
    RED_TYPE_STRUCT,
    RED_TYPE_ENUM,
    RED_TYPE_UNION,
    RED_TYPE_UNDEFINED,
    RED_TYPE_NULL,
    RED_TYPE_FUNCTION,
} RedTypeID;

void print(const char* format, ...);
void logger(LogType log_type, const char *format, ...);

#define NEW(T, count) (T*)(allocate_chunk(count * sizeof(T)))
#define RENEW(T, old_ptr, count) (T*)(reallocate_chunk(old_ptr, count * sizeof(T)))

#define GEN_BUFFER_STRUCT_PTR(type_name, type) \
    typedef struct type_name##Buffer \
    {\
        struct type* ptr;\
        u32 len;\
        u32 cap;\
    } type_name##Buffer;
#define GEN_BUFFER_STRUCT(type)\
    typedef struct type##Buffer \
    {\
        type* ptr;\
        u32 len;\
        u32 cap;\
    } type##Buffer;

#define GEN_BUFFER_FUNCTIONS(p_type_prefix, buffer_name, t_type, elem_type)\
static inline u32 p_type_prefix##_len(t_type* buffer_name)\
{\
    return buffer_name->len;\
}\
static inline elem_type* p_type_prefix##_ptr(t_type* buffer_name)\
{\
    return buffer_name->ptr;\
}\
static inline void p_type_prefix##_deinit()\
{\
    RED_NOT_IMPLEMENTED;\
}\
\
static inline void p_type_prefix##_ensure_capacity(t_type* buffer_name, u32 new_capacity)\
{\
    if (buffer_name->cap >= new_capacity)\
        return;\
\
    u32 better_capacity = buffer_name->cap;\
    do {\
        better_capacity = better_capacity * 5 / 2 + 8;\
    } while (better_capacity < new_capacity);\
\
    buffer_name->ptr = RENEW(elem_type, buffer_name->ptr, better_capacity);\
    buffer_name->cap = better_capacity;\
}\
\
static inline void p_type_prefix##_resize(t_type* buffer_name, size_t new_length)\
{\
    redassert(new_length != SIZE_MAX);\
    p_type_prefix##_ensure_capacity(buffer_name, new_length);\
}\
\
static inline void p_type_prefix##_append(t_type* buffer_name, elem_type item)\
{\
    p_type_prefix##_ensure_capacity(buffer_name, buffer_name->len + 1);\
    buffer_name->ptr[buffer_name->len++] = item;\
}\
\
static inline void p_type_prefix##_append_assuming_capacity(t_type* buffer_name, elem_type item)\
{\
    buffer_name->ptr[buffer_name->len++] = item;\
}\
\
static inline elem_type p_type_prefix##_pop(t_type* buffer_name)\
{\
    redassert(buffer_name->len >= 1);\
    return buffer_name->ptr[--buffer_name->len];\
}\
\
static inline elem_type* p_type_prefix##_last(t_type* buffer_name)\
{\
    redassert(buffer_name->len >= 1);\
    return &buffer_name->ptr[buffer_name->len - 1];\
}\
\
static inline elem_type* p_type_prefix##_add_one(t_type* buffer_name)\
{\
    p_type_prefix##_resize(buffer_name, buffer_name->len + 1);\
    buffer_name->len++;\
    return p_type_prefix##_last(buffer_name);\
}\
\
static inline void p_type_prefix##_clear(t_type* buffer_name)\
{\
    buffer_name->len = 0;\
}

void* allocate_chunk(size_t size);
void* reallocate_chunk(void* allocated_address, usize size);
void  mem_init(void);

//static inline bool mem_eql_mem(const char* a, size_t a_len, const char* b, size_t b_len)
//{
//    if (a_len != b_len)
//    {
//        return false;
//    }
//    return memcmp(a, b, a_len) == 0;
//}
//
//static inline bool mem_eql_mem_ignore_case(const char* a, size_t a_len, const char* b, size_t b_len)
//{
//    if (a_len != b_len)
//    {
//        return false;
//    }
//
//    for (size_t i = 0; i < a_len; i++)
//    {
//        if (tolower(a[i]) != tolower(b[i]))
//            return false;
//    }
//    return true;
//}
//
//static inline bool mem_eql_str(const char* mem, size_t mem_len, const char* str)
//{
//    return mem_eql_mem(mem, mem_len, str, strlen(str));
//}
//
//static inline bool str_eql_str(const char* a, const char* b)
//{
//    return mem_eql_mem(a, strlen(a), b, strlen(b));
//}
//
//static inline bool str_eql_str_ignore_case(const char* a, const char* b)
//{
//    return mem_eql_mem_ignore_case(a, strlen(a), b, strlen(b));
//}
//
//static inline bool is_power_of_2(u64 x)
//{
//    return x != 0 && ((x & (~x + 1)) == x);
//}
//
//static inline bool mem_ends_with_mem(const char* mem, size_t mem_len, const char* end, size_t end_len)
//{
//    if (mem_len < end_len)
//    {
//        return false;
//    }
//    return memcmp(mem + mem_len - end_len, end, end_len) == 0;
//}
//
//static inline bool mem_ends_with_str(const char* mem, size_t mem_len, const char* str)
//{
//    return mem_ends_with_mem(mem, mem_len, str, strlen(str));
//}
//

typedef struct BigInt
{
    size_t digit_count;
    union
    {
        u64 digit;
        u64* digits; // least significant digit first
    };
    bool is_negative;
} BigInt;

typedef struct BigFloat
{
    f128 fn_handle;
} BigFloat;
typedef struct TokenFloatLit
{
    BigFloat big_float;
    bool overflow;
} TokenFloatLit;
typedef struct TokenIntLit
{
    BigInt big_int;
} TokenIntLit;
typedef struct TokenStrLit
{
    struct StringBuffer str;
} TokenStrLit;
typedef struct TokenCharLit
{
    // TODO: we are only supporting 1-byte characters for now
    char fn_handle;
} TokenCharLit;

typedef struct Token
{
    TokenID id;
    size_t start_position;
    size_t end_position;
    size_t start_line;
    size_t start_column;

    union
    {
        TokenIntLit int_lit;
        TokenFloatLit float_lit;
        TokenStrLit str_lit;
        TokenCharLit char_lit;
    };
} Token;

GEN_BUFFER_STRUCT(Token)
typedef usize Usize;
GEN_BUFFER_STRUCT(Usize)
typedef TokenBuffer TB;
typedef struct ASTNode ASTNode;
GEN_BUFFER_STRUCT_PTR(ASTNode, ASTNode*)
typedef u8 U8;
GEN_BUFFER_STRUCT(U8)
GEN_BUFFER_FUNCTIONS(u8, u8b, U8Buffer, u8)

typedef struct RedAST
{
    ASTNodeBuffer struct_decls;
    ASTNodeBuffer union_decls;
    ASTNodeBuffer enum_decls;
    ASTNodeBuffer fn_definitions;
} RedAST;

typedef struct UsizeBuffer UsizeBuffer;
typedef struct LexingResult
{
    TokenBuffer tokens;
    UsizeBuffer line_offsets;

    StringBuffer error;
    size_t error_line;
    size_t error_column;
} LexingResult;

static inline struct StringBuffer* token_buffer(Token* token)
{
    if (!token)
    {
        return NULL;
    }
    redassert(token->id == TOKEN_ID_STRING_LIT || token->id == TOKEN_ID_MULTILINE_STRING_LIT || token->id == TOKEN_ID_SYMBOL);
    return &token->str_lit.str;
}

static inline BigInt* token_bigint(Token* token)
{
    if (!token)
    {
        return NULL;
    }
    redassert(token->id == TOKEN_ID_INT_LIT);
    return &token->int_lit.big_int;
}

static inline BigFloat* token_bigfloat(Token* token)
{
    if (!token)
    {
        return NULL;
    }
    redassert(token->id == TOKEN_ID_FLOAT_LIT);
    return &token->float_lit.big_float;
}

static inline bool token_is_binop_char(TokenID op)
{
    bool is_it = op == TOKEN_ID_PLUS ||
        op == TOKEN_ID_EQ ||
        op == TOKEN_ID_DASH ||
        op == TOKEN_ID_STAR ||
        op == TOKEN_ID_SLASH ||
        op == TOKEN_ID_CMP_NOT_EQ ||
        op == TOKEN_ID_CMP_EQ ||
        op == TOKEN_ID_CMP_GREATER ||
        op == TOKEN_ID_CMP_GREATER_OR_EQ ||
        op == TOKEN_ID_CMP_LESS ||
        op == TOKEN_ID_CMP_LESS_OR_EQ;
    return is_it;
}

typedef enum TypeKind
{
    TYPE_KIND_INVALID,
    TYPE_KIND_VOID,
    TYPE_KIND_PRIMITIVE,
    TYPE_KIND_COMPLEX_TO_BE_DETERMINED,
    TYPE_KIND_STRUCT,
    TYPE_KIND_UNION,
    TYPE_KIND_ENUM,
    TYPE_KIND_ARRAY,
    TYPE_KIND_FUNCTION,
} TypeKind;


