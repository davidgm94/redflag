#pragma once
#include <vector>

namespace llvm
{
    class Value;
    class Function;
}
namespace RedIR
{
    struct Context;
}

namespace RedAST
{
    static inline Buffer* token_buffer(Token* token)
    {
        if (token == nullptr)
        {
            return nullptr;
        }
        redassert(token->id == TOKEN_ID_STRING_LIT || token->id == TOKEN_ID_MULTILINE_STRING_LIT || token->id == TOKEN_ID_SYMBOL);
        return &token->str_lit.str;
    }

    static inline BigInt* token_bigint(Token* token)
    {
        if (token == nullptr)
        {
            return nullptr;
        }
        redassert(token->id == TOKEN_ID_INT_LIT);
        return &token->int_lit.big_int;
    }

    static inline BigFloat* token_bigfloat(Token* token)
    {
        if (token == nullptr)
        {
            return nullptr;
        }
        redassert(token->id == TOKEN_ID_FLOAT_LIT);
        return &token->float_lit.big_float;
    }

    static inline bool token_is_binop(TokenID op)
    {
        bool is_it = op == TOKEN_ID_PLUS ||
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

    enum ASTNodeType : u32
    {
        COMMON,
        STATEMENT_BRANCH,
        STATEMENT_FOR,
        STATEMENT_WHILE,
        STATEMENT_DO_WHILE,
        STATEMENT_RETURN,
        STATEMENT_SWITCH,
        STATEMENT_GOTO,
        STATEMENT_BREAK,
        STATEMENT_CONTINUE,
        STATEMENT_VAR_DECL,
        STATEMENT_SYMBOL_OP,
        /* should this be symbol_op?
        STATEMENT_FN_CALL_EXPR,
        */
        STATEMENT_BOOL_EXPR,
        STATEMENT_INT_EXPR,
        STATEMENT_BIN_EXPR,
        STATEMENT_META,
    };

    struct Expression
    {
        u32 line;
        u32 column;
        ASTNodeType type;

        Expression(Token* token, ASTNodeType ast_node_type = COMMON)
        {
            line = token->start_line;
            column = token->start_column;
            type = ast_node_type;
        }

        virtual void print() = 0;
        virtual llvm::Value* codegen(RedIR::Context* context) = 0;
    };

    struct IntExpr: public Expression
    {
        BigInt* bigint;
        IntExpr(Token* token)
            :  Expression(token, STATEMENT_INT_EXPR), bigint(token_bigint(token))
        {}

        virtual void print();
        virtual llvm::Value* codegen(RedIR::Context* context);
    };

    struct FloatExpr : public Expression
    {
        BigFloat* bigfloat;
        FloatExpr(Token* token)
            :  Expression(token), bigfloat(token_bigfloat(token))
        {}

        virtual void print();
        virtual llvm::Value* codegen(RedIR::Context* context);
    };

    struct SymbolExpr : public Expression
    {
        Buffer* name;
        SymbolExpr(Token* name_token)
            : Expression(name_token), name(token_buffer(name_token))
        {}

        virtual void print();
        virtual llvm::Value* codegen(RedIR::Context* context);
    };

    struct VariableExpr : public Expression
    {
        Buffer* name;
        Buffer* type;

        VariableExpr(Token* name_token, Token* type_token)
            : Expression(name_token), name(token_buffer(name_token)), type(token_buffer(type_token))
        { }

        virtual void print();
        virtual llvm::Value* codegen(RedIR::Context* context);
    };

    struct BinaryExpr : public Expression
    {
        TokenID op;
        Expression* left;
        Expression* right;

        BinaryExpr(Token* bin_op_token, Expression* left, Expression* right)
            : Expression(bin_op_token, STATEMENT_BIN_EXPR), op(bin_op_token->id), left(left), right(right)
        { }

        virtual void print();
        virtual llvm::Value* codegen(RedIR::Context* context);
    };

    struct ReturnExpr : public Expression
    {
        Expression* return_expr;

        ReturnExpr(Token* ret_keyword_tok, Expression* ret_expr)
            : Expression(ret_keyword_tok, STATEMENT_RETURN), return_expr(ret_expr)
        { }

        virtual void print();
        virtual llvm::Value* codegen(RedIR::Context* context);
    };

    struct CallExpr : public Expression
    {
        Buffer* callee;
        std::vector<Expression*> args;

        virtual void print();
        virtual llvm::Value* codegen(RedIR::Context* context);
    };

    struct BoolExpr : public Expression
    {
        bool value;
        BoolExpr(Token* bool_lit_token, bool value)
            : Expression(bool_lit_token, STATEMENT_BOOL_EXPR), value(value)
        { }

        virtual void print();
        virtual llvm::Value* codegen(RedIR::Context* context);
    };

    struct BlockExpr : public Expression
    {
        std::vector<Expression*>* expressions_in_block;
        BlockExpr(Token* token, std::vector<Expression*>* expr_vec)
            : Expression(token)
        {
            expressions_in_block = expr_vec;
        }

        virtual void print();
        virtual llvm::Value* codegen(RedIR::Context* context);
    };

    struct BranchExpr : public Expression
    {
        Expression* condition;
        BlockExpr* true_block;
        BlockExpr* false_block;

        BranchExpr(Token* token, Expression* condition, BlockExpr* true_block, BlockExpr* false_block)
            : Expression(token, STATEMENT_BRANCH), condition(condition), true_block(true_block), false_block(false_block)
        { }

        virtual void print();
        virtual llvm::Value* codegen(RedIR::Context* context);
    };

    struct Prototype
    {
        Buffer* name;
        std::vector<VariableExpr*> args;
        Buffer* return_type;

        Prototype(Token* name_token, std::vector<VariableExpr*> args, Token* return_type_token)
            : name(token_buffer(name_token)), args(args), return_type(token_buffer(return_type_token))
        { }

        virtual void print();
        virtual llvm::Function* codegen(RedIR::Context* context);
    };

    struct Function
    {
        Prototype* proto;
        BlockExpr* body;

        Function(Prototype* proto, BlockExpr* body)
            : proto(proto), body(body)
        {}

        virtual void print();
        virtual llvm::Function* codegen(RedIR::Context* context);
    };

    template <typename T>
    struct LLVMMap
    {
        Buffer* keys[10000];
        T* values[10000];
        size_t count;

        T* find_value(Buffer* key)
        {
            for (usize i = 0; i < count; i++)
            {
                bool found = buf_eql_buf(key, keys[i]);
                if (found)
                {
                    return values[i];
                }
            }

            return nullptr;
        }

        void append(Buffer* key, T* value)
        {
            redassert(count + 1 < 10000);
            keys[count] = key;
            values[count] = value;
            count++;
        }

        void clear()
        {
            memset(keys, 0, sizeof(Buffer*) * count);
            memset(values, 0, sizeof(T*) * count);
            count = 0;
        }
    };
}
