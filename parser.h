//
// Created by david on 10/11/20.
//
#pragma once
#include "types.h"
#include "buffer.h"
#include "lexer.h"

enum ErrorColor
{
    ERROR_COLOR_AUTO,
    ERROR_COLOR_ON,
    ERROR_COLOR_OFF,
};

struct RedType;
struct Scope;
struct ASTNode;

enum NodeType {
    NODE_TYPE_FN_PROTO,
    NODE_TYPE_FN_DEF,
    NODE_TYPE_PARAM_DECL,
    NODE_TYPE_BLOCK,
    NODE_TYPE_GROUPED_EXPR,
    NODE_TYPE_RETURN_EXPR,
    NODE_TYPE_DEFER,
    NODE_TYPE_VARIABLE_DECLARATION,
    NODE_TYPE_TEST_DECL,
    NODE_TYPE_BIN_OP_EXPR,
    NODE_TYPE_CATCH_EXPR,
    NODE_TYPE_FLOAT_LITERAL,
    NODE_TYPE_INT_LITERAL,
    NODE_TYPE_STRING_LITERAL,
    NODE_TYPE_CHAR_LITERAL,
    NODE_TYPE_SYMBOL,
    NODE_TYPE_PREFIX_OP_EXPR,
    NODE_TYPE_POINTER_TYPE,
    NODE_TYPE_FN_CALL_EXPR,
    NODE_TYPE_ARRAY_ACCESS_EXPR,
    NODE_TYPE_SLICE_EXPR,
    NODE_TYPE_FIELD_ACCESS_EXPR,
    NODE_TYPE_PTR_DEREF,
    NODE_TYPE_UNWRAP_OPTIONAL,
    NODE_TYPE_USING_NAMESPACE,
    NODE_TYPE_BOOL_LITERAL,
    NODE_TYPE_NULL_LITERAL,
    NODE_TYPE_UNDEFINED_LITERAL,
    NODE_TYPE_UNREACHABLE,
    NODE_TYPE_IF_BOOL_EXPR,
    NODE_TYPE_WHILE_EXPR,
    NODE_TYPE_FOR_EXPR,
    NODE_TYPE_SWITCH_EXPR,
    NODE_TYPE_SWITCH_PRONG,
    NODE_TYPE_SWITCH_RANGE,
    NODE_TYPE_COMP_TIME,
    NODE_TYPE_NO_SUSPEND,
    NODE_TYPE_BREAK,
    NODE_TYPE_CONTINUE,
    NODE_TYPE_ASM_EXPR,
    NODE_TYPE_CONTAINER_DECL,
    NODE_TYPE_STRUCT_FIELD,
    NODE_TYPE_CONTAINER_INIT_EXPR,
    NODE_TYPE_STRUCT_VALUE_FIELD,
    NODE_TYPE_ARRAY_TYPE,
    NODE_TYPE_INFERRED_ARRAYTYPE,
    NODE_TYPE_ERROR_TYPE,
    NODE_TYPE_IF_ERROR_EXPR,
    NODE_TYPE_IF_OPTIONAL,
    NODE_TYPE_ERROR_SET_DECL,
    NODE_TYPE_ERROR_SET_FIELD,
    NODE_TYPE_RESUME,
    NODE_TYPE_AWAIT_EXPR,
    NODE_TYPE_SUSPEND,
    NODE_TYPE_ANY_FRAME_TYPE,
    NODE_TYPE_ENUM_LITERAL,
    NODE_TYPE_ANY_TYPE_FIELD,
};

enum FunctionInline
{
    FUNCTION_INLINE_AUTO,
    FUNCTION_INLINE_ALWAYS,
    FUNCTION_INLINE_NEVER
};

enum Visibility
{
    VISIBILITY_PUB,
    VISIBILITY_PRIVATE,
};

struct ASTNodeFunctionPrototype
{
    Buffer* name;
    List<ASTNode*> parameters;
    ASTNode* return_type;
    Token* return_any_type_token;
    ASTNode* function_definition_node;
    Buffer* library_name;
    ASTNode* align_expression;
    ASTNode* section_expression;
    ASTNode* call_convention_expression;
    Buffer documentation_comments;
    FunctionInline function_inline;

    Visibility visibility;
    bool auto_err_set;
    bool is_var_args;
    bool is_extern;
    bool is_export;
};

struct ASTNodeFunctionDefinition
{
    ASTNode* function_prototype;
    ASTNode* body;
};

struct ASTNodeParameterDeclaration
{
    Buffer* name;
    ASTNode* type;
    Token* any_type_token;
    Buffer document_comments;
    bool is_noalias;
    bool is_compile_time;
    bool is_var_args;
};

struct ASTNodeBlock
{
    Buffer* name;
    List<ASTNode*> statements;
};

enum ReturnType
{
    RETURN_TYPE_UNCONDITIONAL,
    RETURN_TYPE_ERROR,
};

struct ASTNodeReturnExpression
{
    ReturnType type;
    ASTNode* expression;
};

struct ASTNodeDefer
{
    ReturnType type;
    ASTNode* error_payload;
    ASTNode* expression;

    Scope* child_scope;
    Scope* expression_scope;
};

struct ASTNodeVariableDeclaration
{
    Buffer* symbol;

    ASTNode* type;
    ASTNode* expression;

    Buffer* library_name;

    ASTNode* align_expression;
    ASTNode* section_expression;

    Token* threadlocal_token;
    Buffer document_comments;

    Visibility visibility;
    bool is_const;
    bool is_compile_time;
    bool is_export;
    bool is_extern;
};

struct ASTNodeTestDeclaration
{
    Buffer* name;
    ASTNode* body;
};

enum BinaryOpType
{
    BIN_OP_TYPE_INVALID,
    BIN_OP_TYPE_ASSIGN,
    BIN_OP_TYPE_ASSIGN_TIMES,
    BIN_OP_TYPE_ASSIGN_TIMES_WRAP,
    BIN_OP_TYPE_ASSIGN_DIV,
    BIN_OP_TYPE_ASSIGN_MOD,
    BIN_OP_TYPE_ASSIGN_PLUS,
    BIN_OP_TYPE_ASSIGN_PLUS_WRAP,
    BIN_OP_TYPE_ASSIGN_MINUS,
    BIN_OP_TYPE_ASSIGN_MINUS_WRAP,
    BIN_OP_TYPE_ASSIGN_BIT_SHIFT_LEFT,
    BIN_OP_TYPE_ASSIGN_BIT_SHIFT_RIGHT,
    BIN_OP_TYPE_ASSIGN_BIT_AND,
    BIN_OP_TYPE_ASSIGN_BIT_XOR,
    BIN_OP_TYPE_ASSIGN_BIT_OR,
    BIN_OP_TYPE_ASSIGN_MERGE_ERROR_SETS,
    BIN_OP_TYPE_BOOL_OR,
    BIN_OP_TYPE_BOOL_AND,
    BIN_OP_TYPE_CMP_EQ,
    BIN_OP_TYPE_CMP_NOT_EQ,
    BIN_OP_TYPE_CMP_LESS_THAN,
    BIN_OP_TYPE_CMP_GREATER_THAN,
    BIN_OP_TYPE_CMP_LESS_OR_EQ,
    BIN_OP_TYPE_CMP_GREATER_OR_EQ,
    BIN_OP_TYPE_BIN_OR,
    BIN_OP_TYPE_BIN_XOR,
    BIN_OP_TYPE_BIN_AND,
    BIN_OP_TYPE_BIT_SHIFT_LEFT,
    BIN_OP_TYPE_BIT_SHIFT_RIGHT,
    BIN_OP_TYPE_ADD,
    BIN_OP_TYPE_ADD_WRAP,
    BIN_OP_TYPE_SUB,
    BIN_OP_TYPE_SUB_WRAP,
    BIN_OP_TYPE_MULT,
    BIN_OP_TYPE_MULT_WRAP,
    BIN_OP_TYPE_DIV,
    BIN_OP_TYPE_MOD,
    BIN_OP_TYPE_UNWRAP_OPTIONAL,
    BIN_OP_TYPE_ARRAY_CAT,
    BIN_OP_TYPE_ARRAY_MULT,
    BIN_OP_TYPE_ERROR_UNION,
    BIN_OP_TYPE_MERGE_ERROR_SETS,
};

struct ASTNodeBinaryOpExpression
{
    ASTNode* op1;
    ASTNode* op2;
    BinaryOpType op;
};

enum CallModifier
{
    CALL_MODIFIER_NONE,
    CALL_MODIFIER_ASYNC,
    CALL_MODIFIER_NEVER_TAIL,
    CALL_MODIFIER_NEVER_INLINE,
    CALL_MODIFIER_NO_SUSPEND,
    CALL_MODIFIER_ALWAYS_TAIL,
    CALL_MODIFIER_ALWAYS_INLINE,
    CALL_MODIFIER_COMPILE_TIME,

    // These are additional tags in the compiler, but not exposed in the std lib.
    CALL_MODIFIER_BUILTIN,
};

struct ASTNodeFunctionCallExpression
{
    ASTNode* function;
    List<ASTNode*> parameters;
    CallModifier modifier;
    bool seen;
};

struct ASTNodeArrayAccessExpression
{
    ASTNode* array;
    ASTNode* subscript;
};

struct ASTNodeSliceExpression
{
    ASTNode* array;
    ASTNode* start;
    ASTNode* end;
    ASTNode* sentinel;
};

struct ASTNodeFieldAccessExpression
{
    ASTNode* struct_ref;
    Buffer* field_name;
};

struct ASTNodePtrDereferenceExpression
{
    ASTNode* target;
};

enum PrefixOp
{
    Prefix_Op_Invalid,
    Prefix_Op_Bool_Not,
    Prefix_Op_Bin_Not,
    Prefix_Op_Negation,
    Prefix_Op_Negation_Wrap,
    Prefix_Op_Optional,
    Prefix_Op_Addr_Of,
};

struct ASTNodePrefixOpExpression
{
    PrefixOp op;
    ASTNode* expression;
};

// todo:
struct ASTNodePointerType
{
    Token* star_token;
    ASTNode* sentinel;
    ASTNode* align;
    BigInt* bit_offset_start;
    BigInt* host_int_bytes;
    ASTNode* op_expr;
    Token* allow_zero_token;
    bool is_const;
    bool is_volatile;
};

struct ASTNodeInferredArrayType
{
    ASTNode* sentinel;
    ASTNode* child_type;
};

struct ASTNodeArrayType
{
    ASTNode* size;
    ASTNode* sentinel;
    ASTNode* child_type;
    ASTNode* align_expression;
    Token* allow_zero_token;
    bool is_const;
    bool is_volatile;
};

struct ASTNodeUsingNamespace
{
    Visibility visibility;
    ASTNode* expression;
};

struct ASTNodeIfBoolExpression
{
    ASTNode* condition;
    ASTNode* true_block;
    ASTNode* false_block;
};

struct ASTNodeTestExpression
{
    Buffer* var_symbol;
    ASTNode* target_node;
    ASTNode* true_node;
    ASTNode* false_node;
    bool var_is_ptr;
};

struct ASTNodeWhileExpression
{
    Buffer* name;
    ASTNode* condition;
    Buffer* var_symbol;
    ASTNode* continue_expression;
    ASTNode* body;
    ASTNode* else_node;
    Buffer* error_symbol;
    bool var_is_ptr;
    bool is_inline;
};

struct ASTNodeForExpression
{
    Buffer* name;
    ASTNode* array_expression;
    ASTNode* element_node;
    ASTNode* index_node;
    ASTNode* body;
    ASTNode* else_node;
    bool element_is_ptr;
    bool is_inline;
};

struct ASTNodeSwitchExpression
{
    ASTNode* expression;
    List<ASTNode*> cases;
};

struct ASTNodeSwitchCase
{
    List<ASTNode*> items;
    ASTNode* var_symbol;
    ASTNode* expression;
    bool var_is_ptr;
    bool any_items_are_range;
};
struct ASTNodeSwitchRange
{
    ASTNode* start;
    ASTNode* end;
};

struct ASTNodeCompileTime
{
    ASTNode* expression;
};

struct ASTNodeNoSuspend
{
    ASTNode* expression;
};

struct ASMOutput
{
    Buffer* asm_symbolic_name;
    Buffer* constraint;
    Buffer* variable_name;
    ASTNode* return_type;
};

struct ASMInput
{
    Buffer* asm_symbolic_name;
    Buffer* constraint;
    ASTNode* expression;
};

struct SourcePosition
{
    size_t line;
    size_t column;
};

enum ASMTokenID
{
    ASM_TOKEN_ID_TEMPLATE,
    ASM_TOKEN_ID_PERCENT,
    ASM_TOKEN_ID_VAR,
    ASM_TOKEN_ID_UNIQUE_ID,
};

struct ASMToken
{
    ASMTokenID id;
    size_t start;
    size_t end;
};

struct ASTNodeASMExpression
{
    Token* volatile_token;
    ASTNode* asm_template;
    List<ASMOutput*> output_list;
    List<ASMInput*> input_list;
    List<Buffer*> clobber_list;
};

enum ContainerType
{
    CONTAINER_TYPE_STRUCT,
    CONTAINER_TYPE_ENUM,
    CONTAINER_TYPE_UNION,
    CONTAINER_TYPE_OPAQUE, // ?
};

enum ContainerLayout
{
    CONTAINER_LAYOUT_AUTO,
    CONTAINER_LAYOUT_EXTERN,
    CONTAINER_LAYOUT_PACKED,
};

struct ASTNodeContainerDeclaration
{
    ASTNode* init_argument_expression; /* enum, struct, ... */
    List<ASTNode*> fields;
    List<ASTNode*> declarations;
    Buffer document_comments;

    ContainerType type;
    ContainerLayout layout;
    bool auto_enum;
    bool is_root;
};

struct ASTNodeErrorSetField
{
    Buffer document_comments;
    ASTNode* field_name;
};

struct ASTNodeErrorSetDeclaration
{
    List<ASTNode*> declarations;
};

struct ASTNodeStructField
{
    Buffer* name;
    ASTNode* type;
    ASTNode* value;
    ASTNode* align_expression;
    Buffer document_comments;
    Token* compile_time_token;
};

struct ASTNodeStringLiteral
{
    Buffer* buffer;
};

struct ASTNodeCharLiteral
{
    char value;
};

struct ASTNodeFloatLiteral
{
    BigFloat* big_float;
    bool overflow;
};

struct ASTNodeIntLiteral
{
    BigInt* big_int;
};

struct ASTNodeStructValueField
{
    Buffer* name;
    ASTNode* expression;
};

enum ContainerInitializationType
{
    CONTAINER_INITIALIZATION_TYPE_STRUCT,
    CONTAINER_INITIALIZATION_TYPE_ARRAY,
};

struct ASTNodeContainerInitializationExpression
{
    ASTNode* type;
    List<ASTNode*> entries;
    ContainerInitializationType init_type;
};

struct ASTNodeNullLiteral
{};

struct ASTNodeUndefinedLiteral
{ };

struct ASTNodeThisLiteral
{ };

struct ASTNodeSymbolExpression
{
    Buffer* symbol;
};

struct ASTNodeBoolLiteral
{
    bool value;
};

struct ASTNodeBreakExpression
{
    Buffer* name;
    ASTNode* expression;
};

struct ASTNodeResumeExpression
{
    ASTNode* expression;
};

struct ASTNodeContinueExpression
{
    Buffer* name;
};

struct ASTNodeUnreachableExpression{};

struct ASTNodeErrorType
{ };

struct ASTNodeAwaitExpression
{
    ASTNode* expression;
};

struct ASTNodeSuspend
{
    ASTNode* block;
};

struct ASTNodeAnyFrameType
{
    ASTNode* payload_type;
};

struct ASTNodeEnumLiteral
{
    Token* period;
    Token* identifier;
};

struct ASTNode
{
    NodeType type;
    bool already_traced_this_node;
    size_t line;
    size_t column;
    RedType* owner;
    union
    {
        ASTNodeFunctionDefinition fn_definition;
        ASTNodeFunctionPrototype fn_prototype;
        ASTNodeParameterDeclaration param_decl;
        ASTNodeBlock block;
        ASTNode * grouped_expr;
        ASTNodeReturnExpression return_expr;
        ASTNodeDefer defer;
        ASTNodeVariableDeclaration variable_declaration;
        ASTNodeTestDeclaration test_decl;
        ASTNodeBinaryOpExpression bin_op_expr;
        ASTNodePrefixOpExpression prefix_op_expr;
        ASTNodePointerType pointer_type;
        ASTNodeFunctionCallExpression fn_call_expr;
        ASTNodeArrayAccessExpression array_access_expr;
        ASTNodeSliceExpression slice_expr;
        ASTNodeUsingNamespace using_namespace;
        ASTNodeIfBoolExpression if_bool_expr;
        ASTNodeTestExpression test_expr;
        ASTNodeWhileExpression while_expr;
        ASTNodeForExpression for_expr;
        ASTNodeSwitchExpression switch_expr;
        ASTNodeSwitchCase switch_prong;
        ASTNodeSwitchRange switch_range;
        ASTNodeCompileTime comptime_expr;
        ASTNodeNoSuspend nosuspend_expr;
        ASTNodeASMExpression asm_expr;
        ASTNodeFieldAccessExpression field_access_expr;
        ASTNodePtrDereferenceExpression ptr_deref_expr;
        ASTNodeContainerDeclaration container_decl;
        ASTNodeStructField struct_field;
        ASTNodeStringLiteral string_literal;
        ASTNodeCharLiteral char_literal;
        ASTNodeFloatLiteral float_literal;
        ASTNodeIntLiteral int_literal;
        ASTNodeContainerInitializationExpression container_init_expr;
        ASTNodeStructValueField struct_val_field;
        ASTNodeNullLiteral null_literal;
        ASTNodeUndefinedLiteral undefined_literal;
        ASTNodeThisLiteral this_literal;
        ASTNodeSymbolExpression symbol_expr;
        ASTNodeBoolLiteral bool_literal;
        ASTNodeBreakExpression break_expr;
        ASTNodeContinueExpression continue_expr;
        ASTNodeUnreachableExpression unreachable_expr;
        ASTNodeArrayType array_type;
        ASTNodeInferredArrayType inferred_array_type;
        ASTNodeErrorType error_type;
        ASTNodeErrorSetDeclaration err_set_decl;
        ASTNodeErrorSetField err_set_field;
        ASTNodeResumeExpression resume_expr;
        ASTNodeAwaitExpression await_expr;
        ASTNodeSuspend suspend;
        ASTNodeAnyFrameType anyframe_type;
        ASTNodeEnumLiteral enum_literal;
    } data;
};


void parser_error(Token* token, const char* format, ...);

ASTNode* parse(Buffer* buffer, List<Token>* tokens, RedType* owner, ErrorColor error_color);

void parser_print(ASTNode* node, s32 indent);

void AST_visit_node_children(ASTNode*, void (*visit)(ASTNode**, void* context), void* context);

