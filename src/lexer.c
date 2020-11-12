//
// Created by david on 7/10/20.
//

#include "compiler_types.h"
#include "lexer.h"
#include "os.h"
#include "bigint.h"
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

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

GEN_BUFFER_FUNCTIONS(tokb, tb, TokenBuffer, Token)
GEN_BUFFER_FUNCTIONS(uszbf, ub, UsizeBuffer, usize)

typedef struct RedKeyword
{
    const char* text;
    TokenID id;
} RedKeyword;

void print_token(SB* src_buffer, Token* token, u32 token_index)
{
    //fwrite(src_buffer->ptr + token->start_position, 1, token->end_position - token->start_position, stdout);
    PRINT_TOKEN_WITH_PREFIX("Printing", token, token_index, symbol_name);
}

void print_tokens(SB* src_buffer, TokenBuffer* tokens)
{
#if RED_LEXER_VERBOSE
    for (size_t i = 0; i < tokb_len(tokens); i++)
    {
        Token* token = &tokens->ptr[i];
        //logger(LOG_TYPE_INFO, "%s ", token_name(token->id));
        if (token->start_position != SIZE_MAX)
        {
            //fwrite(sb_ptr(src_buffer) + token->start_position, 1, token->end_position - token->start_position, stdout);
            print_token(src_buffer, token, i);
        }
    }
    fputc('\n', stdout);
#endif
}

/* TODO: SoA this*/
static const struct RedKeyword red_keywords[] =
{
    { "const", TOKEN_ID_KEYWORD_CONST, },
    { "else", TOKEN_ID_KEYWORD_ELSE, },
    { "enum", TOKEN_ID_KEYWORD_ENUM, },
    { "extern", TOKEN_ID_KEYWORD_EXTERN, },
    { "false", TOKEN_ID_KEYWORD_FALSE, },
    { "for", TOKEN_ID_KEYWORD_FOR, },
    { "if", TOKEN_ID_KEYWORD_IF, },
    { "null", TOKEN_ID_KEYWORD_NULL, },
    { "return", TOKEN_ID_KEYWORD_RETURN, },
    { "struct", TOKEN_ID_KEYWORD_STRUCT, },
    { "switch", TOKEN_ID_KEYWORD_SWITCH, },
    { "true", TOKEN_ID_KEYWORD_TRUE, },
    { "undefined", TOKEN_ID_KEYWORD_UNDEFINED, },
    { "union", TOKEN_ID_KEYWORD_UNION, },
    { "var", TOKEN_ID_KEYWORD_VAR, },
    { "void", TOKEN_ID_KEYWORD_VOID, },
    { "while", TOKEN_ID_KEYWORD_WHILE, },
};

bool is_red_keyword_id(TokenID id)
{
    for (size_t i = 0; i < array_length(red_keywords); i++)
    {
        if (id == red_keywords[i].id)
        {
            return true;
        }
    }

    return false;
}

bool is_red_keyword_sb(SB* sb)
{
    for (size_t i = 0; i < array_length(red_keywords); i++)
    {
        if (strcmp(sb->ptr, red_keywords[i].text))
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

typedef enum LexerState
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
    LEXER_STATE_CHAR_CODE,
    LEXER_STATE_PLUS,
    LEXER_STATE_LINE_STRING,
    LEXER_STATE_LINE_STRING_END,
    LEXER_STATE_LINE_STRING_CONTINUE,
    LEXER_STATE_ERROR,
} LexerState;

typedef struct Lexer
{
    LexingResult result;
    SB* src_buffer;
    Token* current_token;
    LexerState state;
    size_t position;
    s32 line;
    s32 column;
    u32 radix;
} Lexer;

static void lexer_error(Lexer* l, const char* format, ...)
{
    l->state = LEXER_STATE_ERROR;
    l->result.error_line = l->line;
    l->result.error_column = l->column;

    va_list args;
    va_start(args, format);
    sb_vprintf(&l->result.error, format, args);
    va_end(args);
    print("Error: %s\n", sb_ptr(&l->result.error));
    os_exit(1);
}

static void set_token_id(Lexer* l, Token* token, TokenID id)
{
    token->id = id;
    if (id == TOKEN_ID_INT_LIT)
    {
        BigInt_init_unsigned(&token->int_lit.big_int, 0);
    }
    else if (id == TOKEN_ID_FLOAT_LIT)
    {
        RED_NOT_IMPLEMENTED;
        //BigFloat_init_32(&token->float_lit.big_float, 0.0f);
        //token->float_lit.overflow = false;
    }
    else if (id == TOKEN_ID_STRING_LIT || id == TOKEN_ID_MULTILINE_STRING_LIT || id == TOKEN_ID_SYMBOL)
    {
        sb_clear(&token->str_lit.str);
        sb_resize(&token->str_lit.str, 0);
    }
}

static void begin_token(Lexer* l, TokenID id)
{
    redassert(!l->current_token);
    Token* token = tokb_add_one(&l->result.tokens);
    token->start_line = l->line;
    token->start_column = l->column;
    token->start_position = l->position;

    set_token_id(l, token, id);
    l->current_token = token;
}

static void end_float_token(Lexer* l)
{
    RED_NOT_IMPLEMENTED;
    //u8* buffer_ptr = (u8*)(l->buffer) + l->current_token->start_position;
    //size_t buffer_length = l->current_token->end_position - l->current_token->start_position;
    //if (BigFloat_init_buffer(&l->current_token->float_lit.big_float, buffer_ptr, buffer_length))
    //{
    //    l->current_token->float_lit.overflow = true;
    //}
}

static void end_token(Lexer* l)
{
    Token* current_token = l->current_token;
    redassert(current_token);
    current_token->end_position = l->position + 1;

    if (current_token->id == TOKEN_ID_FLOAT_LIT)
    {
        end_float_token(l);
    }
    else if (current_token->id == TOKEN_ID_SYMBOL)
    {
        char* token_str = sb_ptr(l->src_buffer) + current_token->start_position;
        s32 token_len = (s32)(current_token->end_position - current_token->start_position);

        for (size_t i = 0; i < array_length(red_keywords); i++)
        {
            char* kw = (char*)red_keywords[i].text;
            usize kw_len = strlen(kw);
            if (token_len == kw_len && strncmp(token_str, kw, token_len) == 0)
            {
                l->current_token->id = red_keywords[i].id;
                break;
            }
        }
    }
    l->current_token = NULL;
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
        l->current_token->char_lit.value = c;
        l->state = LEXER_STATE_CHAR_LITERAL_END;
    }
    else if (l->current_token->id == TOKEN_ID_STRING_LIT || l->current_token->id == TOKEN_ID_SYMBOL)
    {
        sb_append_char(&l->current_token->str_lit.str, c);
        l->state = LEXER_STATE_STRING;
    }
    else
    {
        RED_UNREACHABLE;
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
            return NULL;
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
        lexer_error(l, "Invalid character: '%value'", c);
    }

    lexer_error(l, "Invalid character: '\\x%02x'", c);
}

LexingResult lex_file(SB* src_buffer)
{
    //ScopeTimer lexer_time("Lexer");
    Lexer l = {0};
    /* TODO: stack return may involve some kind of errors, check later */
    l.src_buffer = src_buffer;
    uszbf_append(&l.result.line_offsets, 0);

    /* Skip UTF-8 BOM */
    //if (buf_starts_with_mem(buffer, "\xEF\xBB\xBF", 3))
    //{
    //    l.position += 3;
    //}

    for (; l.position < sb_len(l.src_buffer); l.position += 1)
    {
        u8 c = l.src_buffer->ptr[l.position];
        
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
                    case SYMBOL_START:
                        l.state = LEXER_STATE_SYMBOL;
                        begin_token(&l, TOKEN_ID_SYMBOL);
                        sb_append_char(&l.current_token->str_lit.str, c);
                        break;
                    case '0':
                        l.state = LEXER_STATE_ZERO;
                        begin_token(&l, TOKEN_ID_INT_LIT);
                        l.radix = 10;
                        BigInt_init_unsigned(&l.current_token->int_lit.big_int, 0);
                        break;
                    case DIGIT_NON_ZERO:
                        l.state = LEXER_STATE_NUMBER;
                        begin_token(&l, TOKEN_ID_INT_LIT);
                        l.radix = 10;
                        BigInt_init_unsigned(&l.current_token->int_lit.big_int, get_digit_value(c));
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
                        end_token(&l);
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
                        begin_token(&l, TOKEN_ID_CARET);
                        l.state = LEXER_STATE_CARET;
                        break;
                    case '|':
                        begin_token(&l, TOKEN_ID_BAR);
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
                        end_token(&l);
                        break;
                    default:
                        invalid_char_error(&l, c);
                }
                break;
            }
//            case LEXER_STATE_DOT:
//            {
//                switch (value)
//                {
//                    case '.':
//
//                }
//                break;
//            }
//            case LEXER_STATE_DOT_DOT:
//            {
//                switch (value)
//                {
//
//                }
//                break;
//            }
            case LEXER_STATE_GREATER_THAN:
            {
                switch (c)
                {
                    case '=':
                        set_token_id(&l, l.current_token, TOKEN_ID_CMP_GREATER_OR_EQ);
                        end_token(&l);
                        l.state = LEXER_STATE_START;
                        break;
                    case '>':
                        set_token_id(&l, l.current_token, TOKEN_ID_BIT_SHR);
                        l.state = LEXER_STATE_GREATER_THAN_GREATER_THAN;
                        break;
                    default:
                        l.position -= 1;
                        end_token(&l);
                        l.state = LEXER_STATE_START;
                        continue;
                }
                break;
            }
            case LEXER_STATE_GREATER_THAN_GREATER_THAN:
            {
                switch (c)
                {
                    case '=':
                        set_token_id(&l, l.current_token, TOKEN_ID_BIT_SHR_EQ);
                        end_token(&l);
                        l.state = LEXER_STATE_START;
                        break;
                    default:
                        l.position -= 1;
                        end_token(&l);
                        l.state = LEXER_STATE_START;
                        break;
                }
                break;
            }
            case LEXER_STATE_LESS_THAN:
            {
                switch (c)
                {
                    case '=':
                        set_token_id(&l, l.current_token, TOKEN_ID_CMP_LESS_OR_EQ);
                        end_token(&l);
                        l.state = LEXER_STATE_START;
                        break;
                    case '<':
                        set_token_id(&l, l.current_token, TOKEN_ID_BIT_SHL);
                        l.state = LEXER_STATE_LESS_THAN_LESS_THAN;
                        break;
                    default:
                        l.position -= 1;
                        end_token(&l);
                        l.state = LEXER_STATE_START;
                        continue;
                }
                break;
            }
            case LEXER_STATE_LESS_THAN_LESS_THAN:
            {
                switch (c)
                {
                    case '=':
                        set_token_id(&l, l.current_token, TOKEN_ID_BIT_SHL_EQ);
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
            case LEXER_STATE_BANG:
            {
                switch (c)
                {
                    case '=':
                        set_token_id(&l, l.current_token, TOKEN_ID_CMP_NOT_EQ);
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
            case LEXER_STATE_EQUAL:
            {
                switch (c)
                {
                    case '=':
                        set_token_id(&l, l.current_token, TOKEN_ID_CMP_EQ);
                        end_token(&l);
                        l.state = LEXER_STATE_START;
                        break;
                    case '>':
                        set_token_id(&l, l.current_token, TOKEN_ID_FAT_ARROW);
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
            case LEXER_STATE_STAR:
            {
                switch (c)
                {
                    case '=':
                        set_token_id(&l, l.current_token, TOKEN_ID_TIMES_EQ);
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
            case LEXER_STATE_PERCENT:
            {
                switch (c)
                {
                    case '=':
                        set_token_id(&l, l.current_token, TOKEN_ID_MOD_EQ);
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
            case LEXER_STATE_PLUS:
            {
                switch (c)
                {
                    case '=':
                        set_token_id(&l, l.current_token, TOKEN_ID_PLUS_EQ);
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
            case LEXER_STATE_AMPERSAND:
            {
                switch (c)
                {
                    case '&':
                        lexer_error(&l, "\'&&\' is invalid. For boolean AND, use the \'and\' keyword");
                        break;
                    case '=':
                        set_token_id(&l, l.current_token, TOKEN_ID_BIT_AND_EQ);
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
            case LEXER_STATE_CARET:
            {
                switch (c)
                {
                    case '=':
                        set_token_id(&l, l.current_token, TOKEN_ID_BIT_XOR_EQ);
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
            case LEXER_STATE_BAR:
            {
                switch (c)
                {
                    case '=':
                        set_token_id(&l, l.current_token, TOKEN_ID_BIT_OR_EQ);
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
            case LEXER_STATE_SLASH:
            {
                switch (c)
                {
                    case '=':
                        set_token_id(&l, l.current_token, TOKEN_ID_DIV_EQ);
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
            case LEXER_STATE_BACKSLASH:
            {
                switch (c)
                {
                    case '\\':
                        l.state = LEXER_STATE_LINE_STRING;
                        break;
                    default:
                        invalid_char_error(&l, c);
                        break;
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
                        sb_append_char(&l.current_token->str_lit.str, c);
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
                        sb_append_char(&l.current_token->str_lit.str, c);
                        break;
                    default:
                        invalid_char_error(&l, c);
                        break;
                }
                break;
            }
//            case LEXER_STATE_AT:
//            {
//                switch (value)
//                {
//                    case '"':
//                }
//                break;
//            }
            case LEXER_STATE_SYMBOL:
            {
                switch (c)
                {
                    case SYMBOL_CHAR:
                        sb_append_char(&l.current_token->str_lit.str, c);
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
                        sb_append_char(&l.current_token->str_lit.str, c);
                        break;
                }
                break;
            }
            case LEXER_STATE_STRING_ESCAPE:
            {
                switch (c)
                {
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
                RED_NOT_IMPLEMENTED;
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
                    RED_NOT_IMPLEMENTED;
                }
                else if (c >= 0xe0 && c <= 0xef)
                {
                    RED_NOT_IMPLEMENTED;
                }
                else if (c >= 0xf0 && c <= 0xf7)
                {
                    RED_NOT_IMPLEMENTED;
                }
                else
                {
                    l.current_token->char_lit.value = c;
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
                    if (is_symbol_char(c))
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
                BigInt_mul(&multiplied, &l.current_token->int_lit.big_int, &radix_bi);
                BigInt_add(&l.current_token->int_lit.big_int, &multiplied, &digit_value_bi);
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
                l.state = LEXER_STATE_FLOAT;
                redassert(l.current_token->id == TOKEN_ID_INT_LIT);
                set_token_id(&l, l.current_token, TOKEN_ID_FLOAT_LIT);
                continue;
            }
            case LEXER_STATE_FLOAT:
            {
                RED_NOT_IMPLEMENTED;
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
            uszbf_append(&l.result.line_offsets, l.position + 1);
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
                RED_UNREACHABLE;
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
        case LEXER_STATE_LINE_STRING_END:
        case LEXER_STATE_LINE_STRING:
            end_token(&l);
        case LEXER_STATE_BACKSLASH:
        case LEXER_STATE_LINE_STRING_CONTINUE:
            lexer_error(&l, "Unexpected EOF");
            break;
    }

    if (l.state != LEXER_STATE_ERROR)
    {
        if (tokb_len(&l.result.tokens) > 0)
        {
            Token* last_token = tokb_last(&l.result.tokens);
            l.line = (s32)last_token->start_line;
            l.column = (s32)last_token->start_column;
            l.position = last_token->start_position + 1;
        }
        else
        {
            l.position = 0;
        }
        begin_token(&l, TOKEN_ID_END_OF_FILE);
        end_token(&l);
        redassert(!l.current_token);
    }

    return l.result;
}

const char* token_name(TokenID id)
{
    switch (id)
    {
        //case TOKEN_ID_BIT_AND: return "&";
        case TOKEN_ID_AMPERSAND: return "&";
        case TOKEN_ID_ARROW: return "->";
        case TOKEN_ID_AT: return "@";
        case TOKEN_ID_BANG: return "!";
        case TOKEN_ID_BAR: return "|";
        case TOKEN_ID_CARET: return "^";
        case TOKEN_ID_BIT_SHL: return "<<";
        case TOKEN_ID_BIT_SHR: return ">>";
        case TOKEN_ID_BIT_XOR_EQ: return "^=";
        case TOKEN_ID_BIT_OR_EQ: return "&=";
        case TOKEN_ID_BIT_AND_EQ: return "&=";
        case TOKEN_ID_BIT_SHL_EQ: return "<<=";
        case TOKEN_ID_BIT_SHR_EQ: return ">>=";
        case TOKEN_ID_CHAR_LIT: return "CharLiteral";
        case TOKEN_ID_CMP_EQ: return "==";
        case TOKEN_ID_CMP_NOT_EQ: return "!=";
        case TOKEN_ID_CMP_GREATER: return ">";
        case TOKEN_ID_CMP_GREATER_OR_EQ: return ">=";
        case TOKEN_ID_CMP_LESS: return "<=";
        case TOKEN_ID_CMP_LESS_OR_EQ: return "<=";
        case TOKEN_ID_COLON: return ";";
        case TOKEN_ID_COMMA: return ",";
        case TOKEN_ID_DASH: return "-";
        case TOKEN_ID_DIV_EQ: return "/=";
        case TOKEN_ID_DOT: return ".";
        case TOKEN_ID_END_OF_FILE: return "EOF";
        case TOKEN_ID_EQ: return "=";
        case TOKEN_ID_FAT_ARROW: return "=>";
        case TOKEN_ID_FLOAT_LIT: return "FloatLiteral";
        case TOKEN_ID_INT_LIT: return "IntLiteral";
        case TOKEN_ID_KEYWORD_ALIGN:
        case TOKEN_ID_KEYWORD_ALLOW_ZERO:
        case TOKEN_ID_KEYWORD_AND:
        case TOKEN_ID_KEYWORD_ANY:
        case TOKEN_ID_KEYWORD_ANY_FRAME:
        case TOKEN_ID_KEYWORD_CALL_CONV:
        case TOKEN_ID_KEYWORD_COMPTIME:
        case TOKEN_ID_KEYWORD_CONST:
        case TOKEN_ID_KEYWORD_DEFER:
        case TOKEN_ID_KEYWORD_ELSE:
        case TOKEN_ID_KEYWORD_ENUM:
        case TOKEN_ID_KEYWORD_ERROR:
        case TOKEN_ID_KEYWORD_ERROR_DEFER:
        case TOKEN_ID_KEYWORD_EXPORT:
        case TOKEN_ID_KEYWORD_EXTERN:
        case TOKEN_ID_KEYWORD_FALSE:
        case TOKEN_ID_KEYWORD_FN:
        case TOKEN_ID_KEYWORD_FOR:
        case TOKEN_ID_KEYWORD_IF:
        case TOKEN_ID_KEYWORD_INLINE:
        case TOKEN_ID_KEYWORD_NO_ALIAS:
        case TOKEN_ID_KEYWORD_NO_INLINE:
        case TOKEN_ID_KEYWORD_NULL:
        case TOKEN_ID_KEYWORD_OR:
        case TOKEN_ID_KEYWORD_PACKED:
        case TOKEN_ID_KEYWORD_PUB:
        case TOKEN_ID_KEYWORD_RETURN: return "return";
        case TOKEN_ID_KEYWORD_SECTION: return "section";
        case TOKEN_ID_KEYWORD_STRUCT: return "struct";
        case TOKEN_ID_KEYWORD_SWITCH: return "switch";
        case TOKEN_ID_KEYWORD_TEST: return "test";
        case TOKEN_ID_KEYWORD_THREAD_LOCAL:
        case TOKEN_ID_KEYWORD_TRUE: return "true";
        case TOKEN_ID_KEYWORD_UNDEFINED: return "undefined";
        case TOKEN_ID_KEYWORD_UNION: return "union";
        case TOKEN_ID_KEYWORD_VAR: return "var";
        case TOKEN_ID_KEYWORD_VOLATILE: return "volatile";
        case TOKEN_ID_KEYWORD_WHILE: return "while";
        case TOKEN_ID_LEFT_BRACE: return "{";
        case TOKEN_ID_LEFT_BRACKET: return "[";
        case TOKEN_ID_LEFT_PARENTHESIS: return "(";
        case TOKEN_ID_QUESTION: return "?";
        case TOKEN_ID_MINUS_EQ: return "-=";
        case TOKEN_ID_MOD_EQ: return "%=";
        case TOKEN_ID_HASH: return "#";
        case TOKEN_ID_PERCENT: return "%";
        case TOKEN_ID_PLUS: return "+";
        case TOKEN_ID_PLUS_EQ: return "+=";
        case TOKEN_ID_RIGHT_BRACE: return "}";
        case TOKEN_ID_RIGHT_BRACKET: return "]";
        case TOKEN_ID_RIGHT_PARENTHESIS: return ")";
        case TOKEN_ID_SEMICOLON: return ";";
        case TOKEN_ID_SLASH: return "/";
        case TOKEN_ID_STAR: return "*";
        case TOKEN_ID_STRING_LIT: return "StringLiteral";
        case TOKEN_ID_MULTILINE_STRING_LIT: return "MultilineStringLiteral";
        case TOKEN_ID_SYMBOL: return "Symbol";
        case TOKEN_ID_TILDE: return "~";
        case TOKEN_ID_TIMES_EQ: return "*=";
        default:
            RED_NOT_IMPLEMENTED;
            break;
    }
    return NULL;
}
