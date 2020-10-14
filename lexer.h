//
// Created by david on 7/10/20.
//

#ifndef REDFLAG_LEXER_H
#define REDFLAG_LEXER_H


#include "types.h"
#include "buffer.h"
#include "bigint.h"
#include "bigfloat.h"

enum TokenID
{
    TOKEN_ID_AMPERSAND,
    TOKEN_ID_ARROW,
    TOKEN_ID_AT,
    TOKEN_ID_BANG,
    TOKEN_ID_BIT_OR,
    TOKEN_ID_BIT_OR_EQ,
    TOKEN_ID_BIT_XOR,
    TOKEN_ID_BIT_XOR_EQ,
    TOKEN_ID_BIT_AND,
    TOKEN_ID_BIT_AND_EQ,
    TOKEN_ID_BIT_SHL,
    TOKEN_ID_BIT_SHL_EQ,
    TOKEN_ID_BIT_SHR,
    TOKEN_ID_BIT_SHR_EQ,
    TOKEN_ID_CHAR_LIT,
    TOKEN_ID_CMP_EQ,
    TOKEN_ID_CMP_GREATER_OR_EQ,
    TOKEN_ID_CMP_GREATER,
    TOKEN_ID_CMP_LESS_OR_EQ,
    TOKEN_ID_CMP_LESS,
    TOKEN_ID_CMP_NOT_EQ,
    TOKEN_ID_COLON,
    TOKEN_ID_COMMA,
    TOKEN_ID_DASH,
    TOKEN_ID_DIV_EQ,
    TOKEN_ID_DOT,
    TOKEN_ID_END_OF_FILE,
    TOKEN_ID_EQ,
    TOKEN_ID_FAT_ARROW,
    TOKEN_ID_FLOAT_LIT,
    TOKEN_ID_INT_LIT,
    TOKEN_ID_HASH,
    TOKEN_ID_KEYWORD_ALIGN,
    TOKEN_ID_KEYWORD_ALLOW_ZERO,
    TOKEN_ID_KEYWORD_ANY,
    TOKEN_ID_KEYWORD_ANY_FRAME,
    TOKEN_ID_KEYWORD_COMPTIME,
    TOKEN_ID_KEYWORD_CONST,
    TOKEN_ID_KEYWORD_DEFER,
    TOKEN_ID_KEYWORD_ELSE,
    TOKEN_ID_KEYWORD_ERROR_DEFER,
    TOKEN_ID_KEYWORD_EXPORT,
    TOKEN_ID_KEYWORD_EXTERN,
    TOKEN_ID_KEYWORD_FN,
    TOKEN_ID_KEYWORD_FOR,
    TOKEN_ID_KEYWORD_IF,
    TOKEN_ID_KEYWORD_INLINE,
    TOKEN_ID_KEYWORD_NO_INLINE,
    TOKEN_ID_KEYWORD_PUB,
    TOKEN_ID_KEYWORD_SECTION,
    TOKEN_ID_KEYWORD_TEST,
    TOKEN_ID_KEYWORD_THREAD_LOCAL,
    TOKEN_ID_KEYWORD_VAR,
    TOKEN_ID_KEYWORD_VOLATILE,
    TOKEN_ID_KEYWORD_WHILE,
    // ...
    TOKEN_ID_LEFT_BRACE,
    TOKEN_ID_LEFT_BRACKET,
    TOKEN_ID_LEFT_PARENTHESIS,
    TOKEN_ID_QUESTION,
    TOKEN_ID_MINUS_EQ,
    TOKEN_ID_MOD_EQ,
    TOKEN_ID_PERCENT,
    TOKEN_ID_PLUS,
    TOKEN_ID_PLUS_EQ,
    TOKEN_ID_RIGHT_BRACE,
    TOKEN_ID_RIGHT_BRACKET,
    TOKEN_ID_RIGHT_PARENTHESIS,
    TOKEN_ID_SEMICOLON,
    TOKEN_ID_SLASH,
    TOKEN_ID_STAR,
    TOKEN_ID_STRING_LIT,
    TOKEN_ID_MULTILINE_STRING_LIT,
    TOKEN_ID_SYMBOL,
    TOKEN_ID_TILDE,
    TOKEN_ID_TIMES_EQ,
    TOKEN_ID_COUNT
};

struct TokenFloatLit
{
    BigFloat big_float;
    bool overflow;
};
struct TokenIntLit
{
    BigInt big_int;
};
struct TokenStrLit
{
    Buffer str;
};
struct TokenCharLit
{
    // TODO: we are only supporting 1-byte characters for now
    char c;
};

struct Token
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
    } data;
};

struct LexingResult
{
    List<Token> tokens;
    List<size_t> line_offsets;

    Buffer* error;
    size_t error_line;
    size_t error_column;
};

LexingResult lex(Buffer* buffer);
void print_tokens(Buffer* buffer, List<Token>* tokens);
const char* token_name(TokenID token_enum);
bool valid_symbol_starter(char c);
bool is_red_keyword(Buffer* buffer);

#endif //REDFLAG_LEXER_H
