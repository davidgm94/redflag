#include "parser.h"
#include "lexer.h"
#include <vector>
#include <stdarg.h>
#include "os.h"

namespace RedAST
{
    struct ParseContext
    {
        Buffer* buffer;
        List<Token>* tokens;
        size_t current_token;
        //RedType* owner;
    };

    static void error(ParseContext* pc, Token* token, const char* format, ...)
    {
        fprintf(stdout, "error parsing token %s at line %zu column %zu: ", token_name(token->id), token->start_line + 1, token->start_column + 1);
        va_list args;
        va_start(args, format);
        vfprintf(stdout, format, args);
        va_end(args);
        exit(1);
    }

    static inline void invalid_token_error(ParseContext* pc, Token* token)
    {
        error(pc, token, "invalid token: '%s'", token_name(token->id));
    }

    static inline Token* get_token_i(ParseContext* pc, size_t i)
    {
        return &pc->tokens->at(pc->current_token + i);
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
        return nullptr;
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
        Buffer* wrong_symbol = wrong_token->id == TOKEN_ID_SYMBOL ? token_buffer(wrong_token) : nullptr;
        Buffer* good_symbol = good_token->id == TOKEN_ID_SYMBOL ? token_buffer(good_token) : nullptr;
        print("Current token #%zu: %s name: %s ******** Putting back token #%zu: %s name: %s\n", pc->current_token + 1, token_name(wrong_token->id), wrong_symbol ? wrong_symbol->items : "not a symbol", pc->current_token, token_name(good_token->id), good_symbol ? good_symbol->items : "not a symbol");
#endif
    }

    
    //static inline Buffer* token_type(ParseContext* pc, Token* token)
    //{
    //    redassert(token->id == TOKEN_ID_SYMBOL);
    //    return token_buffer(token);
    //}

    static inline Expression* parse_bool_expr(ParseContext* pc)
    {
        Token* token = consume_token_if(pc, TOKEN_ID_KEYWORD_TRUE);
        if (token)
        {
            return new BoolExpr(token, true);
        }
        token = consume_token_if(pc, TOKEN_ID_KEYWORD_FALSE);
        if (token)
        {
            return new BoolExpr(token, false);
        }
        return nullptr;
    }

    static inline Expression* parse_int_expr(ParseContext* pc)
    {
        Token* token = consume_token_if(pc, TOKEN_ID_INT_LIT);
        if (token)
        {
            auto intexpr = new IntExpr(token);
            return intexpr;
        }
        return nullptr;
    }

    static inline Expression* parse_float_expr(ParseContext* pc)
    {
        Token* token = consume_token_if(pc, TOKEN_ID_FLOAT_LIT);
        if (token)
        {
            auto floatexpr = new FloatExpr(token);
            return floatexpr;
        }
        return nullptr;
    }

    static inline Expression* parse_expression(ParseContext* pc);

    static inline Expression* parse_expression_parenthesis(ParseContext* pc)
    {
        Token* token = consume_token_if(pc, TOKEN_ID_LEFT_PARENTHESIS);
        if (!token)
        {
            return nullptr;
        }
        auto expr = parse_expression(pc);
        if (!expr)
        {
            put_back_token(pc);
            return nullptr;
        }

        expect_token(pc, TOKEN_ID_RIGHT_PARENTHESIS);
        return expr;
    }

    static inline Expression* parse_symbol(ParseContext* pc)
    {
        //Token* token = consume_token_if(pc, TOKEN_ID_SYMBOL);

        RED_NOT_IMPLEMENTED;
        return nullptr;
    }

    static inline Expression* parse_primary_expr(ParseContext* pc)
    {
        Expression* result = nullptr;
        Token* token = get_token(pc);
        switch (token->id)
        {
            case TOKEN_ID_SYMBOL:
                result = parse_symbol(pc);
                break;
            case TOKEN_ID_INT_LIT:
                result = parse_int_expr(pc);
                break;
            case TOKEN_ID_FLOAT_LIT:
                result = parse_float_expr(pc);
                break;
            case TOKEN_ID_LEFT_PARENTHESIS:
                result = parse_expression_parenthesis(pc);
                break;
            case TOKEN_ID_KEYWORD_FALSE:
            case TOKEN_ID_KEYWORD_TRUE:
                result = parse_bool_expr(pc);
                break;
            default:
                invalid_token_error(pc, token);
                break;
        }

        return result;
    }


    static inline Expression* parse_right_expr(ParseContext* pc, Expression** left_expr)
    {
        while (true)
        {
            Token* token = get_token(pc);
            if (token_is_binop(token->id))
            {
                consume_token(pc);
            }
            else
            {
                return *left_expr;
            }

            auto right_expr = parse_primary_expr(pc);
            if (!right_expr)
            {
                return nullptr;
            }

            *left_expr = new BinaryExpr(token, *left_expr, right_expr);
        }
    }

    static inline Expression* parse_expression(ParseContext* pc)
    {
        auto left_expr = parse_primary_expr(pc);
        if (!left_expr)
        {
            return nullptr;
        }

        return parse_right_expr(pc, &left_expr);
    }

    static inline VariableExpr* parse_arg_decl(ParseContext* pc)
    {
        Token* symbol_name = expect_token(pc, TOKEN_ID_SYMBOL);
        Token* symbol_type = expect_token(pc, TOKEN_ID_SYMBOL);

        auto elem_decl = new VariableExpr(symbol_name, symbol_type);
        return elem_decl;
    }

    static inline std::vector<VariableExpr*> parse_arg_list(ParseContext* pc, TokenID separator, TokenID end_of_list)
    {
        std::vector<VariableExpr*> elem_list = {};
        // TODO: empty parameter list
        while (get_token(pc)->id != end_of_list)
        {
            elem_list.emplace_back(parse_arg_decl(pc));
            if (get_token(pc)->id != end_of_list)
            {
                expect_token(pc, separator);
            }
        }
        expect_token(pc, end_of_list);
        return elem_list;
    }

    static inline ReturnExpr* parse_return_expr(ParseContext* pc);
    void print_statement(Expression* statement);
    static inline BranchExpr* parse_if_expr(ParseContext* pc);
    static inline Expression* parse_statement(ParseContext* pc)
    {
        // TODO: modify to amplify
        /*
        * return expr
        * if
        * var_decl
        * var_assign
        * fn_call
        * for
        * while
        * do while
        * switch
        */
        switch (get_token(pc)->id)
        {
            case TOKEN_ID_KEYWORD_IF:
                return parse_if_expr(pc);
            case TOKEN_ID_KEYWORD_RETURN:
                return parse_return_expr(pc);
            default:
                RED_NOT_IMPLEMENTED;
                return nullptr;
        }
    }
    
    static inline BlockExpr* parse_block_expr(ParseContext* pc)
    {
        std::vector<Expression*>* expr_list = new std::vector<Expression*>();
        Token* token = consume_token_if(pc, TOKEN_ID_LEFT_BRACE);
        if (!token)
        {
            return nullptr;
        }

        while (get_token(pc)->id != TOKEN_ID_RIGHT_BRACE)
        {
            Expression* expr = parse_statement(pc);
            if (expr == nullptr)
            {
                return nullptr;
            }
            expr_list->emplace_back(expr);
        }

        expect_token(pc, TOKEN_ID_RIGHT_BRACE);

        return new BlockExpr(token, expr_list);
    }

    static inline BranchExpr* parse_if_expr(ParseContext* pc)
    {
        Token* if_kw_token = consume_token_if(pc, TOKEN_ID_KEYWORD_IF);
        if (!if_kw_token)
        {
            return nullptr;
        }

        Expression* expression = parse_expression(pc);
        if (!expression)
        {
            return nullptr;
        }
        BoolExpr* bool_expr = static_cast<BoolExpr*>(expression);
        if (!bool_expr)
        {
            return nullptr;
        }

        BlockExpr* if_block = parse_block_expr(pc);
        if (!if_block)
        {
            return nullptr;
        }
        
        BlockExpr* else_block = nullptr;
        Token* else_token = consume_token_if(pc, TOKEN_ID_KEYWORD_ELSE);
        if (else_token)
        {
            else_block = parse_block_expr(pc);
            if (!else_block)
            {
                return nullptr;
            }
        }

        return new BranchExpr(if_kw_token, bool_expr, if_block, else_block);
    }

    static inline Prototype* parse_fn_prototype(ParseContext* pc)
    {
        Token* identifier = get_token(pc);
        if (identifier->id != TOKEN_ID_SYMBOL)
        {
            return nullptr;
        }

        Token* eq_sign = get_token_i(pc, 1);
        if (eq_sign->id != TOKEN_ID_EQ)
        {
            return nullptr;
        }

        Token* left_parenthesis = get_token_i(pc, 2);
        if (left_parenthesis->id != TOKEN_ID_LEFT_PARENTHESIS)
        {
            return nullptr;
        }
        // name
        consume_token(pc);
        // equal
        consume_token(pc);
        consume_token(pc);

        auto arg_list = parse_arg_list(pc, TOKEN_ID_COMMA, TOKEN_ID_RIGHT_PARENTHESIS);

        Buffer* return_typename = nullptr;
        Token* return_type = consume_token_if(pc, TOKEN_ID_SYMBOL);
        if (return_type)
        {
            return_typename = token_buffer(return_type);
        }
        else if (!(get_token(pc)->id == TOKEN_ID_SEMICOLON || get_token(pc)->id == TOKEN_ID_LEFT_BRACE))
        {
            invalid_token_error(pc, get_token(pc));
        }

        auto result = new Prototype(identifier, arg_list, return_type);
        return result;
    }
    static inline ReturnExpr* parse_return_expr(ParseContext* pc)
    {
        // Assume we parsed the return token
        Token* ret_token = consume_token(pc);
        Expression* ret_expr = parse_primary_expr(pc);
        expect_token(pc, TOKEN_ID_SEMICOLON);
        return new ReturnExpr(ret_token, ret_expr);
    }

    static inline Function* parse_fn_definition(ParseContext* pc)
    {
        Prototype* proto = parse_fn_prototype(pc);
        if (!proto)
        {
            return nullptr;
        }

        BlockExpr* body = parse_block_expr(pc);
        if (body)
        {
            return new Function(proto, body);
        }

        return nullptr;
    }


    // @unused: interpreted stuff, which i am not interested in right now
    //static inline Function* parse_top_level_expr()
    //static inline Prototype* parse_extern_proto(ParseContext* pc)
    //{
    //    Token* token = consume_token_if(pc, TOKEN_ID_KEYWORD_EXTERN);
    //    if (!token)
    //    {
    //        return nullptr;
    //    }

    //    return parse_fn_prototype(pc);
    //}

    /***
    * Top level parsing
    */
    static inline List<Function*> parse_internal(ParseContext* pc)
    {
        List<Function*> function_definitions = {};
        while (get_token(pc)->id != TOKEN_ID_END_OF_FILE)
        {
            Function* function = parse_fn_definition(pc);
            if (function)
            {
                function_definitions.append(function);
            }
            else
            {
                RED_UNREACHABLE;
            }
        }

        return function_definitions;
    }

    void print_statement(Expression* statement)
    {
        if (statement)
        {
            statement->print();
        }
        else
        {
            print("Statement is null\n");
        }
    }

    void FloatExpr::print()
    {
        RED_NOT_IMPLEMENTED;
    }

    void VariableExpr::print()
    {
        ::print("Variable declaration: Name: %s. Type: %s.\n", buf_ptr(name), buf_ptr(type));
    }

    void BranchExpr::print()
    {
        ::print("Branching over condition: ");
        print_statement(this->condition);
        ::print("If block\n");
        this->true_block->print();
        ::print("Else block\n");
        this->false_block->print();
    }

    void BoolExpr::print()
    {
        ::print("Bool value: %s\n", this->value ? "true" : "false");
    }

    void IntExpr::print()
    {
        // TODO: modify
        ::print("Int value: %d\n", this->bigint->digit);
    }

    void Prototype::print()
    {
        ::print("Printing function: %s\n", buf_ptr(this->name));
        for (auto& arg : args)
        {
            ::print("Parameter %s: %s\n", arg->name, arg->type);
        }
        ::print("Return type: %s\n", buf_ptr(this->return_type));
    }

    //void VariableExpr::print()
    //{
    //    ::print("Variable declaration. Name: %s. Type: %s\n", buf_ptr(name), buf_ptr(type));
    //}

    void ReturnExpr::print()
    {
        ::print("Return expr: ");
        print_statement(this->return_expr);
    }

    void BlockExpr::print()
    {
        ::print("{\n");
        for (auto& statement : *expressions_in_block)
        {
            print_statement(statement);
        }
        ::print("}\n");
    }

    void Function::print()
    {
        proto->print();
        body->print();
    }

    void BinaryExpr::print()
    {
        print_statement(this->left);
        ::print(" %c ", this->op);
        print_statement(this->right);
    }
}

List<RedAST::Function*> parse(Buffer* file_buffer, List<Token>* tokens)
{
    ScopeTimer parser_time("Parse");
    RedAST::ParseContext pc = {};
    pc.buffer = file_buffer;
    pc.tokens = tokens;
    List<RedAST::Function*> function_list = RedAST::parse_internal(&pc);
    return function_list;
}

void parser_print_ast(List<RedAST::Function*>* fn_list)
{
    print("Printing Abstract Syntax Tree\n=======================\n");
    for (u32 i = 0; i < fn_list->length; i++)
    {
        RedAST::Function* fn = fn_list->at(i);
        fn->print();
    }
}
