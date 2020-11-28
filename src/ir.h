#pragma once

#include "parser.h"
typedef struct IRExpression IRExpression;
typedef struct IRType IRType;
typedef struct IRStructDecl IRStructDecl;
typedef struct IRFunctionPrototype IRFunctionPrototype;
typedef struct IREnumDecl IREnumDecl;
typedef struct IRStatement IRStatement;
typedef struct IRSymDeclStatement IRSymDeclStatement;
typedef struct IRExpression IRExpression;
GEN_BUFFER_STRUCT(IRStatement)
GEN_BUFFER_STRUCT(IRSymDeclStatement)

typedef enum IRTypePrimitive
{
    IR_TYPE_PRIMITIVE_U8,
    IR_TYPE_PRIMITIVE_U16,
    IR_TYPE_PRIMITIVE_U32,
    IR_TYPE_PRIMITIVE_U64,
    IR_TYPE_PRIMITIVE_S8,
    IR_TYPE_PRIMITIVE_S16,
    IR_TYPE_PRIMITIVE_S32,
    IR_TYPE_PRIMITIVE_S64,
    IR_TYPE_PRIMITIVE_F32,
    IR_TYPE_PRIMITIVE_F64,
    IR_TYPE_PRIMITIVE_F128,
    IR_TYPE_PRIMITIVE_BOOL,
    IR_TYPE_PRIMITIVE_COUNT,
    IR_TYPE_PRIMITIVE_POINTER,
} IRTypePrimitive;

static_assert(IR_TYPE_PRIMITIVE_COUNT == 12, "Primitive type enum has been modified! Correct it");

static inline const char* primitive_type_str(IRTypePrimitive primitive_type_id)
{
    switch (primitive_type_id)
    {
        CASE_TO_STR(IR_TYPE_PRIMITIVE_U8);
        CASE_TO_STR(IR_TYPE_PRIMITIVE_U16);
        CASE_TO_STR(IR_TYPE_PRIMITIVE_U32);
        CASE_TO_STR(IR_TYPE_PRIMITIVE_U64);
        CASE_TO_STR(IR_TYPE_PRIMITIVE_S8);
        CASE_TO_STR(IR_TYPE_PRIMITIVE_S16);
        CASE_TO_STR(IR_TYPE_PRIMITIVE_S32);
        CASE_TO_STR(IR_TYPE_PRIMITIVE_S64);
        CASE_TO_STR(IR_TYPE_PRIMITIVE_F32);
        CASE_TO_STR(IR_TYPE_PRIMITIVE_F64);
        CASE_TO_STR(IR_TYPE_PRIMITIVE_F128);
        CASE_TO_STR(IR_TYPE_PRIMITIVE_POINTER);
        default:
            RED_NOT_IMPLEMENTED;
            return null;
    }
}

typedef struct IRArrayType
{
    IRType* base_type;
    IRExpression* elem_count_expr;
} IRArrayType;

typedef struct IRPointerType
{
    IRType* base_type;
} IRPointerType;

typedef struct IRType
{
    TypeKind kind;
    usize size;

    union
    {
        IRTypePrimitive primitive_type;
        IRStructDecl* struct_type;
        IREnumDecl* enum_type;
        IRFunctionPrototype* fn_type;
        IRArrayType array_type;
        IRPointerType pointer_type;
    };
} IRType;

typedef enum IRLoadStoreCfg
{
    LOAD,
    STORE,
} IRLoadStoreCfg;

typedef struct IRParamDecl
{
    IRType type;
    SB* name;
} IRParamDecl;

typedef struct IRFieldDecl
{
    IRType type;
    SB* name;
} IRFieldDecl;

typedef struct IREnumField
{
    SB* name;
    union
    {
        s64 signed64;
        u64 unsigned64;
        s32 signed32;
        u32 unsigned32;
        s16 signed16;
        u16 unsigned16;
        s8 signed8;
        u8 unsigned8;
    } value;
    IREnumDecl* parent;
} IREnumField;

GEN_BUFFER_STRUCT(IREnumField)

typedef struct IREnumDecl
{
    IRType type;
    IREnumFieldBuffer fields;
    SB* name;
} IREnumDecl;

typedef struct IRUnionDecl
{
    u64 foo;
} IRUnionDecl;

GEN_BUFFER_STRUCT(IRUnionDecl)

typedef struct IRStructDecl
{
    SB name;
    IRFieldDecl* fields;
    u32 field_count;
} IRStructDecl;

typedef struct IRConstValue
{
    IRTypePrimitive type;
    union
    {
        BigInt int_lit;
        BigFloat float_lit;
        SB* str_lit;
        char char_lit;
    };
} IRConstValue;

typedef enum IRLinkageType
{
    EXTERN = 0,
    INTERN,
} IRLinkageType;

typedef struct IRFunctionConfig
{
    IRLinkageType link_type;
} IRFunctionConfig;

typedef struct IRFunctionPrototype
{
    IRParamDecl* params;
    SB* name;
    // TODO: remove
    IRType ret_type;
    IRFunctionConfig fn_cfg;
    u8 param_count;
} IRFunctionPrototype;
GEN_BUFFER_STRUCT(IRFunctionPrototype)

typedef struct IRCompoundStatement
{
    IRStatementBuffer stmts;
} IRCompoundStatement;

typedef enum IRStatementType
{
    IR_ST_TYPE_COMPOUND_ST,
    IR_ST_TYPE_RETURN_ST,
    IR_ST_TYPE_BRANCH_ST,
    IR_ST_TYPE_SWITCH_ST,
    IR_ST_TYPE_SYM_DECL_ST,
    IR_ST_TYPE_ASSIGN_ST,
    IR_ST_TYPE_FN_CALL_ST,
    IR_ST_TYPE_LOOP_ST,
} IRStatementType;

typedef enum IRExpressionType
{
    IR_EXPRESSION_TYPE_VOID = 0,
    IR_EXPRESSION_TYPE_INT_LIT,
    IR_EXPRESSION_TYPE_ARRAY_LIT,
    IR_EXPRESSION_TYPE_STRING_LIT,
    IR_EXPRESSION_TYPE_SYM_EXPR,
    IR_EXPRESSION_TYPE_BIN_EXPR,
    IR_EXPRESSION_TYPE_FN_CALL_EXPR,
    IR_EXPRESSION_TYPE_SUBSCRIPT_ACCESS,
} IRExpressionType;

typedef struct IRIntLiteral
{
    BigInt bigint;
    IRTypePrimitive type;
} IRIntLiteral;

typedef struct IRArrayLiteral
{
    IRExpression* expressions;
    u64 expression_count;
} IRArrayLiteral;

typedef struct IRStringLiteral
{
    SB* str_lit;
} IRStringLiteral;

typedef enum IRSymExprType
{
    IR_SYM_EXPR_TYPE_SYM,
    IR_SYM_EXPR_TYPE_PARAM,
    IR_SYM_EXPR_TYPE_GLOBAL_SYM,
    IR_SYM_EXPR_TYPE_ENUM,
    IR_SYM_EXPR_TYPE_ENUM_FIELD,
    IR_SYM_EXPR_TYPE_STRUCT,
    IR_SYM_EXPR_TYPE_STRUCT_FIELD,
} IRSymExprType;

typedef struct IRSymExpr
{
    union
    {
        IRSymDeclStatement* sym_decl;
        IRSymDeclStatement* global_sym_decl;
        IRParamDecl* param_decl;
        IREnumField* enum_field;
        IREnumDecl* enum_decl;
        IRStructDecl* struct_decl;
    };

    IRExpression* subscript;
    IRSymExprType type;
    IRLoadStoreCfg use_type;
} IRSymExpr;

typedef struct IRBinaryExpr
{
    IRExpression* left;
    IRExpression* right;
    TokenID op;
} IRBinaryExpr;

typedef struct IRFunctionCallExpr
{
    // TODO: change for fn prototype pointer
    IRFunctionPrototype* fn;
    IRExpression* args;
    u8 arg_count;
} IRFunctionCallExpr, IRFunctionCallStatement;

typedef struct IRSubscriptAccess
{
    SB* name;
    struct
    {
        union
        {
            IRStructDecl* struct_p;
            IREnumDecl* enum_p;
        };
        TypeKind type;
    } parent;
    IRExpression* subscript;
    IRSymbolSubscriptType subscript_type;
} IRSubscriptAccess;

typedef struct IRExpression
{
    IRExpressionType type;
    union
    {
        IRIntLiteral int_literal;
        IRArrayLiteral array_literal;
        IRStringLiteral string_literal;
        IRSymExpr sym_expr;
        IRBinaryExpr bin_expr;
        IRFunctionCallExpr fn_call_expr;
        IRSubscriptAccess subscript_access;
    };
} IRExpression;

typedef struct IRReturnStatement
{
    IRExpression expression;
    IRType red_type;
} IRReturnStatement;

typedef struct IRBranchStatement
{
    IRExpression condition;
    IRCompoundStatement if_block;
    IRStatement* else_block;
} IRBranchStatement;

typedef struct IRSwitchCase
{
    IRExpression case_expr;
    IRCompoundStatement case_body;
} IRSwitchCase;

GEN_BUFFER_STRUCT(IRSwitchCase)

typedef struct IRSwitchStatement
{
    IRExpression switch_expr;
    IRSwitchCaseBuffer cases;
} IRSwitchStatement;

typedef struct IRSymDeclStatement
{
    IRType type;
    SB* name;
    IRExpression value;
    bool is_const;
} IRSymDeclStatement;

typedef struct IRSymAssignStatement
{
    IRExpression* left;
    IRExpression* right;
} IRSymAssignStatement;

typedef struct IRLoopStatement
{
    IRExpression condition;
    IRCompoundStatement body;
} IRLoopStatement;

typedef struct IRStatement
{
    IRStatementType type;
    union
    {
        IRCompoundStatement compound_st;
        IRReturnStatement return_st;
        IRBranchStatement branch_st;
        IRSwitchStatement switch_st;
        IRSymDeclStatement sym_decl_st;
        IRSymAssignStatement sym_assign_st;
        IRFunctionCallStatement fn_call_st;
        IRLoopStatement loop_st;
    };
} IRStatement;

typedef struct IRFunctionDefinition
{
    IRFunctionPrototype* proto;
    IRCompoundStatement body;
    IRSymDeclStatementBuffer sym_declarations;
} IRFunctionDefinition;

GEN_BUFFER_STRUCT(IRStructDecl)
GEN_BUFFER_STRUCT(IRFunctionDefinition)
GEN_BUFFER_STRUCT(IREnumDecl)

typedef struct IRModule
{
    IRStructDeclBuffer struct_decls;
    IREnumDeclBuffer enum_decls;
    IRUnionDeclBuffer union_decls;
    IRSymDeclStatementBuffer global_sym_decls;
   
    IRFunctionPrototypeBuffer fn_prototypes;
    IRFunctionDefinitionBuffer fn_definitions;
} IRModule;

IRModule transform_ast_to_ir(RedAST* ast);

IRType ast_to_ir_find_expression_type(IRExpression* expression);
