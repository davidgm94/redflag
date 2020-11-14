#pragma once


typedef struct IRParamDecl
{
    RedType type;
    SB* name;
} IRParamDecl;


typedef struct IRConstValue
{
    RedTypePrimitive type;
    union
    {
        BigInt int_lit;
        BigFloat float_lit;
        SB* str_lit;
        char char_lit;
    };
} IRConstValue;

typedef enum LinkageType
{
    EXTERN = 0,
    INTERN,
} LinkageType;

typedef struct IRFunctionConfig
{
    LinkageType link_type;
} IRFunctionConfig;

typedef struct IRFunctionPrototype
{
    IRParamDecl* params;
    RedType ret_type;
    SB* name;
    IRFunctionConfig fn_cfg;
    u8 param_count;
} IRFunctionPrototype;

typedef struct IRStatement IRStatement;
GEN_BUFFER_STRUCT(IRStatement)
typedef struct IRCompoundStatement
{
    IRStatementBuffer stmts;
} IRCompoundStatement;

typedef enum IRStatementType
{
    IR_ST_TYPE_COMPOUND_ST,
    IR_ST_TYPE_RETURN_ST,
    IR_ST_TYPE_BRANCH_ST,
    IR_ST_TYPE_SYM_DECL_ST,
} IRStatementType;

typedef enum IRExpressionType
{
    IR_EXPRESSION_TYPE_VOID = 0,
    IR_EXPRESSION_TYPE_INT_LIT,
    IR_EXPRESSION_TYPE_SYM_EXPR,
    IR_EXPRESSION_TYPE_BIN_EXPR,
} IRExpressionType;

typedef struct IRIntLiteral
{
    BigInt bigint;
} IRIntLiteral;

typedef enum IRSymExprType
{
    IR_SYM_EXPR_TYPE_SYM,
    IR_SYM_EXPR_TYPE_PARAM,
} IRSymExprType;

typedef struct IRSymDeclStatement IRSymDeclStatement;
typedef struct IRSymExpr
{
    IRSymExprType type;
    union
    {
        IRSymDeclStatement* sym_decl;
        IRParamDecl* param_decl;
    };
} IRSymExpr;

typedef struct IRExpression IRExpression;

typedef struct IRBinaryExpr
{
    IRExpression* left;
    IRExpression* right;
    TokenID op;
} IRBinaryExpr;

typedef struct IRExpression
{
    IRExpressionType type;
    union
    {
        IRIntLiteral int_literal;
        IRSymExpr sym_expr;
        IRBinaryExpr bin_expr;
    };
} IRExpression;

typedef struct IRReturnStatement
{
    IRExpression expression;
    RedType red_type;
} IRReturnStatement;

typedef struct IRBranchStatement
{
    IRExpression condition;
    IRCompoundStatement if_block;
    IRCompoundStatement else_block;
} IRBranchStatement;

typedef struct IRSymDeclStatement
{
    RedType type;
    SB* name;
    IRExpression value;
    bool is_const;
} IRSymDeclStatement;

typedef struct IRStatement
{
    IRStatementType type;
    union
    {
        IRCompoundStatement compound_st;
        IRReturnStatement return_st;
        IRBranchStatement branch_st;
        IRSymDeclStatement sym_decl_st;
    };
} IRStatement;

GEN_BUFFER_STRUCT_PTR(IRSymDeclStatement, IRSymDeclStatement*)
typedef struct IRFunctionDefinition
{
    IRFunctionPrototype proto;
    IRCompoundStatement body;
    IRSymDeclStatementBuffer sym_declarations;
} IRFunctionDefinition;

GEN_BUFFER_STRUCT(IRFunctionDefinition)

typedef struct RedModuleIR
{
    IRFunctionDefinitionBuffer fn_definitions;
} RedModuleIR;

RedModuleIR transform_ast_to_ir(RedModuleTree* ast);


