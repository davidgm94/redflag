#pragma once

typedef enum AST_ID
{
    AST_TYPE_FN_DEF,
    AST_TYPE_FN_PROTO,
    AST_TYPE_TYPE_EXPR,
    AST_TYPE_PARAM_DECL,
    AST_TYPE_SIZE_EXPR,
    AST_TYPE_COMPOUND_STATEMENT,
    AST_TYPE_RETURN_STATEMENT,
    AST_TYPE_SWITCH_STATEMENT,
    AST_TYPE_SWITCH_CASE,
    AST_TYPE_SYM_DECL,
    AST_TYPE_SYM_EXPR,
    AST_TYPE_BIN_EXPR,
    AST_TYPE_FN_CALL,
    AST_TYPE_BRANCH_EXPR,
    AST_TYPE_LOOP_EXPR,
    AST_TYPE_INT_LIT,
    AST_TYPE_STRING_LIT,
    AST_TYPE_ARRAY_LIT,
    AST_TYPE_FIELD_DECL,
    AST_TYPE_ENUM_FIELD,
    AST_TYPE_STRUCT_DECL,
    AST_TYPE_UNION_DECL,
    AST_TYPE_ENUM_DECL,
} AST_ID;

GEN_BUFFER_FUNCTIONS(node, nb, ASTNodeBuffer, struct ASTNode*)


typedef enum ASTSymbolSubscriptType
{
    AST_SYMBOL_SUBSCRIPT_TYPE_ARRAY_ACCESS,
    AST_SYMBOL_SUBSCRIPT_TYPE_FIELD_ACCESS,
} ASTSymbolSubscriptType, IRSymbolSubscriptType;

typedef struct ASTSymbol
{
    SB* name;
    struct
    {
        ASTNode* subscript;
        ASTSymbolSubscriptType subscript_type;
    };
} ASTSymbol;

typedef struct ASTStringLit
{
    SB* str_lit;
} ASTStringLit;

typedef struct ASTArrayType
{
    ASTNode* type;
    ASTNode* element_count_expr;
} ASTArrayType;

typedef struct ASTStructType
{
    ASTNodeBuffer fields;
} ASTUnionType, ASTStructType; 

typedef struct ASTStructDecl
{
    SB name;
    ASTNodeBuffer fields;
} ASTStructDecl, ASTUnionDecl;

typedef struct ASTEnumField
{
    SB* name;
    ASTNode* field_value;
} ASTEnumField;

typedef enum ASTEnumType
{
    ENUM_TYPE_U8,
    ENUM_TYPE_U16,
    ENUM_TYPE_U32,
    ENUM_TYPE_U64,
    ENUM_TYPE_S8,
    ENUM_TYPE_S16,
    ENUM_TYPE_S32,
    ENUM_TYPE_S64,
} ASTEnumType, IREnumType;

typedef struct ASTEnumDecl
{
    SB* name;
    ASTNodeBuffer fields;
    ASTEnumType type;
} ASTEnumDecl;

typedef struct ASTPointerType
{
    ASTNode* type;
} ASTPointerType;

typedef struct ASTType
{
    TypeKind kind;
    SB* name;
    union
    {
        ASTArrayType array;
        ASTStructType struct_;
        ASTUnionType union_;
        // probably buggy enum decl: subst for enum type
        ASTEnumDecl enum_;
        ASTPointerType pointer_;
    };
} ASTType;

typedef struct ASTSizeExpr
{
    ASTNode* expr;
} ASTSizeExpr;

typedef struct ASTParamDecl
{
    ASTNode* sym;
    ASTNode* type;
} ASTParamDecl, ASTFieldDecl;

typedef struct ASTSymDecl
{
    ASTNode* sym;
    ASTNode* type;
    ASTNode* value;
    bool is_const;
} ASTSymDecl;

typedef struct ASTIntLit
{
    BigInt* bigint;
} ASTIntLit;

typedef struct ASTArrayLit
{
    ASTNodeBuffer values;
} ASTArrayLit;

typedef struct ASTBinExpr
{
    TokenID op;
    ASTNode* left;
    ASTNode* right;
} ASTBinExpr;

typedef struct ASTRetExpr
{
    ASTNode* expr;
} ASTRetExpr;

typedef struct ASTCompoundStatement
{
    ASTNodeBuffer statements;
} ASTCompoundStatement;

typedef struct ASTBranchExpr
{
    ASTNode* condition;
    ASTNode* if_block;
    ASTNode* else_block;
} ASTBranchExpr;

typedef struct ASTSwitchCase
{
    ASTNode* case_value;
    ASTNode* case_body;
} ASTSwitchCase;

typedef struct ASTSwitchExpr
{
    ASTNode* expr_to_switch_on;
    ASTNodeBuffer cases;
} ASTSwitchExpr;

typedef struct ASTLoopExpr
{
    ASTNode* condition;
    ASTNode* body;
} ASTLoopExpr;

typedef struct ASTFnCallExpr
{
    SB name;
    ASTNode** args;
    u8 arg_count;
} ASTFnCallExpr;

typedef struct ASTFnProto
{
    ASTNodeBuffer params;
    ASTNode* sym;
    ASTNode* ret_type;
} ASTFnProto;

typedef struct ASTFnDef
{
    ASTNode* proto;
    ASTNode* body;
} ASTFnDef;

typedef struct ASTNode
{
    AST_ID node_id;
    u32 node_line;
    u32 node_column;
    u32 node_padding;

    union
    {
        ASTSymbol sym_expr;
        ASTType type_expr;
        ASTParamDecl param_decl;
        ASTFieldDecl field_decl;
        ASTStructDecl struct_decl;
        ASTUnionDecl union_decl;
        ASTEnumField enum_field;
        ASTEnumDecl enum_decl;
        ASTSymDecl sym_decl;
        ASTSizeExpr size_expr;
        ASTIntLit int_lit;
        ASTStringLit string_lit;
        ASTArrayLit array_lit;
        ASTBinExpr bin_expr;
        ASTRetExpr return_expr;
        ASTCompoundStatement compound_statement;
        ASTBranchExpr branch_expr;
        ASTSwitchCase switch_case;
        ASTSwitchExpr switch_expr;
        ASTLoopExpr loop_expr;
        ASTFnCallExpr fn_call;
        ASTFnProto fn_proto;
        ASTFnDef fn_def;
    };
} ASTNode;

RedAST parse_translation_unit(StringBuffer* src_buffer, TokenBuffer* tb, const char* module_name);
