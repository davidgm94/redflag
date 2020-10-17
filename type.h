//
// Created by david on 10/16/20.
//

#pragma once

#include "types.h"
#include "buffer.h"
#include "bigint.h"
#include "hash_map.h"
#include "parser.h"

struct RedType;
struct RedValue;
struct InferredStructField;
struct ASTNode;
struct ErrorTableEntry;
struct RedFunction;
struct ScopeDecls;

enum PointerLength
{
    UNKNOWN,
    SINGLE,
    C,
};

enum ResolveStatus
{
    UNSTARTED,
    INVALID,
    BEING_INFERRED,
    ZERO_BITS_KNOWN,
    ALIGNMENT_KNOWN,
    SIZE_KNOWN,
    LLVM_FWD_DECL,
    LLVM_FULL,
};

struct RedTypePointer
{
    RedType* child_type;
    RedType* slice_parent;

    InferredStructField* inferred_struct_field;

    RedValue* sentinel;

    PointerLength pointer_length;
    u32 explicit_argument;

    u32 bit_offset_in_host;
    u32 host_int_bytes;
    u32 vector_index;

    bool is_const;
    bool is_volatile;
    bool allow_zero;
    bool resolve_loop_flag_zero_bits;
};

struct RedTypeInt
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

struct TypeStructField
{
    Buffer* name;
    RedType* type_entry;
    RedValue* type_value;
    size_t src_index;
    size_t gen_index;
    size_t offset;
    ASTNode* decl_node;
    RedValue* init_value;
    u32 bit_offset_in_host;
    u32 host_int_bytes;
    u32 align;
    bool is_comptime;
};

struct RedPackage;
struct RootStruct
{
    RedPackage* package;
    Buffer* path;
    List<size_t>* line_offsets;
    Buffer* src_code;
    /* LLVM */
};

enum StructSpecial
{
    NONE,
    SLICE,
    INFERRED_TUPLE,
    INFERRED_STRUCT,
};

struct RedTypeStruct
{
    ASTNode* decl_node;
    TypeStructField** fields;
    ScopeDecls* decls_scope;
    HashMap<Buffer*, TypeStructField*, buf_hash, buf_eql_buf> fields_by_name;
    RootStruct* root_struct;
    u32* host_int_bytes;
    size_t llvm_full_type_queue_index;

    u32 src_field_count;
    u32 gen_field_count;

    ContainerLayout layout;
    ResolveStatus resolve_status;
    StructSpecial special;

    bool requires_comptime;
    bool resolve_loop_flag_zero_bits;
    bool resolve_loop_flag_other;
    bool created_by_at_type;
};


struct RedTypeOptional
{
    RedType* child_type;
    ResolveStatus resolve_status;
};

struct RedTypeErrorUnion
{
    RedType* error_set_type;
    RedType* payload_type;
    size_t padding_bytes;
    /* LLVM */
};

struct RedTypeErrorSet
{
    ErrorTableEntry** errors;
    RedFunction* infer_function;
    u32 error_count;
    bool incomplete;
};

struct TypeEnumField
{
    Buffer* name;
    BigInt value;
    u32 declaration_index;
    ASTNode* declaration_node;
};

struct RedTypeEnum
{
    ASTNode* declaration_node;
    TypeEnumField* fields;
    RedType* tag_int_type;

    ScopeDecls* decls_scope;

    /* LLVM */

    HashMap<Buffer*, TypeEnumField*, buf_hash, buf_eql_buf> fields_by_name;
    u32 src_field_count;

    ContainerLayout layout;
    ResolveStatus resolve_status;

    bool has_explicit_tag_type;
    bool non_exhaustive;
    bool resolve_loop_flag;
};

struct TypeUnionField
{
    Buffer* name;
    RedType* type_entry;
    RedValue* type_value;
    TypeEnumField* enum_field;
    ASTNode* declaration_node;
    u32 gen_index;
    u32 align;
};

struct RedTypeUnion
{
    ASTNode* declaration_node;
    TypeUnionField* fields;
    ScopeDecls* decls_scope;
    HashMap<Buffer*, TypeUnionField*, buf_hash, buf_eql_buf> fields_by_name;
    RedType* tag_type;
    /* LLVM */
    TypeUnionField* most_aligned_union_member;
    size_t gen_union_index;
    size_t gen_tag_index;
    size_t union_abi_size;

    u32 src_field_count;
    u32 gen_field_count;

    ContainerLayout layout;
    ResolveStatus resolve_status;

    bool has_explicit_tag_type;
    bool requires_comptime;
    bool resolve_loop_flag_zero_bits;
    bool resolve_loop_flag_other;
};

struct FunctionGenParamInfo
{
    size_t src_index;
    size_t gen_index;
    bool is_by_value;
    RedType* type;
};

struct FunctionTypeParamInfo
{
    bool is_no_alias;
    RedType* type;
};

enum CallingConvention
{
    CALLING_CONVENTION_UNSPECIFIED,
    CALLING_CONVENTION_C,
    CALLING_CONVENTION_NAKED,
    CALLING_CONVENTION_ASYNC,
    CALLING_CONVENTION_INTERRUPT,
    CALLING_CONVENTION_SIGNAL,
    CALLING_CONVENTION_STDCALL,
    CALLING_CONVENTION_FASTCALL,
    CALLING_CONVENTION_VECTORCALL,
    CALLING_CONVENTION_THISCALL,
    CALLING_CONVENTION_APCS,
    CALLING_CONVENTION_AAPCS,
    CALLING_CONVENTION_AAPCSVFP,
};

struct FunctionTypeID
{
    RedType* return_type;
    FunctionTypeParamInfo* param_info;
    size_t param_count;
    size_t next_param_index;
    bool is_var_args;
    CallingConvention cc;
    u32 alignment;
};

struct RedTypeFn
{
    FunctionTypeID id;
    bool is_generic;
    RedType* gen_return_type;
    size_t gen_param_count;
    FunctionGenParamInfo* gen_param_info;

    /* LLVM */

    RedType* bound_fn_parent;
};

struct RedTypeBoundFn
{
    RedType* fn_type;
};

struct RedTypeVector
{
    RedType* elem_type;
    u64 length;
    size_t padding;
};

static_assert(sizeof(RedTypeVector) == sizeof(RedTypeArray), "Size of RedTypeVector and RedTypeArray do not match!");

struct RedTypeOpaque
{
    ASTNode* decl_node;
    Buffer* bare_name;

    ScopeDecls* decls_scope;
};

struct RedTypeFnFrame
{
    RedFunction* fn;
    RedType* locals_struct;

    RedType* resolved_loop_type;
    ASTNode* resolve_loop_src_node;
    bool reported_loop_error;
};

struct RedTypeAnyFrame
{
    RedType* result_type;
};

enum RedTypeID
{
    RED_TYPE_INVALID,
    RED_TYPE_META_TYPE,
    RED_TYPE_VOID,
    RED_TYPE_BOOL,
    RED_TYPE_UNREACHABLE,
    RED_TYPE_INT,
    RED_TYPE_FLOAT,
    RED_TYPE_POINTER,
    RED_TYPE_ARRAY,
    RED_TYPE_STRUCT,
    RED_TYPE_COMPILE_TIME_FLOAT,
    RED_TYPE_COMPILE_TIME_INT,
    RED_TYPE_UNDEFINED,
    RED_TYPE_NULL,
    RED_TYPE_OPTIONAL,
    RED_TYPE_ERROR_UNION,
    RED_TYPE_ERROR_SET,
    RED_TYPE_ENUM,
    RED_TYPE_UNION,
    RED_TYPE_FUNCTION,
    RED_TYPE_BOUND_FUNCTION,
    RED_TYPE_OPAQUE,
    RED_TYPE_FUNCTION_FRAME,
    RED_TYPE_ANY_FRAME,
    RED_TYPE_VECTOR,
    RED_TYPE_ENUM_LITERAL,
};

enum OnePossibleValue
{
    ONE_POSSIBLE_VALUE_INVALID,
    ONE_POSSIBLE_VALUE_NO,
    ONE_POSSIBLE_VALUE_YES,
};

struct RedType
{
    RedTypeID id;
    Buffer name;

    /* LLVM */

    union
    {
        RedTypePointer pointer;
        RedTypeInt integer;
        RedTypeFloat floating;
        RedTypeArray array;
        RedTypeStruct structure;
        RedTypeOptional optional;
        RedTypeErrorUnion error_union;
        RedTypeErrorSet error_set;
        RedTypeEnum enumeration;
        RedTypeUnion union_;
        RedTypeFn function;
        RedTypeBoundFn bound_fn;
        RedTypeVector vector;
        RedTypeOpaque opaque;
        RedTypeFnFrame frame;
        RedTypeAnyFrame any_frame;
    } data;

    RedType* pointer_parent[2];
    RedType* optional_parent;
    RedType* any_frame_parent;

    RedType* cached_const_name_value;

    OnePossibleValue one_possible_value;

    u32 abi_align;
    size_t abi_size;
    size_t size_in_bits;
};