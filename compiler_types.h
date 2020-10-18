//
// Created by david on 8/10/20.
//
#pragma once
#include "types.h"


/* Forward declaration of structs */
struct RedType;
struct ASTNode;
//struct RedPackage;
/* Enum declaration */
#include "list.h"
enum Error {
    ERROR_NONE,
    ERROR_NO_MEM,
    ERROR_INVALID_FORMAT,
    ERROR_SEMANTIC_ANALYZEFAIL,
    ERROR_ACCESS,
    ERROR_INTERRUPTED,
    ERROR_SYSTEM_RESOURCES,
    ERROR_FILE_NOT_FOUND,
    ERROR_FILE_SYSTEM,
    ERROR_FILE_TOO_BIG,
    ERROR_DIV_BY_ZERO,
    ERROR_OVERFLOW,
    ERROR_PATH_ALREADY_EXISTS,
    ERROR_UNEXPECTED,
    ERROR_EXACT_DIV_REMAINDER,
    ERROR_NEGATIVE_DENOMINATOR,
    ERROR_SHIFTED_OUT_ONEBITS,
    ERROR_C_COMPILE_ERRORS,
    ERROR_END_OF_FILE,
    ERROR_IS_DIR,
    ERROR_NOT_DIR,
    ERROR_UNSUPPORTED_OPERATING_SYSTEM,
    ERROR_SHARING_VIOLATION,
    ERROR_PIPE_BUSY,
    ERROR_PRIMITIVE_TYPE_NOTFOUND,
    ERROR_CACHE_UNAVAILABLE,
    ERROR_PATH_TOO_LONG,
    ERROR_C_COMPILER__CANNOT_FIND_FILE,
    ERROR_NO_C_COMPILER_INSTALLED,
    ERROR_READING_DEP_FILE,
    ERROR_INVALID_DEP_FILE,
    ERROR_MISSING_ARCHITECTURE,
    ERROR_MISSING_OPERATING_SYSTEM,
    ERROR_UNKNOWN_ARCHITECTURE,
    ERROR_UNKNOWN_OPERATING_SYSTEM,
    ERROR_UNKNOWN_ABI,
    ERROR_INVALID_FILENAME,
    ERROR_DISK_QUOTA,
    ERROR_DISK_SPACE,
    ERROR_UNEXPECTED_WRITE_FAILURE,
    ERROR_UNEXPECTED_SEEK_FAILURE,
    ERROR_UNEXPECTED_FILE_TRUNCATION_FAILURE,
    ERROR_UNIMPLEMENTED,
    ERROR_OPERATION_ABORTED,
    ERROR_BROKEN_PIPE,
    ERROR_NO_SPACE_LEFT,
    ERROR_NOT_LAZY,
    ERROR_IS_ASYNC,
    ERROR_IMPORT_OUTSIDE_PKG_PATH,
    ERROR_UNKNOWN_CPU,
    ERROR_UNKNOWN_CPU_FEATURE,
    ERROR_INVALID_CPU_FEATURES,
    ERROR_INVALID_LLVM_CPU_FEATURES_FORMAT,
    ERROR_UNKNOWN_APPLICATION_BINARY_INTERFACE,
    ERROR_AST_UNIT_FAILURE,
    ERROR_BAD_PATH_NAME,
    ERROR_SYM_LINK_LOOP,
    ERROR_PROCESS_FD_QUOTA_EXCEEDED,
    ERROR_SYSTEM_FD_QUOTA_EXCEEDED,
    ERROR_NO_DEVICE,
    ERROR_DEVICE_BUSY,
    ERROR_UNABLE_TO_SPAWN_C_COMPILER,
    ERROR_C_COMPILER_EXIT_CODE,
    ERROR_C_COMPILER_CRASHED,
    ERROR_C_COMPILER_CANNOT_FIND_HEADERS,
    ERROR_LIB_C_RUNTIME_NOT_FOUND,
    ERROR_LIB_C_STD_LIB_HEADER_NOT_FOUND,
    ERROR_LIB_C_KERNEL32_LIB_NOT_FOUND,
    ERROR_UNSUPPORTED_ARCHITECTURE,
    ERROR_WINDOWS_SDK_NOT_FOUND,
    ERROR_UNKNOWN_DYNAMIC_LINKER_PATH,
    ERROR_TARGET_HAS_NO_DYNAMIC_LINKER,
    ERROR_INVALID_ABI_VERSION,
    ERROR_INVALID_OPERATING_SYSTEM_VERSION,
    ERROR_UNKNOWN_CLANG_OPTION,
    ERROR_NESTED_RESPONSE_FILE,
    ERROR_ZIG_IS_THEC_COMPILER,
    ERROR_FILE_BUSY,
    ERROR_LOCKED,
};
enum Cmp
{
    CMP_LESS,
    CMP_GREATER,
    CMP_EQ
};

enum TokenID
{
    TOKEN_ID_AMPERSAND,
    TOKEN_ID_ARROW,
    TOKEN_ID_AT,
    TOKEN_ID_BANG,
    TOKEN_ID_BIT_OR,
    TOKEN_ID_BIT_OR_EQ,
    TOKEN_ID_BIT_XOR,
    TOKEN_ID_BIT_XOR_EQ,
    TOKEN_ID_BIT_AND,
    TOKEN_ID_BIT_AND_EQ,
    TOKEN_ID_BIT_SHL,
    TOKEN_ID_BIT_SHL_EQ,
    TOKEN_ID_BIT_SHR,
    TOKEN_ID_BIT_SHR_EQ,
    TOKEN_ID_CHAR_LIT,
    TOKEN_ID_CMP_EQ,
    TOKEN_ID_CMP_GREATER_OR_EQ,
    TOKEN_ID_CMP_GREATER,
    TOKEN_ID_CMP_LESS_OR_EQ,
    TOKEN_ID_CMP_LESS,
    TOKEN_ID_CMP_NOT_EQ,
    TOKEN_ID_COLON,
    TOKEN_ID_COMMA,
    TOKEN_ID_DASH,
    TOKEN_ID_DIV_EQ,
    TOKEN_ID_DOT,
    TOKEN_ID_END_OF_FILE,
    TOKEN_ID_EQ,
    TOKEN_ID_FAT_ARROW,
    TOKEN_ID_FLOAT_LIT,
    TOKEN_ID_INT_LIT,
    TOKEN_ID_HASH,
    TOKEN_ID_KEYWORD_ALIGN,
    TOKEN_ID_KEYWORD_ALLOW_ZERO,
    TOKEN_ID_KEYWORD_ANY,
    TOKEN_ID_KEYWORD_ANY_FRAME,
    TOKEN_ID_KEYWORD_CALL_CONV,
    TOKEN_ID_KEYWORD_COMPTIME,
    TOKEN_ID_KEYWORD_CONST,
    TOKEN_ID_KEYWORD_DEFER,
    TOKEN_ID_KEYWORD_ELSE,
    TOKEN_ID_KEYWORD_ENUM,
    TOKEN_ID_KEYWORD_ERROR,
    TOKEN_ID_KEYWORD_ERROR_DEFER,
    TOKEN_ID_KEYWORD_EXPORT,
    TOKEN_ID_KEYWORD_EXTERN,
    TOKEN_ID_KEYWORD_FALSE,
    TOKEN_ID_KEYWORD_FN,
    TOKEN_ID_KEYWORD_FOR,
    TOKEN_ID_KEYWORD_IF,
    TOKEN_ID_KEYWORD_INLINE,
    TOKEN_ID_KEYWORD_NO_ALIAS,
    TOKEN_ID_KEYWORD_NO_INLINE,
    TOKEN_ID_KEYWORD_NULL,
    TOKEN_ID_KEYWORD_PACKED,
    TOKEN_ID_KEYWORD_PUB,
    TOKEN_ID_KEYWORD_SECTION,
    TOKEN_ID_KEYWORD_STRUCT,
    TOKEN_ID_KEYWORD_SWITCH,
    TOKEN_ID_KEYWORD_TEST,
    TOKEN_ID_KEYWORD_THREAD_LOCAL,
    TOKEN_ID_KEYWORD_TRUE,
    TOKEN_ID_KEYWORD_UNDEFINED,
    TOKEN_ID_KEYWORD_UNION,
    TOKEN_ID_KEYWORD_VAR,
    TOKEN_ID_KEYWORD_VOLATILE,
    TOKEN_ID_KEYWORD_WHILE,
    // ...
    TOKEN_ID_LEFT_BRACE,
    TOKEN_ID_LEFT_BRACKET,
    TOKEN_ID_LEFT_PARENTHESIS,
    TOKEN_ID_QUESTION,
    TOKEN_ID_MINUS_EQ,
    TOKEN_ID_MOD_EQ,
    TOKEN_ID_PERCENT,
    TOKEN_ID_PLUS,
    TOKEN_ID_PLUS_EQ,
    TOKEN_ID_RIGHT_BRACE,
    TOKEN_ID_RIGHT_BRACKET,
    TOKEN_ID_RIGHT_PARENTHESIS,
    TOKEN_ID_SEMICOLON,
    TOKEN_ID_SLASH,
    TOKEN_ID_STAR,
    TOKEN_ID_STRING_LIT,
    TOKEN_ID_MULTILINE_STRING_LIT,
    TOKEN_ID_SYMBOL,
    TOKEN_ID_TILDE,
    TOKEN_ID_TIMES_EQ,
    TOKEN_ID_COUNT
};

enum RedTypeID
{
    RED_TYPE_INVALID,
    RED_TYPE_VOID,
    RED_TYPE_BOOL,
    RED_TYPE_UNREACHABLE,
    RED_TYPE_INT,
    RED_TYPE_FLOAT,
    RED_TYPE_POINTER,
    RED_TYPE_ARRAY,
    RED_TYPE_STRUCT,
    RED_TYPE_ENUM,
    RED_TYPE_UNION,
    RED_TYPE_UNDEFINED,
    RED_TYPE_NULL,
    RED_TYPE_FUNCTION,
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

enum ScopeID
{
    DECL,
    BLOCK,
    VAR_DECL,
    LOOP,
    FUNCTION_DEF,
    EXPRESSION,
};

enum PrefixOp
{
    PREFIX_OP_INVALID,
    PREFIX_OP_BOOL_NOT,
    PREFIX_OP_BIN_NOT,
    PREFIX_OP_NEGATION,
    PREFIX_OP_NEGATION_WRAP,
    PREFIX_OP_OPTIONAL,
    PREFIX_OP_ADDR_OF,
};

enum NodeType
{
    NODE_TYPE_FN_PROTO,
    NODE_TYPE_FN_DEF,
    NODE_TYPE_PARAM_DECL,
    NODE_TYPE_BLOCK,
    NODE_TYPE_GROUPED_EXPR,
    NODE_TYPE_RETURN_EXPR,
    NODE_TYPE_VARIABLE_DECLARATION,
    NODE_TYPE_BIN_OP_EXPR,
    NODE_TYPE_FLOAT_LITERAL,
    NODE_TYPE_INT_LITERAL,
    NODE_TYPE_STRING_LITERAL,
    NODE_TYPE_CHAR_LITERAL,
    NODE_TYPE_SYMBOL,
    NODE_TYPE_PREFIX_OP_EXPR,
    NODE_TYPE_POINTER_TYPE,
    NODE_TYPE_FN_CALL_EXPR,
    NODE_TYPE_ARRAY_ACCESS_EXPR,
    NODE_TYPE_FIELD_ACCESS_EXPR,
    NODE_TYPE_PTR_DEREF,
    NODE_TYPE_BOOL_LITERAL,
    NODE_TYPE_NULL_LITERAL,
    NODE_TYPE_UNDEFINED_LITERAL,
    NODE_TYPE_UNREACHABLE,
    NODE_TYPE_IF_BOOL_EXPR,
    NODE_TYPE_WHILE_EXPR,
    NODE_TYPE_FOR_EXPR,
    NODE_TYPE_SWITCH_EXPR,
    //NODE_TYPE_SWITCH_PRONG,
    //NODE_TYPE_SWITCH_RANGE,
    NODE_TYPE_BREAK,
    NODE_TYPE_CONTINUE,
    //NODE_TYPE_ASM_EXPR,
    NODE_TYPE_CONTAINER_DECL,
    NODE_TYPE_STRUCT_FIELD,
    // ?? NODE_TYPE_CONTAINER_INIT_EXPR,
    // NODE_TYPE_STRUCT_VALUE_FIELD,
    //NODE_TYPE_ARRAY_TYPE,
    //NODE_TYPE_INFERRED_ARRAYTYPE,
    //NODE_TYPE_ERROR_TYPE,
    //NODE_TYPE_IF_ERROR_EXPR,
    //NODE_TYPE_IF_OPTIONAL,
    NODE_TYPE_ENUM_LITERAL,
    //NODE_TYPE_ANY_TYPE_FIELD,
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


struct BigInt
{
    size_t digit_count;
    union
    {
        u64 digit;
        u64* digits; // least significan digit first
    } data;
    bool is_negative;
};

struct BigFloat
{
    f128 value;
};
struct TokenFloatLit
{
    BigFloat big_float;
    bool overflow;
};
struct TokenIntLit
{
    BigInt big_int;
};
struct TokenStrLit
{
    Buffer str;
};
struct TokenCharLit
{
    // TODO: we are only supporting 1-byte characters for now
    char c;
};

struct Token
{
    TokenID id;
    size_t start_position;
    size_t end_position;
    size_t start_line;
    size_t start_column;

    union
    {
        TokenIntLit int_lit;
        TokenFloatLit float_lit;
        TokenStrLit str_lit;
        TokenCharLit char_lit;
    } data;
};

struct Scope
{
    ScopeID id;
    ASTNode* src_node;
    Scope* parent;
};

struct ASTNodeFunctionPrototype
{
    Buffer* name;
    List<ASTNode*> parameters;
    ASTNode* return_type;
    ASTNode* function_definition_node;
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
};

struct ASTNodeBlock
{
    Buffer* name;
    List<ASTNode*> statements;
};

//struct ASTNodeDefer
//{
//    ASTNode* expression;
//
//    Scope* child_scope;
//    Scope* expression_scope;
//};

struct ASTNodeVariableDeclaration
{
    Buffer* symbol;
    ASTNode* type;
    ASTNode* expression;

    bool is_mutable;
};
struct ASTNodeBinaryOpExpression
{
    ASTNode* op1;
    ASTNode* op2;
    BinaryOpType op;
};

struct ASTNodeFunctionCallExpression
{
    ASTNode* function;
    List<ASTNode*> parameters;
    // ?? bool seen;
};

struct ASTNodeArrayAccessExpression
{
    ASTNode* array;
    ASTNode* subscript;
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
    bool is_const;
};

struct ASTNodeArrayType
{
    ASTNode* size;
    ASTNode* sentinel;
    ASTNode* child_type;
    bool is_const;
};

struct ASTNodeIfBoolExpression
{
    ASTNode* condition;
    ASTNode* true_block;
    ASTNode* false_block;
};

struct ASTNodeWhileExpression
{
    Buffer* name;
    ASTNode* condition;
    ASTNode* continue_expression;
    ASTNode* body;
};

struct ASTNodeForExpression
{
    Buffer* name;
    ASTNode* element_node;
    ASTNode* index_node;
    ASTNode* body;
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
};

struct SourcePosition
{
    size_t line;
    size_t column;
};

struct ASTNodeContainerDeclaration
{
    Buffer* name;
    ContainerType type;
    /* TODO: is this any different? */
    List<ASTNode*> fields;
    List<ASTNode*> declarations;
    /* TODO: is this any different? */

    bool is_root;
};

struct ASTNodeStructField
{
    Buffer* name;
    ASTNode* type;
    ASTNode* value;
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

struct ASTNodeContinueExpression
{
    Buffer* name;
};

struct ASTNodeBreakExpression
{
    Buffer* name;
    ASTNode* expression;
};
struct ASTNodeUnreachableExpression{};

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
        ASTNode* grouped_expr;
        ASTNode* return_expr;
        ASTNodeVariableDeclaration variable_declaration;
        ASTNodeBinaryOpExpression bin_op_expr;
        ASTNodePrefixOpExpression prefix_op_expr;
        ASTNodePointerType pointer_type;
        ASTNodeFunctionCallExpression fn_call_expr;
        ASTNodeArrayAccessExpression array_access_expr;
        ASTNodeIfBoolExpression if_bool_expr;
        ASTNodeWhileExpression while_expr;
        ASTNodeForExpression for_expr;
        ASTNodeSwitchExpression switch_expr;
        ASTNodeSwitchCase switch_prong;
        ASTNodeFieldAccessExpression field_access_expr;
        ASTNodePtrDereferenceExpression ptr_deref_expr;
        ASTNodeContainerDeclaration container_decl;
        ASTNodeStructField struct_field;
        ASTNodeStringLiteral string_literal;
        ASTNodeCharLiteral char_literal;
        ASTNodeFloatLiteral float_literal;
        ASTNodeIntLiteral int_literal;
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
        ASTNodeEnumLiteral enum_literal;
    } data;
};

struct LexingResult
{
    List<Token> tokens;
    List<size_t> line_offsets;

    Buffer* error;
    size_t error_line;
    size_t error_column;
};


struct RedValue
{
    RedType* type;
    union
    {
        int value;
    } data;
};

struct RedTypePointer
{
    RedType* type;
    RedValue* sentinel;
    bool is_const;
    bool is_volatile;
};

struct RedTypeInteger
{
    u32 bit_count;
    bool is_signed;
};

struct RedTypeFloat
{
    size_t bit_count;
};

struct RedTypeArray
{
    RedType* child_type;
    u64 length;
    RedValue* sentinel;
};

struct RootStruct
{
    Buffer* path;
    List<size_t>* line_offsets;
    Buffer* src_code;
};

struct TypeStructField
{
    Buffer* name;
    RedType* type_entry;
    RedValue* type_value;
    size_t offset;
    ASTNode* decl_node;
    RedValue* init_value;
    u32 align;
};

struct RedTypeStruct
{
    ASTNode* decl_node;
    TypeStructField* fields;
    Buffer* field_names;
    RootStruct* root_struct;
    u32 field_count;
};

struct TypeEnumField
{
    Buffer* name;
    BigInt value;
};

struct RedTypeEnum
{
    ASTNode* decl_node;
    TypeEnumField* fields;
};

struct TypeUnionField
{
    Buffer* name;
    RedType* type_entry;
    RedValue* type_value;
};

struct RedTypeUnion
{
    ASTNode* declaration_node;
    TypeUnionField* fields;
};

struct TypeFunctionParameter
{
    RedType* type;
};

struct RedTypeFunction
{
    RedType* return_type;
    TypeFunctionParameter* parameters;
    size_t parameter_count;
};

struct RedType
{
    RedTypeID id;
    Buffer name;

    union
    {
        RedTypePointer pointer;
        RedTypeInteger integer;
        RedTypeFloat floating;
        RedTypeArray array;
        RedTypeStruct structure;
        RedTypeEnum enumeration;
        RedTypeUnion union_;
        RedTypeFunction function;
    } data;
};