//
// Created by david on 7/10/20.
//

#include "lexer.h"
#include "types.h"
#include "buffer.h"
#include "bigint.h"
#include "bigfloat.h"
#include "memory.h"
#include "error.h"
#include <stdarg.h>

#define WHITESPACE \
         ' ': \
    case '\r': \
    case '\n'

#define DIGIT_NON_ZERO \
         '1': \
    case '2': \
    case '3': \
    case '4': \
    case '5': \
    case '6': \
    case '7': \
    case '8': \
    case '9'
#define DIGIT \
         '0': \
    case DIGIT_NON_ZERO

#define ALPHA \
         'a': \
    case 'b': \
    case 'c': \
    case 'd': \
    case 'e': \
    case 'f': \
    case 'g': \
    case 'h': \
    case 'i': \
    case 'j': \
    case 'k': \
    case 'l': \
    case 'm': \
    case 'n': \
    case 'o': \
    case 'p': \
    case 'q': \
    case 'r': \
    case 's': \
    case 't': \
    case 'u': \
    case 'v': \
    case 'w': \
    case 'x': \
    case 'y': \
    case 'z': \
    case 'A': \
    case 'B': \
    case 'C': \
    case 'D': \
    case 'E': \
    case 'F': \
    case 'G': \
    case 'H': \
    case 'I': \
    case 'J': \
    case 'K': \
    case 'L': \
    case 'M': \
    case 'N': \
    case 'O': \
    case 'P': \
    case 'Q': \
    case 'R': \
    case 'S': \
    case 'T': \
    case 'U': \
    case 'V': \
    case 'W': \
    case 'X': \
    case 'Y': \
    case 'Z'

#define SYMBOL_CHAR \
    ALPHA: \
    case DIGIT: \
    case '_'

#define SYMBOL_START \
    ALPHA: \
    case '_'
//enum TokenEnum
//{
//    TYPE_U8,
//    TYPE_U16,
//    TYPE_U32,
//    TYPE_U64,
//    TYPE_S8,
//    TYPE_S16,
//    TYPE_S32,
//    TYPE_S64,
//    TYPE_F32,
//    TYPE_F64,
//    VALUE_U8,
//    VALUE_U16,
//    VALUE_U32,
//    VALUE_U64,
//    VALUE_S8,
//    VALUE_S16,
//    VALUE_S32,
//    VALUE_S64,
//    VALUE_F32,
//    VALUE_F64,
//    ARITHMETIC_SUM,
//    ARITHMETIC_SUBSTRACT,
//    ARITHMETIC_MULTIPLY,
//    ARITHMETIC_DIVIDE,
//    ARITHMETIC_MODULUS,
//    RELATIONAL_EQUAL,
//    RELATIONAL_NOT_EQUAL,
//    RELATIONAL_GREATER,
//    RELATIONAL_LESS,
//    RELATIONAL_GREATER_OR_EQUAL,
//    RELATIONAL_LESS_OR_EQUAL,
//    LOGICAL_AND,
//    LOGICAL_OR,
//    LOGICAL_NOT,
//    BITWISE_AND,
//    BITWISE_OR,
//    BITWISE_NOT,
//    BITWISE_XOR,
//    BITWISE_SHL,
//    BITWISE_SHR,
//    ASSIGN_ASSIGN,
//    ASSIGN_SUM,
//    ASSIGN_SUBSTRACT,
//    ASSIGN_MULTIPLY,
//    ASSIGN_DIVIDE,
//    ASSIGN_MODULUS,
//    ASSIGN_BIT_AND,
//    ASSIGN_BIT_OR,
//    ASSIGN_BIT_XOR,
//    ASSIGN_SHL,
//    ASSIGN_SHR,
//    PTR_OP,
//    DEREF_OP,
//    SIZE_OP,
//};

struct RedKeyword
{
    const char* text;
    TokenID id;
};

static const struct RedKeyword red_keywords[] =
        {
            "align", TOKEN_ID_KEYWORD_ALIGN,
        };

bool is_red_keyword(Buffer* buf)
{
    for (size_t i = 0; i < array_length(red_keywords); i++)
    {
        if (buf_eql_str(buf, red_keywords[i].text))
        {
            return true;
        }
    }
    return false;
}

static bool is_symbol_char(char c)
{
    switch (c)
    {
        case SYMBOL_CHAR:
            return true;
        default:
            return false;
    }
}

enum LexerState
{
    LEXER_STATE_START,
    LEXER_STATE_SYMBOL,
    LEXER_STATE_ZERO, // 0 (may lead to )0x
    LEXER_STATE_NUMBER, // 123, 0x123
    LEXER_STATE_NUMBER_DOT,
    LEXER_STATE_FLOAT,
    LEXER_STATE_STRING,
    LEXER_STATE_STRING_ESCAPE,
    LEXER_STATE_CHAR_LITERAL,
    LEXER_STATE_CHAR_LITERAL_END,
    LEXER_STATE_STAR,
    LEXER_STATE_SLASH,
    // TODO: more slash
    LEXER_STATE_BACKSLASH,
    LEXER_STATE_PERCENT,
    LEXER_STATE_DASH,
    LEXER_STATE_AMPERSAND,
    LEXER_STATE_CARET, // What is this?
    LEXER_STATE_BAR,
    LEXER_STATE_BANG,
    LEXER_STATE_EQUAL,
    LEXER_STATE_LESS_THAN,
    LEXER_STATE_LESS_THAN_LESS_THAN,
    LEXER_STATE_GREATER_THAN,
    LEXER_STATE_GREATER_THAN_GREATER_THAN,
    LEXER_STATE_DOT,
    LEXER_STATE_DOT_DOT,
    LEXER_STATE_AT,
    LEXER_STATE_CHAR_CODE,
    LEXER_STATE_PLUS,
    LEXER_STATE_LINE_STRING,
    LEXER_STATE_LINE_STRING_END,
    LEXER_STATE_LINE_STRING_CONTINUE,
    LEXER_STATE_ERROR,
};

struct Lexer
{
    Buffer* buffer;
    size_t position;
    LexerState state;
    List<Token> tokens;
    s32 line;
    s32 column;
    Token* current_token;
    LexingResult result;
    u32 radix;
    size_t char_code_index;
    bool unicode;
    u32 char_code;
    size_t remaining_code_units;
};

static void lexer_error(Lexer* l, const char* format, ...)
{
    l->state = LEXER_STATE_ERROR;
    l->result.error_line = l->line;
    l->result.error_column = l->column;

    va_list args;
    va_start(args, format);
    l->result.error = buf_vprintf(format, args);
    va_end(args);
}

static void set_token_id(Lexer* l, Token* token, TokenID id)
{
    token->id = id;
    if (id == TOKEN_ID_INT_LIT)
    {
        BigInt_init_unsigned(&token->data.int_lit.big_int, 0);
    }
    else if (id == TOKEN_ID_FLOAT_LIT)
    {
        BigFloat_init_32(&token->data.float_lit.big_float, 0.0f);
        token->data.float_lit.overflow = false;
    }
    else if (id == TOKEN_ID_STRING_LIT || id == TOKEN_ID_MULTILINE_STRING_LIT || id == TOKEN_ID_SYMBOL)
    {
        memset(&token->data.str_lit.str, 0, sizeof(Buffer));
        buf_resize(&token->data.str_lit.str, 0);
    }
}

static void begin_token(Lexer* l, TokenID id)
{
    assert(!l->current_token);
    l->tokens.add_one();
    Token* token = &l->tokens.last();
    token->start_line = l->line;
    token->start_column = l->column;
    token->start_position = l->position;

    set_token_id(l, token, id);
    l->current_token = token;
}

static void cancel_token(Lexer* l)
{
    l->tokens.pop();
    l->current_token = nullptr;
}

static void end_float_token(Lexer* l)
{
    u8* buffer_ptr = (u8*)buf_ptr(l->buffer) + l->current_token->start_position;
    size_t buffer_length = l->current_token->end_position - l->current_token->start_position;
    if (BigFloat_init_buffer(&l->current_token->data.float_lit.big_float, buffer_ptr, buffer_length))
    {
        l->current_token->data.float_lit.overflow = true;
    }
}

static void end_token(Lexer* l)
{
    Token* current_token = l->current_token;
    assert(current_token);
    current_token->end_position = l->position + 1;

    if (current_token->id == TOKEN_ID_FLOAT_LIT)
    {
        end_float_token(l);
    }
    else if (current_token->id == TOKEN_ID_SYMBOL)
    {
        char* token_memory = buf_ptr(l->buffer) + current_token->start_position;
        s32 token_len = (s32)(current_token->end_position - current_token->start_position);

        for (size_t i = 0; i < array_length(red_keywords); i++)
        {
            if (mem_eql_str(token_memory, token_len, red_keywords[i].text))
            {
                l->current_token->id = red_keywords[i].id;
                break;
            }
        }
    }
    l->current_token = nullptr;
}

static inline u32 get_digit_value(u8 c)
{
    if ('0' <= c && c <= '9')
    {
        return c - '0';
    }
    if ('A' <= c && c <= 'Z')
    {
        return c - 'A' + 10;
    }
    if ('a' <= c && c <= 'z')
    {
        return c - 'a' + 10;
    }

    return UINT32_MAX;
}

static void handle_string_escape(Lexer* l, u8 c)
{
    if (l->current_token->id == TOKEN_ID_CHAR_LIT)
    {
        l->current_token->data.char_lit.c = c;
        l->state = LEXER_STATE_CHAR_LITERAL_END;
    }
    else if (l->current_token->id == TOKEN_ID_STRING_LIT || l->current_token->id == TOKEN_ID_SYMBOL)
    {
        buf_append_char(&l->current_token->data.str_lit.str, c);
        l->state = LEXER_STATE_STRING;
    }
    else
    {
        UNREACHABLE;
    }
}

static inline const char* char_escape_to_str(u8 c)
{
    switch (c)
    {
        case '\0':
            return "\\0";
        case '\a':
            return "\\a";
        case '\b':
            return "\\b";
        case '\t':
            return "\\t";
        case '\n':
            return "\\n";
        case '\v':
            return "\\f";
        case '\r':
            return "\\r";
        default:
            return nullptr;
    }
}

static void invalid_char_error(Lexer* l, u8 c)
{
    if (c == '\r')
    {
        lexer_error(l, "Invalid carriage return, only '\\n' line endings are supported");
        return;
    }

    const char* sh = char_escape_to_str(c);
    if (sh)
    {
        lexer_error(l, "Invalid character: '%s'", sh);
        return;
    }

    if (isprint(c))
    {
        lexer_error(l, "Invalid character: '%c'", c);
    }

    lexer_error(l, "Invalid character: '\\x%02x'", c);
}

LexingResult lex(Buffer* buffer)
{
    Lexer l = {0};
    /* TODO: stack return may involve some kind of errors, check later */
    l.buffer = buffer;

    l.result.line_offsets.append(0);

    /* Skip UTF-8 BOM */
    if (buf_starts_with_mem(buffer, "\xEF\xBB\xBF", 3))
    {
        l.position += 3;
    }

    for (; l.position < buf_len(l.buffer); l.position += 1)
    {
        u8 c = buf_ptr(l.buffer)[l.position];

        switch (l.state)
        {
            case LEXER_STATE_ERROR:
                break;
            case LEXER_STATE_START:
            {
                switch (c)
                {
                    case WHITESPACE:
                        break;
                    case ALPHA:
                    /*case '_':*/
                        l.state = LEXER_STATE_SYMBOL;
                        begin_token(&l, TOKEN_ID_SYMBOL);
                        buf_append_char(&l.current_token->data.str_lit.str, c);
                        break;
                    case '0':
                        l.state = LEXER_STATE_ZERO;
                        begin_token(&l, TOKEN_ID_INT_LIT);
                        l.radix = 10;
                        BigInt_init_unsigned(&l.current_token->data.int_lit.big_int, 0);
                        break;
                    case DIGIT_NON_ZERO:
                        l.state = LEXER_STATE_NUMBER;
                        begin_token(&l, TOKEN_ID_INT_LIT);
                        l.radix = 10;
                        BigInt_init_unsigned(&l.current_token->data.int_lit.big_int, get_digit_value(c));
                        break;
                    case '"':
                        begin_token(&l, TOKEN_ID_STRING_LIT);
                        l.state = LEXER_STATE_STRING;
                        break;
                    case '\'':
                        begin_token(&l, TOKEN_ID_CHAR_LIT);
                        l.state = LEXER_STATE_CHAR_LITERAL;
                        break;
                    case '(':
                        begin_token(&l, TOKEN_ID_LEFT_PARENTHESIS);
                        l.state = LEXER_STATE_CHAR_LITERAL;
                        break;
                    case ')':
                        begin_token(&l, TOKEN_ID_RIGHT_PARENTHESIS);
                        end_token(&l);
                        break;
                    case ',':
                        begin_token(&l, TOKEN_ID_COMMA);
                        end_token(&l);
                        break;
                    case '?':
                        begin_token(&l, TOKEN_ID_QUESTION);
                        end_token(&l);
                        break;
                    case '{':
                        begin_token(&l, TOKEN_ID_LEFT_BRACE);
                        end_token(&l);
                        break;
                    case '}':
                        begin_token(&l, TOKEN_ID_RIGHT_BRACE);
                        end_token(&l);
                        break;
                    case '[':
                        begin_token(&l, TOKEN_ID_LEFT_BRACKET);
                        end_token(&l);
                        break;
                    case ']':
                        begin_token(&l, TOKEN_ID_RIGHT_BRACKET);
                        end_token(&l);
                        break;
                    case ';':
                        begin_token(&l, TOKEN_ID_SEMICOLON);
                        end_token(&l);
                        break;
                    case ':':
                        begin_token(&l, TOKEN_ID_COLON);
                        end_token(&l);
                        break;
                    case '#':
                        begin_token(&l, TOKEN_ID_HASH);
                        end_token(&l);
                        break;
                    case '*':
                        begin_token(&l, TOKEN_ID_STAR);
                        break;
                    case '/':
                        begin_token(&l, TOKEN_ID_SLASH);
                        break;
                    case '\\':
                        begin_token(&l, TOKEN_ID_MULTILINE_STRING_LIT);
                        l.state = LEXER_STATE_BACKSLASH;
                        break;
                    case '%':
                        begin_token(&l, TOKEN_ID_PERCENT);
                        l.state = LEXER_STATE_PERCENT;
                        break;
                    case '+':
                        begin_token(&l, TOKEN_ID_PLUS);
                        l.state = LEXER_STATE_PLUS;
                        break;
                    case '~':
                        begin_token(&l, TOKEN_ID_TILDE);
                        end_token(&l);
                        break;
                    case '@':
                        begin_token(&l, TOKEN_ID_AT);
                        end_token(&l);
                        break;
                    case '-':
                        begin_token(&l, TOKEN_ID_DASH);
                        l.state = LEXER_STATE_DASH;
                        break;
                    case '&':
                        begin_token(&l, TOKEN_ID_AMPERSAND);
                        l.state = LEXER_STATE_AMPERSAND;
                        break;
                    case '^':
                        begin_token(&l, TOKEN_ID_BIT_XOR);
                        l.state = LEXER_STATE_CARET;
                        break;
                    case '|':
                        begin_token(&l, TOKEN_ID_BIT_OR);
                        l.state = LEXER_STATE_BAR;
                        break;
                    case '=':
                        begin_token(&l, TOKEN_ID_EQ);
                        l.state = LEXER_STATE_EQUAL;
                        break;
                    case '!':
                        begin_token(&l, TOKEN_ID_BANG);
                        l.state = LEXER_STATE_BANG;
                        break;
                    case '<':
                        begin_token(&l, TOKEN_ID_CMP_LESS);
                        l.state = LEXER_STATE_LESS_THAN;
                        break;
                    case '>':
                        begin_token(&l, TOKEN_ID_CMP_GREATER);
                        l.state = LEXER_STATE_GREATER_THAN;
                        break;
                    case '.':
                        begin_token(&l, TOKEN_ID_DOT);
                        l.state = LEXER_STATE_DOT;
                        break;
                    default:
                        invalid_char_error(&l, c);
                }
                break;
            }
            case LEXER_STATE_DOT:
            {
                switch (c)
                {

                }
                break;
            }
            case LEXER_STATE_DOT_DOT:
            {
                switch (c)
                {

                }
                break;
            }
            case LEXER_STATE_GREATER_THAN:
            {
                switch (c)
                {

                }
                break;
            }
            case LEXER_STATE_GREATER_THAN_GREATER_THAN:
            {
                switch (c)
                {

                }
                break;
            }
            case LEXER_STATE_LESS_THAN:
            {
                switch (c)
                {

                }
                break;
            }
            case LEXER_STATE_LESS_THAN_LESS_THAN:
            {
                switch (c)
                {

                }
                break;
            }
            case LEXER_STATE_BANG:
            {
                switch (c)
                {

                }
                break;
            }
            case LEXER_STATE_EQUAL:
            {
                switch (c)
                {

                }
                break;
            }
            case LEXER_STATE_STAR:
            {
                switch (c)
                {

                }
                break;
            }
            case LEXER_STATE_PERCENT:
            {
                switch (c)
                {

                }
                break;
            }
            case LEXER_STATE_PLUS:
            {
                switch (c)
                {

                }
                break;
            }
            case LEXER_STATE_AMPERSAND:
            {
                switch (c)
                {

                }
                break;
            }
            case LEXER_STATE_CARET:
            {
                switch (c)
                {

                }
                break;
            }
            case LEXER_STATE_BAR:
            {
                switch (c)
                {

                }
                break;
            }
            case LEXER_STATE_SLASH:
            {
                switch (c)
                {

                }
                break;
            }
            case LEXER_STATE_BACKSLASH:
            {
                switch (c)
                {

                }
                break;
            }
            case LEXER_STATE_LINE_STRING:
            {
                switch (c)
                {
                    case '\n':
                        l.state = LEXER_STATE_LINE_STRING_END;
                        break;
                    default:
                        buf_append_char(&l.current_token->data.str_lit.str, c);
                        break;

                }
                break;
            }
            case LEXER_STATE_LINE_STRING_END:
            {
                switch (c)
                {
                    case WHITESPACE:
                        break;
                    case '\\':
                        l.state = LEXER_STATE_LINE_STRING_CONTINUE;
                        break;
                    default:
                        l.position -= 1;
                        end_token(&l);
                        l.state = LEXER_STATE_START;
                        continue;
                }
                break;
            }
            case LEXER_STATE_LINE_STRING_CONTINUE:
            {
                switch (c)
                {
                    case '\\':
                        l.state = LEXER_STATE_LINE_STRING;
                        buf_append_char(&l.current_token->data.str_lit.str, '\n');
                        break;
                    default:
                        invalid_char_error(&l, c);
                        break;
                }
                break;
            }
            case LEXER_STATE_AT:
            {
                switch (c)
                {

                }
                break;
            }
            case LEXER_STATE_SYMBOL:
            {
                switch (c)
                {
                    case SYMBOL_CHAR:
                        buf_append_char(&l.current_token->data.str_lit.str, c);
                        break;
                    default:
                        l.position -= 1;
                        end_token(&l);
                        l.state = LEXER_STATE_START;
                        continue;
                }
                break;
            }
            case LEXER_STATE_STRING:
            {
                switch (c)
                {
                    case '"':
                        end_token(&l);
                        l.state = LEXER_STATE_START;
                        break;
                    case '\n':
                        lexer_error(&l, "Newline not allowed in string literal");
                        break;
                    case '\\':
                        l.state = LEXER_STATE_STRING_ESCAPE;
                        break;
                    default:
                        buf_append_char(&l.current_token->data.str_lit.str, c);
                        break;
                }
                break;
            }
            case LEXER_STATE_STRING_ESCAPE:
            {
                switch (c)
                {
                    case 'x':
                    {
                        l.state = LEXER_STATE_CHAR_CODE;
                        l.radix = 16;
                        l.char_code = 0;
                        l.char_code_index = 0;
                        l.unicode = false;
                        break;
                    }
                    case 'u':
                        NOT_IMPLEMENTED;
                        break;
                    case 'n':
                        handle_string_escape(&l, '\n');
                        break;
                    case 'r':
                        handle_string_escape(&l, '\r');
                        break;
                    case '\\':
                        handle_string_escape(&l, '\\');
                        break;
                    case '\t':
                        handle_string_escape(&l, '\t');
                        break;
                    case '\'':
                        handle_string_escape(&l, '\'');
                        break;
                    case '"':
                        handle_string_escape(&l, '\"');
                        break;
                    default:
                        invalid_char_error(&l, c);
                }
                break;
            }
            case LEXER_STATE_CHAR_CODE:
            {
                NOT_IMPLEMENTED;
                break;
            }
            case LEXER_STATE_CHAR_LITERAL:
            {
                if (c == '\'')
                {
                    lexer_error(&l, "Expected character");
                }
                else if (c == '\\')
                {
                    l.state = LEXER_STATE_STRING_ESCAPE;
                }
                else if ((c >= 0x80 && c <= 0xbf) || c >= 0xf8)
                {
                    invalid_char_error(&l, c);
                }
                else if (c >= 0xc0 && c <= 0xdf)
                {
                    NOT_IMPLEMENTED;
                }
                else if (c >= 0xe0 && c <= 0xef)
                {
                    NOT_IMPLEMENTED;
                }
                else if (c >= 0xf0 && c <= 0xf7)
                {
                    NOT_IMPLEMENTED;
                }
                else
                {
                    l.current_token->data.char_lit.c = c;
                    l.state = LEXER_STATE_CHAR_LITERAL_END;
                }
                break;
            }
            case LEXER_STATE_CHAR_LITERAL_END:
            {
                switch (c)
                {
                    case '\'':
                        end_token(&l);
                        l.state = LEXER_STATE_START;
                        break;
                    default:
                        invalid_char_error(&l, c);
                }
                break;
            }
            case LEXER_STATE_ZERO:
            {
                switch (c)
                {
                    case 'b':
                        l.radix = 2;
                        l.state = LEXER_STATE_NUMBER;
                        break;
                    case 'o':
                        l.radix = 8;
                        l.state = LEXER_STATE_NUMBER;
                        break;
                    case 'x':
                        l.radix = 16;
                        l.state = LEXER_STATE_NUMBER;
                        break;
                    default:
                        l.position -= 1;
                        /* maybe buggy?*/
                        l.state = LEXER_STATE_NUMBER;
                        continue;
                }
                break;
            }
            case LEXER_STATE_NUMBER:
            {
                if (c == '_')
                {
                    invalid_char_error(&l, c);
                }
                else if (c == '.')
                {
                    l.state = LEXER_STATE_NUMBER_DOT;
                    break;
                }
                u32 digit_value = get_digit_value(c);
                if (digit_value >= l.radix)
                {
                    if  (is_symbol_char(c))
                    {
                        invalid_char_error(&l, c);
                    }
                    l.position -= 1;
                    end_token(&l);
                    l.state = LEXER_STATE_START;
                    continue;
                }
                BigInt digit_value_bi;
                BigInt_init_unsigned(&digit_value_bi, digit_value);
                BigInt radix_bi;
                BigInt_init_unsigned(&radix_bi, l.radix);
                BigInt multiplied;
                BigInt_mul(&multiplied, &l.current_token->data.int_lit.big_int, &radix_bi);
                BigInt_add(&l.current_token->data.int_lit.big_int, &multiplied, &digit_value_bi);
                break;
            }
            case LEXER_STATE_NUMBER_DOT:
            {
                if (c == '.')
                {
                    l.position -= 2;
                    end_token(&l);
                    l.state = LEXER_STATE_START;
                    continue;
                }
                if (l.radix != 16 && l.radix != 10)
                {
                    invalid_char_error(&l, c);
                }
                l.position -= 1;
                l.state = LEXER_STATE_FLOAT,
                assert(l.current_token->id == TOKEN_ID_INT_LIT);
                set_token_id(&l, l.current_token, TOKEN_ID_FLOAT_LIT);
                continue;
            }
            case LEXER_STATE_FLOAT:
            {
                NOT_IMPLEMENTED;
            }
            case LEXER_STATE_DASH:
            {
                switch (c)
                {
                    case '>':
                        set_token_id(&l, l.current_token, TOKEN_ID_ARROW);
                        end_token(&l);
                        l.state = LEXER_STATE_START;
                        break;
                    case '=':
                        set_token_id(&l, l.current_token, TOKEN_ID_MINUS_EQ);
                        end_token(&l);
                        l.state = LEXER_STATE_START;
                        break;
                    default:
                        l.position -= 1;
                        end_token(&l);
                        l.state = LEXER_STATE_START;
                        continue;
                }
                break;
            }
        }

        if (c == '\n')
        {
            l.result.line_offsets.append(l.position + 1);
            l.line += 1;
            l.column = 0;
        }
        else
        {
            l.column += 1;
        }
    }

    // EOF
    switch (l.state)
    {
        case LEXER_STATE_START:
        case LEXER_STATE_ERROR:
            break;
        case LEXER_STATE_NUMBER_DOT:
            lexer_error(&l, "Unterminated nuumber literal");
            break;
        case LEXER_STATE_STRING:
            lexer_error(&l, "Unterminated string literal");
            break;
        case LEXER_STATE_STRING_ESCAPE:
        case LEXER_STATE_CHAR_CODE:
            if (l.current_token->id == TOKEN_ID_STRING_LIT)
            {
                lexer_error(&l, "Unterminated string literal");
                break;
            }
            else if (l.current_token->id == TOKEN_ID_CHAR_LIT)
            {
                lexer_error(&l, "Unterminated character literal");
                break;
            }
            else
            {
                UNREACHABLE;
            }
            break;
        case LEXER_STATE_CHAR_LITERAL:
        case LEXER_STATE_CHAR_LITERAL_END:
            lexer_error(&l, "Unterminated character literal");
            break;
        case LEXER_STATE_SYMBOL:
        case LEXER_STATE_ZERO:
        case LEXER_STATE_NUMBER:
        case LEXER_STATE_FLOAT:
        case LEXER_STATE_STAR:
        case LEXER_STATE_SLASH:
        case LEXER_STATE_PERCENT:
        case LEXER_STATE_PLUS:
        case LEXER_STATE_DASH:
        case LEXER_STATE_AMPERSAND:
        case LEXER_STATE_CARET:
        case LEXER_STATE_BAR:
        case LEXER_STATE_EQUAL:
        case LEXER_STATE_BANG:
        case LEXER_STATE_LESS_THAN:
        case LEXER_STATE_LESS_THAN_LESS_THAN:
        case LEXER_STATE_GREATER_THAN_GREATER_THAN:
        case LEXER_STATE_GREATER_THAN:
        case LEXER_STATE_DOT:
        case LEXER_STATE_AT:
        case LEXER_STATE_LINE_STRING_END:
        case LEXER_STATE_LINE_STRING:
            end_token(&l);
        case LEXER_STATE_DOT_DOT:
        case LEXER_STATE_BACKSLASH:
        case LEXER_STATE_LINE_STRING_CONTINUE:
            lexer_error(&l, "Unexpected EOF");
            break;
    }

    if (l.state != LEXER_STATE_ERROR)
    {
        if (l.tokens.length > 0)
        {
            Token* last_token = &l.tokens.last();
            l.line = (s32)last_token->start_line;
            l.column = (s32)last_token->start_column;
            l.position = last_token->start_position;
        }
        else
        {
            l.position = 0;
        }
        begin_token(&l, TOKEN_ID_END_OF_FILE);
        end_token(&l);
        assert(!l.current_token);
    }

    return l.result;
}

//static void scan(Lexer* lexer, Token* token, size_t* token_bytes)
//{
//    while (true)
//    {
//        lexer->read_char();
//        if (lexer->cursor == ' ')
//            continue;
//        else if (lexer->cursor == '\n')
//            (lexer->line)++;
//        else break;
//    }
//
//    switch (lexer->cursor)
//    {
//        case '&':
//            if (lexer->read_char('&'))
//            {
//                token->t = TokenEnum::LOGICAL_AND;
//                return;
//            }
//            else
//            {
//                // TODO: ptr
//                token->t = TokenEnum::BITWISE_AND;
//                return;
//            }
//        case '|':
//            if (lexer->read_char('|'))
//            {
//                token->t = TokenEnum::LOGICAL_OR;
//                return;
//            }
//            else
//            {
//                token->t = TokenEnum::BITWISE_OR;
//                return;
//            }
//        case '=':
//            if (lexer->read_char('='))
//            {
//                token->t = TokenEnum::RELATIONAL_EQUAL;
//                return;
//            }
//            else
//            {
//                token->t = TokenEnum::ASSIGN_ASSIGN;
//                return;
//            }
//        case '!':
//            if (lexer->read_char('='))
//            {
//                token->t = TokenEnum::RELATIONAL_NOT_EQUAL;
//                return;
//            }
//            else
//            {
//                token->t = TokenEnum::LOGICAL_NOT;
//                return;
//            }
//        case '<':
//            if (lexer->read_char('='))
//            {
//                token->t = TokenEnum::RELATIONAL_LESS_OR_EQUAL;
//                return;
//            }
//            else if (lexer->read_char('<'))
//            {
//                token->t = TokenEnum::BITWISE_SHL;
//                return;
//            }
//            else
//            {
//                token->t = TokenEnum::RELATIONAL_LESS;
//                return;
//            }
//        case '>':
//            if (lexer->read_char('='))
//            {
//                token->t = TokenEnum::RELATIONAL_GREATER_OR_EQUAL;
//                return;
//            }
//            else if (lexer->read_char('>'))
//            {
//                token->t = TokenEnum::BITWISE_SHR;
//                return;
//            }
//            else
//            {
//                token->t = TokenEnum::RELATIONAL_GREATER;
//                return;
//            }
//            // more cases?
//    }
//
//    if (isdigit(lexer->cursor))
//    {
//        u64 v = 0;
//        do
//        {
//            v = 10 * v + char_to_int(lexer->cursor);
//            lexer->read_char();
//        } while (isdigit(lexer->cursor));
//
//        if (lexer->cursor != '.')
//        {
//            token->t = TokenEnum::VALUE_S32;
//            // TODO: check number
//            TokenS32* s32_token = (TokenS32*)token;
//            s32_token->value = (s32)v;
//            *token_bytes = sizeof(TokenS32);
//        }
//        else
//        {
//            token->t = TokenEnum::VALUE_F32;
//            // TODO: check number
//            TokenF32* f32_token = (TokenF32*)token;
//            f32 x = v;
//            f32 d = 10;
//            for (;;)
//            {
//                lexer->read_char();
//                if (!isdigit(lexer->cursor)) break;
//                x = x + char_to_int(lexer->cursor) / d;
//                d *= 10;
//            }
//            *token_bytes = sizeof(TokenF32);
//            return;
//        }
//    }
//
//    if (isalpha(lexer->cursor))
//    {
//
//    }
//
//}