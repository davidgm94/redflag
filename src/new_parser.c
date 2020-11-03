/*
    * Fn declaration - NO
    * Fn prototype - OK
    * Type = OK
    * Parameter declaration = OK
    * Block = OK
    * Return expression = OK
    * Variable declaration = OK
    * Symbol expression = NO
    * Binary operation expression = OK
    * Function call expression = NO
    * Branch expression = OK
    * Int literal = OK
*/
#include "compiler_types.h"
#include "lexer.h"
#include "os.h"
#include <stdarg.h>
#include <stdio.h>
typedef enum ASTType
{
    FN_DEF,
    FN_DECL,
    FN_PROTO,
    TYPE,
    PARAM_DECL,
    BLOCK,
    RETURN_EXPR,
    VAR_DECL,
    SYM_EXPR,
    BIN_EXPR,
    FN_CALL,
    BRANCH_EXPR,
    INT_LIT,
} ASTType;

    struct Node;
    
    typedef struct Symbol
    {
        SB* name;
    } Symbol;

    typedef struct Type
    {
        SB* type_name;
    } Type;

    typedef struct ParamDecl
    {
        Symbol sym;
        Type type;
    } ParamDecl;

    typedef struct SymDecl
    {
        Symbol sym;
        Type type;
        struct Node* value;
    } SymDecl;

    typedef struct IntLit
    {
        BigInt* bigint;
    } IntLit;

    typedef struct BinExpr
    {
        TokenID op;
        struct Node* left;
        struct Node* right;
    } BinExpr;

    typedef struct RetExpr
    {
        struct Node* ret_value;
    } RetExpr;

    typedef struct BlockExpr
    {
        struct Node** statements;
    } BlockExpr;

    typedef struct BranchExpr
    {
        struct Node* condition;
        struct Node* true_block;
        struct Node* false_block;
    } BranchExpr;

    typedef struct FnProto
    {
        Symbol sym;
        ParamDecl* params;
        Type ret_type;
    } FnProto;

    typedef struct FnDef
    {
        struct Node* proto;
        struct Node* body;
    } FnDef;

    typedef struct Node
    {
        ASTType node_type;
        u32 line;
        u32 column;

        union
        {
            Symbol sym_expr;
            Type type;
            ParamDecl param;
            SymDecl sym_decl;
            IntLit int_lit;
            BinExpr bin_expr;
            RetExpr ret_expr;
            BlockExpr block_expr;
            BranchExpr branch_expr;
            FnProto fn_proto;
            FnDef fn_def;
        };
    } Node;

    typedef struct ParseContext
    {
        SB* buffer;
        Token* tokens;
        size_t current_token;
        //RedType* owner;
    } ParseContext;

    static void error(ParseContext* pc, Token* token, const char* format, ...)
    {
        fprintf(stdout, "error parsing token %s at line %zu column %zu: ", token_name(token->id), token->start_line + 1, token->start_column + 1);
        va_list args;
        va_start(args, format);
        vfprintf(stdout, format, args);
        va_end(args);
        os_exit(1);
    }

    static inline void invalid_token_error(ParseContext* pc, Token* token)
    {
        error(pc, token, "invalid token: '%s'", token_name(token->id));
    }

    static inline Token* get_token_i(ParseContext* pc, size_t i)
    {
        return &pc->tokens[pc->current_token + i];
    }

    static inline Token* get_token(ParseContext* pc)
    {
        return get_token_i(pc, 0);
    }

    static inline Token* consume_token(ParseContext* pc)
    {
        Token* token = get_token(pc);

#if RED_PARSER_VERBOSE
        PRINT_TOKEN_WITH_PREFIX("Consuming", token, symbol_name);
#endif
        pc->current_token += 1;
        return token;
    }

    static inline Token* consume_token_if(ParseContext* pc, TokenID id)
    {
        Token* eaten = get_token(pc);
        if (eaten->id == id)
        {
            return consume_token(pc);
        }
        return NULL;
    }

    static inline Token* expect_token(ParseContext* pc, TokenID id)
    {
        Token* token = consume_token(pc);
        if (token->id != id)
        {
            error(pc, token, "expected token '%s', found '%s'", token_name(id), token_name(token->id));
        }

        return token;
    }

    static inline void put_back_token(ParseContext* pc)
    {
#if RED_PARSER_VERBOSE
        Token* wrong_token = get_token(pc);
#endif
        pc->current_token -= 1;
#if RED_PARSER_VERBOSE
        Token* good_token = get_token(pc);
        StringBuffer* wrong_symbol = wrong_token->id == TOKEN_ID_SYMBOL ? token_buffer(wrong_token) : NULL;
        StringBuffer* good_symbol = good_token->id == TOKEN_ID_SYMBOL ? token_buffer(good_token) : NULL;
        print("Current token #%zu: %s name: %s ******** Putting back token #%zu: %s name: %s\n", pc->current_token + 1, token_name(wrong_token->id), wrong_symbol ? wrong_symbol->ptr : "not a symbol", pc->current_token, token_name(good_token->id), good_symbol ? good_symbol->ptr : "not a symbol");
#endif
    }


    Node* parse_top_level_statement(ParseContext* pc);
