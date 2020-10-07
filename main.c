//
// Created by David on 28/09/2020.
//

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "types.h"
#include <stdarg.h>

typedef enum
{
	FAIL = 0,
	SUCCESS = 1,
} GeneralError;

#define GENERAL_FAILED(x) (!(x))

const char *log_type_to_str(LogType log_type)
{
    switch (log_type)
    {
        CASE_TO_STR(LOG_TYPE_INFO);
        CASE_TO_STR(LOG_TYPE_WARN);
        CASE_TO_STR(LOG_TYPE_ERROR);
        default:
            assert(0);
    }
    return NULL;
}

void logger(LogType log_type, const char *format, ...)
{
    fprintf(stdout, "[%s] ", log_type_to_str(log_type));
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
}

static char src_buffer[10000];
char *src_it = &src_buffer[0];

char *file_load(const char *name)
{
    FILE *file = fopen(name, "rb");
    assert(file);

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    printf("[FILE] File length: %ld.\n", length);
    assert(0 < length && (src_it + length) < src_it + COUNT_OF(src_buffer));
    fseek(file, 0, SEEK_SET);

    size_t rc = fread(src_it, 1, length, file);
    assert(rc == (size_t) length);
    fclose(file);

    return src_it;
}

typedef enum Token
{
    TOKEN_INVALID = -1,
    TOKEN_WHITESPACE = 0,
    TOKEN_EQUAL = 1,
    TOKEN_DASH = 2,
    TOKEN_PLUS = 3,
    TOKEN_ASTERISC = 4,
    TOKEN_SLASH = 5,
    TOKEN_BACKSLASH = 6,
    TOKEN_SEMICOLON = 7,
    TOKEN_DOUBLE_COLON = 8,
    TOKEN_LEFT_PARENTHESIS = 9,
    TOKEN_RIGHT_PARENTHESIS = 10,
    TOKEN_LEFT_BRACE = 11,
    TOKEN_RIGHT_BRACE = 12,
    TOKEN_LEFT_BRACKET = 13,
    TOKEN_RIGHT_BRACKET = 14,
    TOKEN_COMMA = 15,
    TOKEN_DOT = 16,
    TOKEN_ARROW = 17,
    //
    TOKEN_VAR = 18,
    TOKEN_VOID = 19,
    TOKEN_S8 = 20,
    TOKEN_S16 = 21,
    TOKEN_S32 = 22,
    TOKEN_S64 = 23,
    TOKEN_U8 = 24,
    TOKEN_U16 = 25,
    TOKEN_U32 = 26,
    TOKEN_U64 = 27,
    TOKEN_IF = 28,
    TOKEN_ELSE = 29,
    TOKEN_WHILE = 30,
    TOKEN_FOR = 31,
    TOKEN_RETURN = 32,
    TOKEN_FALSE = 33,
    TOKEN_TRUE = 34,
    //
    TOKEN_NAME = 35,
    TOKEN_CHAR_LIT = 36,
    TOKEN_STRING_LIT = 37,
    TOKEN_NUMBER_LIT = 38,
    //
    TOKEN_STRUCT = 39,
    TOKEN_UNION = 40,
    TOKEN_ENUM = 41,
    TOKEN_F32 = 42,
    TOKEN_F64 = 43,
    TOKEN_FN = 44,
    //
} Token;

bool is_onechar(Token t)
{
    return t == TOKEN_DASH ||
           t == TOKEN_ARROW ||
           t == TOKEN_EQUAL ||
           t == TOKEN_PLUS ||
           t == TOKEN_ASTERISC ||
           t == TOKEN_SLASH ||
           t == TOKEN_BACKSLASH ||
           t == TOKEN_WHITESPACE ||
           t == TOKEN_INVALID ||
           t == TOKEN_SEMICOLON ||
           t == TOKEN_DOUBLE_COLON ||
           t == TOKEN_LEFT_PARENTHESIS ||
           t == TOKEN_RIGHT_PARENTHESIS ||
           t == TOKEN_LEFT_BRACE ||
           t == TOKEN_RIGHT_BRACE ||
           t == TOKEN_LEFT_BRACKET ||
           t == TOKEN_RIGHT_BRACKET ||
           t == TOKEN_COMMA ||
           t == TOKEN_DOT;
}


const char *get_token_str(Token t)
{
    switch (t)
    {
        CASE_TO_STR(TOKEN_INVALID);
        CASE_TO_STR(TOKEN_WHITESPACE);
        CASE_TO_STR(TOKEN_EQUAL);
        CASE_TO_STR(TOKEN_DASH);
        CASE_TO_STR(TOKEN_PLUS);
        CASE_TO_STR(TOKEN_ASTERISC);
        CASE_TO_STR(TOKEN_SLASH);
        CASE_TO_STR(TOKEN_BACKSLASH);
        CASE_TO_STR(TOKEN_SEMICOLON);
        CASE_TO_STR(TOKEN_DOUBLE_COLON);
        CASE_TO_STR(TOKEN_LEFT_PARENTHESIS);
        CASE_TO_STR(TOKEN_RIGHT_PARENTHESIS);
        CASE_TO_STR(TOKEN_LEFT_BRACE);
        CASE_TO_STR(TOKEN_RIGHT_BRACE);
        CASE_TO_STR(TOKEN_LEFT_BRACKET);
        CASE_TO_STR(TOKEN_RIGHT_BRACKET);
        CASE_TO_STR(TOKEN_COMMA);
        CASE_TO_STR(TOKEN_DOT);
        CASE_TO_STR(TOKEN_ARROW);
            //
        CASE_TO_STR(TOKEN_VAR);
        CASE_TO_STR(TOKEN_VOID);
        CASE_TO_STR(TOKEN_S8);
        CASE_TO_STR(TOKEN_S16);
        CASE_TO_STR(TOKEN_S32);
        CASE_TO_STR(TOKEN_S64);
        CASE_TO_STR(TOKEN_U8);
        CASE_TO_STR(TOKEN_U16);
        CASE_TO_STR(TOKEN_U32);
        CASE_TO_STR(TOKEN_U64);
        CASE_TO_STR(TOKEN_IF);
        CASE_TO_STR(TOKEN_ELSE);
        CASE_TO_STR(TOKEN_WHILE);
        CASE_TO_STR(TOKEN_FOR);
        CASE_TO_STR(TOKEN_RETURN);
        CASE_TO_STR(TOKEN_FALSE);
        CASE_TO_STR(TOKEN_TRUE);
            //
        CASE_TO_STR(TOKEN_NAME);
        CASE_TO_STR(TOKEN_CHAR_LIT);
        CASE_TO_STR(TOKEN_STRING_LIT);
        CASE_TO_STR(TOKEN_NUMBER_LIT);
            //
        CASE_TO_STR(TOKEN_STRUCT);
        CASE_TO_STR(TOKEN_UNION);
        CASE_TO_STR(TOKEN_ENUM);
        CASE_TO_STR(TOKEN_F32);
        CASE_TO_STR(TOKEN_F64);
        CASE_TO_STR(TOKEN_FN);
            //

        default:
            printf("Token not recognized: %d\n", t);
            assert(0);
            return NULL;
    }
}


#define MAX_TOKENS 10000
Token token_arr[MAX_TOKENS];
u32 token_count = 0;
u32 word_indices[MAX_TOKENS];


#define WORD_MAX_CHAR_COUNT 64
typedef char Word[WORD_MAX_CHAR_COUNT];
#define MAX_WORD_COUNT 1000
Word words[MAX_WORD_COUNT];
u16 word_count = 0;

static inline char* get_token_name(size_t token_index)
{
    return words[word_indices[token_index]];
}

static Token get_word(char *string, size_t *word_count);

static Token get_token(void)
{
    char ch = *src_it;
    printf("Lexing string:\n***\n%s\n***\n", src_it);
    switch (ch)
    {
        case '-':
        {
            bool is_left_arrow = *src_it == '>';
            if (!is_left_arrow)
            {
                return TOKEN_DASH;
            }
            else
            {
                src_it++;
                return TOKEN_ARROW;
            }
        }
        case '=':
            return TOKEN_EQUAL;
        case '+':
            return TOKEN_PLUS;
        case '*':
            return TOKEN_ASTERISC;
        case '/':
            return TOKEN_SLASH;
        case '\\':
            return TOKEN_BACKSLASH;
        case '\n': // backslash + char
        case ' ':
            return TOKEN_WHITESPACE;
        case '\r':
        case '\t':
            return TOKEN_INVALID;
        case ';':
            return TOKEN_SEMICOLON;
        case ':':
            return TOKEN_DOUBLE_COLON;
        case '(':
            return TOKEN_LEFT_PARENTHESIS;
        case ')':
            return TOKEN_RIGHT_PARENTHESIS;
        case '{':
            return TOKEN_LEFT_BRACE;
        case '}':
            return TOKEN_RIGHT_BRACE;
        case '[':
            return TOKEN_LEFT_BRACKET;
        case ']':
            return TOKEN_RIGHT_BRACKET;
        case ',':
            return TOKEN_COMMA;
        case '.':
            return TOKEN_DOT;
        case '\'':
        case '\"':
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'g':
        case 'h':
        case 'i':
        case 'j':
        case 'k':
        case 'l':
        case 'm':
        case 'n':
        case 'o':
        case 'p':
        case 'q':
        case 'r':
        case 's':
        case 't':
        case 'u':
        case 'v':
        case 'w':
        case 'x':
        case 'y':
        case 'z':
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case 'G':
        case 'H':
        case 'I':
        case 'J':
        case 'K':
        case 'L':
        case 'M':
        case 'N':
        case 'O':
        case 'P':
        case 'Q':
        case 'R':
        case 'S':
        case 'T':
        case 'U':
        case 'V':
        case 'W':
        case 'X':
        case 'Y':
        case 'Z':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        {
            size_t word_char_count = 0;
            Token token = get_word(src_it, &word_char_count);
            src_it += word_char_count - 1;
            return token;
        }
        default:
            printf("Char not dealt with: %c(%d)\n", ch, (s32) ch);
            assert(0);
            return 0;
    }
}

static bool is_delimiter(char c)
{
    bool delimiter = c == ' ' ||
                     c == '-' ||
                     c == ';' ||
                     c == ',' ||
                     c == '.' ||
                     c == '(' ||
                     c == ')' ||
                     c == '{' ||
                     c == '}' ||
                     c == '[' ||
                     c == ']' ||
                     c == EOF ||
                     c == '\n' ||
                     c == '\'' ||
                     c == '\"' ||
                     c == 0;
    return delimiter;
}

static inline bool is_char(char ch, const char *str, size_t i)
{
    return str[i] == ch && (i > 0 ? (str[i - 1] != '\\') : true);
}

static Token get_word(char *string, size_t *char_count)
{
    Token token = TOKEN_NAME;
    char word_buffer[WORD_MAX_CHAR_COUNT];
    char ch = *string;

    size_t i = 0;
    if (ch == '\'' || ch == '\"')
    {
        char delimiter = ch;
        do
        {
            word_buffer[i] = string[i];
            i++;
        } while (!is_char(delimiter, string, i));
        word_buffer[i] = string[i];
        i++;
    }
    else if (isdigit(ch))
    {
        while (isdigit(string[i]))
        {
            word_buffer[i] = string[i];
            i++;
        }
    }
    else
    {
        for (i = 0; !is_delimiter(string[i]); i++)
        {
            word_buffer[i] = string[i];
        }
    }
    word_buffer[i] = 0;
    *char_count = i;

    switch (ch)
    {
        case '\'':
        {
            token = TOKEN_CHAR_LIT;
            break;
        }
        case '\"':
        {
            token = TOKEN_STRING_LIT;
            break;
        }
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        {
            if (strequal("else", word_buffer))
            {
                token = TOKEN_ELSE;
            }
            else if (strequal("enum", word_buffer))
            {
                token = TOKEN_ENUM;
            }
        }
            break;
        case 'f':
        {
            if (strequal("false", word_buffer))
            {
                token = TOKEN_FALSE;
            }
            else if (strequal("for", word_buffer))
            {
                token = TOKEN_FOR;
            }
            else if (strequal("f32", word_buffer))
            {
                token = TOKEN_F32;
            }
            else if (strequal("f64", word_buffer))
            {
                token = TOKEN_F64;
            }
            else if (strequal("fn", word_buffer))
            {
                token = TOKEN_FN;
            }
            break;
        }
        case 'g':
        case 'h':
        case 'i':
        {
            if (strequal("if", word_buffer))
            {
                token = TOKEN_IF;
            }
            break;
        }
        case 'j':
        case 'k':
        case 'l':
        case 'm':
        case 'n':
        case 'o':
        case 'p':
        case 'q':
        case 'r':
        {
            if (strequal("return", word_buffer))
            {
                token = TOKEN_RETURN;
            }
            break;
        }
        case 's':
        {
            if (strequal("s8", word_buffer))
            {
                token = TOKEN_S8;
            }
            else if (strequal("s16", word_buffer))
            {
                token = TOKEN_S16;
            }
            else if (strequal("s32", word_buffer))
            {
                token = TOKEN_S32;
            }
            else if (strequal("s64", word_buffer))
            {
                token = TOKEN_S64;
            }
            else if (strequal("struct", word_buffer))
            {
                token = TOKEN_STRUCT;
            }
            break;
        }
        case 't':
        {
            if (strequal("true", word_buffer))
            {
                token = TOKEN_TRUE;
            }
            break;
        }
        case 'u':
        {
            if (strequal("u8", word_buffer))
            {
                token = TOKEN_U8;
            }
            else if (strequal("u16", word_buffer))
            {
                token = TOKEN_U16;
            }
            else if (strequal("u32", word_buffer))
            {
                token = TOKEN_U32;
            }
            else if (strequal("u64", word_buffer))
            {
                token = TOKEN_U64;
            }
            else if (strequal("union", word_buffer))
            {
                token = TOKEN_UNION;
            }
            break;
        }
        case 'v':
        {
            if (strequal("var", word_buffer))
            {
                token = TOKEN_VAR;
            }
            else if (strequal("void", word_buffer))
            {
                token = TOKEN_VOID;
            }
            break;
        }
        case 'w':
        {
            if (strequal("while", word_buffer))
            {
                token = TOKEN_WHILE;
            }
            break;
        }
        case 'x':
        case 'y':
        case 'z':
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case 'G':
        case 'H':
        case 'I':
        case 'J':
        case 'K':
        case 'L':
        case 'M':
        case 'N':
        case 'O':
        case 'P':
        case 'Q':
        case 'R':
        case 'S':
        case 'T':
        case 'U':
        case 'V':
        case 'W':
        case 'X':
        case 'Y':
        case 'Z':
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        {
            token = TOKEN_NUMBER_LIT;
            break;
        }
        default:
            printf("\nChar not taken into account: %c(%d)\n", ch, (s32) ch);
            assert(0);
            break;
    }
    u32 word_index = word_count++;
    strcpy(words[word_count], word_buffer);
    word_indices[token_count] = word_index;
    // Only names come this far
    return token;
}

static void lex(void)
{
    while (*src_it != 0)
    {
        Token token = get_token();
        if (token != TOKEN_WHITESPACE)
        {
            printf("Got token: %s\n", get_token_str(token));
            size_t token_index = token_count++;
            token_arr[token_index] = token;
        }
        *src_it++;
    }
}

static void debug_lexing(void)
{
    size_t word_it = 0;
    for (u32 i = 0; i < token_count; i++)
    {
        Token token = token_arr[i];
        if (is_onechar(token))
        {
            printf("[%d] Token: %s\n", i, get_token_str(token));
        }
        else
        {
            printf("[%d] Token: %s    Value: %s\n", i, get_token_str(token), words[word_it]);
            word_it++;
            assert(word_it <= word_count);
        }
    }
}

#define SHIFT_LEFT(n) (1 << n)
// PARSER / AST
typedef enum TypeEnum
{
    TYPE_S8 =        1 << 0,
    TYPE_S16 =       1 << 1,
    TYPE_S32 =       1 << 2,
    TYPE_S64 =       1 << 3,
    TYPE_U8 =        1 << 4,
    TYPE_U16 =       1 << 5,
    TYPE_U32 =       1 << 6,
    TYPE_U64 =       1 << 7,
    TYPE_F32 =       1 << 8,
    TYPE_F64 =       1 << 9,
    TYPE_STRUCT =    1 << 10,
    TYPE_UNION =     1 << 11,
    TYPE_ENUM =      1 << 12,
} TypeEnum;

typedef struct Type
{
    TypeEnum type_enum;
    bool ptr;
    bool arr;
} Type;

const char* get_type_enum(Type type)
{
    switch (type.type_enum)
    {
        case TYPE_S8:
        case TYPE_S16:
        case TYPE_S32:
        case TYPE_S64:
        case TYPE_U8:
        case TYPE_U16:
        case TYPE_U32:
        case TYPE_U64:
        case TYPE_F32:
        case TYPE_F64:
        case TYPE_STRUCT:
        case TYPE_UNION:
        case TYPE_ENUM:
        default:
            assert(0);
            return NULL;
    }
}

typedef union Value
{
    u8  u8v;
    u16 u16v;
    u32 u32v;
    u64 u64v;
    s8  s8v;
    s16 s16v;
    s32 s32v;
    s64 s64v;
    f32 f32v;
    f64 f64v;
    void* ptrv;
} Value;

typedef struct LiteralAST
{
    Type type;
    Value value;
} LiteralAST;

typedef union ComplexValue
{
    LiteralAST lit_value;
    union DataField* values;
} ComplexValue;

typedef union DataField
{
    ComplexValue complex_value;
    char* name;
    u32 element_count;
} DataField;

typedef DataField VariableAST;
typedef DataField ConstantAST;

typedef struct StructAST
{
    DataField *fields;
    char* name;
    u64 id;
} StructAST;

typedef struct UnionAST
{
    DataField *fields;
    char* name;
    u64 id;
} UnionAST;

typedef struct EnumAST
{
    DataField* values;
    char* name;
    u64 id;
} EnumAST;

typedef struct FunctionCallAST
{
    VariableAST *arguments;
    Type return_type;
    char *name;
} FunctionCallAST;
typedef FunctionCallAST FunctionPrototypeAST;

typedef struct OperatorAST
{
    char op;
} OperatorAST;

typedef union ElementAST
{
    VariableAST variable;
    ConstantAST constant;
    LiteralAST literal;
    StructAST struct_;
    union ElementAST *element;
} ElementAST;

typedef struct BinaryExpressionAST
{
    ElementAST left;
    ElementAST right;
    OperatorAST operator;
} ExpressionAST;

typedef struct FunctionAST
{
    FunctionPrototypeAST *prototype;
    ExpressionAST expression;
} FunctionAST;

#define MAX_STRUCTS 10000
StructAST registered_structs[MAX_STRUCTS];
size_t registered_struct_count = 0;
#define MAX_UNIONS 10000
UnionAST registered_unions[MAX_UNIONS];
size_t registered_union_count = 0;
#define MAX_ENUMS 10000
EnumAST registered_enums[MAX_ENUMS];
size_t registered_enum_count = 0;

#define MAX_DATA_FIELDS 10000
DataField data_fields[MAX_DATA_FIELDS];
size_t data_field_count = 0;

static bool is_primitive_type(Token token)
{
    switch (token)
    {
        case TOKEN_S8:
        case TOKEN_S16:
        case TOKEN_S32:
        case TOKEN_S64:
        case TOKEN_U8:
        case TOKEN_U16:
        case TOKEN_U32:
        case TOKEN_U64:
        case TOKEN_F32:
        case TOKEN_F64:
            return true;
        default:
            return false;
    }
}

// TODO: substitute for a better way
static bool struct_ends(Token* token_arr, size_t index)
{
    return token_arr[index] == TOKEN_RIGHT_BRACE;
}

static StructAST* get_registered_struct(const char* struct_name, StructAST* registered_structs, size_t registered_struct_count)
{
    for (size_t i = 0; i < registered_struct_count; i++)
    {
        if (strequal(registered_structs[i].name, struct_name))
        {
            return &registered_structs[i];
        }
    }
    return NULL;
}

static UnionAST* get_registered_union(const char* union_name, UnionAST* registered_unions, size_t registered_union_count)
{
    for (size_t i = 0; i < registered_union_count; i++)
    {
        if (strequal(registered_unions[i].name, union_name))
        {
            return &registered_unions[i];
        }
    }
    return NULL;
}

static EnumAST* get_registered_enum(const char* enum_name, EnumAST* registered_enums, size_t registered_enum_count)
{
    for (size_t i = 0; i < registered_enum_count; i++)
    {
        if (strequal(registered_enums[i].name, enum_name))
        {
            return &registered_enums[i];
        }
    }
    return NULL;
}

static void register_struct_field(StructAST* struct_to_fill, DataField* field)
{
    field->name = struct_to_fill->name;
    field->complex_value.values = struct_to_fill->fields;
}

static Type get_primitive_type(Token token)
{
    Type type = {0};
    switch (token)
    {
        case TOKEN_S8:
            type.type_enum = TYPE_S8;
            break;
        case TOKEN_S16:
            type.type_enum = TYPE_S16;
            break;
        case TOKEN_S32:
            type.type_enum = TYPE_S32;
            break;
        case TOKEN_S64:
            type.type_enum = TYPE_S64;
            break;
        case TOKEN_U8:
            type.type_enum = TYPE_U8;
            break;
        case TOKEN_U16:
            type.type_enum = TYPE_U16;
            break;
        case TOKEN_U32:
            type.type_enum = TYPE_U32;
            break;
        case TOKEN_U64:
            type.type_enum = TYPE_U64;
            break;
        case TOKEN_F32:
            type.type_enum = TYPE_F32;
            break;
        case TOKEN_F64:
            type.type_enum = TYPE_F64;
            break;
        default:
            assert(0);
            return type;
    }
    return type;
}
static size_t register_data_fields(Token* token_arr, size_t* token_index, DataField* data_fields, size_t data_field_count)
{
    size_t i = *token_index;
    while (!struct_ends(token_arr, i))
    {
        size_t data_field_index = data_field_count;
        Token token = token_arr[i];
        data_fields[data_field_index].name = get_token_name(i + 1);
        assert(!strempty(data_fields[i].name));
        if (is_primitive_type(token))
        {
            data_fields[data_field_index].element_count = 1;
            data_fields[data_field_index].complex_value.lit_value.type = get_primitive_type(token);
        }
        else
        {
            void* ptr = get_registered_struct(data_fields[data_field_index].name, registered_structs, registered_struct_count);
            if (ptr)
            {
                StructAST* struct_ptr = (struct StructAST*)ptr;
                data_fields[data_field_index].element_count = struct_ptr->fields->element_count;
                if (data_fields[data_field_index].element_count)
                {
                    // Not implemented yet
                    assert(0);
                }
                else
                {
                    LiteralAST literal_value = struct_ptr->fields->complex_value.lit_value;
                    memcpy(&data_fields[data_field_index].complex_value.lit_value, &literal_value, sizeof(LiteralAST));
                }
            }
            else
            {
                assert(0);
            }
        }
        data_field_count++;
        i += 3;
    }
    *token_index = i;
    token_arr[*token_index];
    return data_field_count;
}

static size_t get_cursor_into_the_struct(Token* token_arr, size_t token_index)
{
    while (token_index != TOKEN_RIGHT_BRACE)
    {
        token_index++;
    }
    token_index++;
    return token_index;
}

static void register_types(Token* token_arr, size_t token_count)
{
    size_t token_index = 0;
    while (token_index < token_count)
    {
        Token token = token_arr[token_index];
        if (token == TOKEN_STRUCT || token == TOKEN_UNION || token == TOKEN_ENUM)
        {
            switch (token) {
                case TOKEN_STRUCT:
                    StructAST* registered_struct = &registered_structs[token_index];
                    registered_struct->name = get_token_name(token_index);
                    assert(registered_struct->name);
                    token_index = get_cursor_into_the_struct(token_arr, token_index);
                    logger(LOG_TYPE_INFO, "Cursor left in: %s\n", get_token_str(token_arr[token_index]));
                    register_data_fields(token_arr, &token_index, &data_fields[0], data_field_count);
                    break;
                case TOKEN_UNION:
                    break;
                case TOKEN_ENUM:
                    break;
            }
        }
        else
        {
            logger(LOG_TYPE_INFO, "Not a complex type\n");
        }
    }
}

bool is_type(Token token, size_t i)
{
    if (token == TOKEN_STRUCT ||
        token == TOKEN_UNION ||
        token == TOKEN_ENUM)
    {
        char* token_name = get_token_name(i);
        if (token_name != NULL)
        {
            if (token == TOKEN_STRUCT)
            {
                return get_registered_struct(token_name, registered_structs, registered_struct_count);
            }
            else if (token == TOKEN_UNION)
            {
                return get_registered_union(token_name, registered_unions, registered_union_count);
            }
            else if (token == TOKEN_ENUM)
            {
                return get_registered_enum(token_name, registered_enums, registered_enum_count);
            }
        }
        else
        {
            return false;
        }
    }
    else
    {
        switch (token)
        {
            case TOKEN_S8:
            case TOKEN_S16:
            case TOKEN_S32:
            case TOKEN_S64:
            case TOKEN_U8:
            case TOKEN_U16:
            case TOKEN_U32:
            case TOKEN_U64:
            case TOKEN_F32:
            case TOKEN_F64:
                return true;
            default:
                return false;
        }
    }
}
#define VARIABLE_OFFSET_WITH_COUNT 4
static GeneralError parse_variable(size_t* token_index, Token* token_arr, size_t token_count, VariableAST* variable)
{
	Token var_tag = token_arr[*token_index];
	Token var_name = 0;
	assert(var_tag == TOKEN_VAR);

	if (token_count < *token_index + VARIABLE_OFFSET_WITH_COUNT)
	{
		return FAIL;
	}

	size_t next = *token_index + 1;
	bool next_is_type = is_type(token_arr[*token_index], next);
	if (is_type)
    {
        Token var_type = token_arr[next++];
        var_name = token_arr[next];
        logger(LOG_TYPE_INFO, "Var type: %s. Var name: %s(%s)\n", get_token_str(var_type), get_token_str(var_name), get_token_name(next));
    }
	else
    {
	    var_name = token_arr[next];
    }
    // TODO: go search var_name
    (*token_index) += (1 + next_is_type);

    return SUCCESS;
}

#define LITERAL_FORMAT "Type: %d. Value: %d"
static void debug_literal(LiteralAST* literal, bool hierarchy)
{
    logger(LOG_TYPE_INFO, "Literal: " LITERAL_FORMAT "\n", literal->type, literal->value);
}
static void debug_variable(VariableAST* variable)
{
    logger(LOG_TYPE_INFO, "Variable %s\n\t" LITERAL_FORMAT "\n", variable->name ? variable->name : "Unnamed", variable->complex_value.lit_value.type, variable->complex_value.lit_value.value);
}

static ElementAST parse_token(size_t* token_index, Token* token_arr, size_t token_count)
{
    Token token = token_arr[*token_index];
    ElementAST element = {0};
    switch (token)
    {
        case TOKEN_INVALID:
            assert(0);
            break;
        case TOKEN_EQUAL:
            //element.literal =
            break;
        case TOKEN_DASH:
            break;
        case TOKEN_PLUS:
            break;
        case TOKEN_ASTERISC:
            break;
        case TOKEN_SLASH:
            break;
        case TOKEN_BACKSLASH:
            break;
        case TOKEN_SEMICOLON:
            break;
        case TOKEN_DOUBLE_COLON:
            break;
        case TOKEN_LEFT_PARENTHESIS:
            break;
        case TOKEN_RIGHT_PARENTHESIS:
            break;
        case TOKEN_LEFT_BRACE:
            break;
        case TOKEN_RIGHT_BRACE:
            break;
        case TOKEN_LEFT_BRACKET:
            break;
        case TOKEN_RIGHT_BRACKET:
            break;
        case TOKEN_COMMA:
            break;
        case TOKEN_DOT:
            break;
        case TOKEN_ARROW:
            break;
        case TOKEN_VAR:
            parse_variable(token_index, token_arr, token_count, &element.variable);
            debug_variable(&element.variable);
            break;
        case TOKEN_VOID:
            break;
        case TOKEN_S8:
            break;
        case TOKEN_S16:
            break;
        case TOKEN_S32:
            break;
        case TOKEN_S64:
            break;
        case TOKEN_U8:
            break;
        case TOKEN_U16:
            break;
        case TOKEN_U32:
            break;
        case TOKEN_U64:
            break;
        case TOKEN_IF:
            break;
        case TOKEN_ELSE:
            break;
        case TOKEN_WHILE:
            break;
        case TOKEN_FOR:
            break;
        case TOKEN_RETURN:
            break;
        case TOKEN_FALSE:
            break;
        case TOKEN_TRUE:
            break;
        case TOKEN_NAME:
            break;
        case TOKEN_CHAR_LIT:
            break;
        case TOKEN_STRING_LIT:
            break;
        case TOKEN_NUMBER_LIT:
            break;
        case TOKEN_STRUCT:
            break;
        case TOKEN_UNION:
            break;
        case TOKEN_ENUM:
            break;
        case TOKEN_F32:
            break;
        case TOKEN_F64:
            break;
        case TOKEN_FN:
            break;
        default:
            printf("Token doesn't exist\n");
            assert(0);
    }
    return element;
}

static void parse_tokens(Token *tokens, size_t token_count)
{
    register_types(tokens, token_count);
    for (u64 i = 0; i < token_count; i++)
    {
        Token token = tokens[i];
        logger(LOG_TYPE_INFO, "Parsing: %s\n", get_token_str(token));
        parse_token(&i, tokens, token_count);
    }
}

#define TEST_SRC_PATH "../test.red"
s32 main(void)
{
    const char *src_code = file_load(TEST_SRC_PATH);
    printf("Compiling:\n***\n%s\n***\n", src_code);
    strcpy(src_buffer, src_code);
    lex();
    debug_lexing();
    parse_tokens(token_arr, token_count);
}