//
// Created by David on 28/09/2020.
//

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "types.h"

static char src_buffer[10000];
char *src_it = &src_buffer[0];

char* file_load(const char *name) {
    FILE* file = fopen(name, "rb");
    assert(file);

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    printf("[FILE] File length: %ld.\n", length);
    assert(0 < length && (src_it + length) < src_it + COUNT_OF(src_buffer));
    fseek(file, 0, SEEK_SET);

    size_t rc = fread(src_it, 1, length, file);
    assert(rc == (size_t)length);
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
    TOKEN_NAME = 35,
    TOKEN_CHAR_LIT = 36,
    TOKEN_STRING_LIT = 37,
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

#define CASE_TO_STR(x) case(x): return #x

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
        CASE_TO_STR(TOKEN_NAME);
        CASE_TO_STR(TOKEN_CHAR_LIT);
        CASE_TO_STR(TOKEN_STRING_LIT);
        default:
            printf("Token not recognized: %d\n");
            assert(0);
    }
}

Token token_arr[10000];
u32 token_count = 0;

#define WORD_MAX_CHAR_COUNT 64
typedef char Word[WORD_MAX_CHAR_COUNT];
#define MAX_WORD_COUNT 1000
Word words[MAX_WORD_COUNT];
u16 word_count = 0;


static Token get_word(char *string, u32 *word_count);

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
        {
            u32 word_char_count = 0;
            Token token = get_word(src_it, &word_char_count);
            src_it += word_char_count - 1;
            return token;
        }
        default:
            printf("Char not dealt with: %c(%d)\n", ch, (s32)ch);
            assert(0);
    }
}

static bool is_delimiter(char c)
{
    bool delimiter = c == ' ' ||
                     c == '-' ||
                     c == ';' ||
                     c == ',' ||
                     c == EOF ||
                     c == 0;
    return delimiter;
}

#define IS_CHAR_DELIMITER(x) x == '\''
#define IS_STRING_DELIMITER(x) x == '"'
#define IS_QUOTE(x, i) (IS_CHAR_DELIMITER(x[i]) || IS_STRING_DELIMITER(x[i]) && (i > 0 ? (x[i-1] != '\'') : true))
static Token get_word(char *string, u32 *char_count)
{
    Token token = TOKEN_NAME;
    char word_buffer[WORD_MAX_CHAR_COUNT];
    char ch = *string;

    size_t i;
    for (i = 0; !is_delimiter(string[i]); i++)
    {
        word_buffer[i] = string[i];
        if (IS_QUOTE(string, i))
        {
            i++;
            break;
        }
    }
    word_buffer[i] = 0;
    *char_count = i;

    switch (ch)
    {
        case '\'':
        {
            token = TOKEN_CHAR_LIT;
        }
        case '\"':
        {
            token = TOKEN_STRING_LIT;
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
        default:
            printf("\nChar not taken into account: %c(%d)\n", ch, (s32)ch);
            assert(0);
            break;
    }
    strcpy(words[word_count++], word_buffer);
    // Only names come this far
    return token;
}

static void lex(void)
{
    while (*src_it != 0)
    {
        Token token = get_token();
        printf("Got token: %s\n", get_token_str(token));
        size_t token_index = token_count++;
        token_arr[token_index] = token;
        *src_it++;
    }
}

static void debug_lexing(void)
{
    size_t word_it = 0;
    for (int i = 0; i < token_count; i++)
    {
        Token token = token_arr[i];
        if (is_onechar(token))
        {
            printf("[%d] Token: %s\n", i, get_token_str(token));
        }
        else
        {
            printf("[%d] Token: %s\t\tValue: %s\n", i, get_token_str(token), words[word_it]);
            word_it++;
            assert(word_it <= word_count);
        }
    }
}

int main(void)
{
    const char *src_code = file_load("test.red");
    printf("Compiling:\n***\n%s\n***\n", src_code);
    strcpy(src_buffer, src_code);
    lex();
    debug_lexing();
    return 0;
}

