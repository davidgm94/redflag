
#include "compiler_types.h"
#include "lexer.h"
#include "ir.h"
#include "os.h"

GEN_BUFFER_FUNCTIONS(decl, db, IRSymDeclStatementBuffer, IRSymDeclStatement)
GEN_BUFFER_FUNCTIONS(ir_stmtb, sb, IRStatementBuffer, IRStatement)
GEN_BUFFER_FUNCTIONS(ir_fn_def, fb, IRFunctionDefinitionBuffer, IRFunctionDefinition)
GEN_BUFFER_FUNCTIONS(ir_struct, sb, IRStructDeclBuffer, IRStructDecl)
GEN_BUFFER_FUNCTIONS(ir_enum_field, efb, IREnumFieldBuffer, IREnumField)
GEN_BUFFER_FUNCTIONS(ir_enum, eb, IREnumDeclBuffer, IREnumDecl)

const char* primitive_types_str[] =
{
    "u8", "u16", "u32", "u64",
    "s8", "s16", "s32", "s64",
    "f32", "f64", "f128",
};

static inline IRExpression ast_to_ir_expression(ASTNode* node, IRModule* module, IRFunctionDefinition* parent_fn, IRLoadStoreCfg use_type);
static const IRType primitive_types[] = {
    [0] =
    {
        .kind = TYPE_KIND_PRIMITIVE,
        .size = 1,
        .primitive_type = IR_TYPE_PRIMITIVE_U8,
    },
    [1] =
    {
        .kind = TYPE_KIND_PRIMITIVE,
        .size = 2,
        .primitive_type = IR_TYPE_PRIMITIVE_U16,
    },
    [2] =
    {
        .kind = TYPE_KIND_PRIMITIVE,
        .size = 4,
        .primitive_type = IR_TYPE_PRIMITIVE_U32,
    },
    [3] =
    {
        .kind = TYPE_KIND_PRIMITIVE,
        .size = 8,
        .primitive_type = IR_TYPE_PRIMITIVE_U64,
    },
    [4] =
    {
        .kind = TYPE_KIND_PRIMITIVE,
        .size = 1,
        .primitive_type = IR_TYPE_PRIMITIVE_S8,
    },
    [5] =
    {
        .kind = TYPE_KIND_PRIMITIVE,
        .size = 2,
        .primitive_type = IR_TYPE_PRIMITIVE_S16,
    },
    [6] =
    {
        .kind = TYPE_KIND_PRIMITIVE,
        .size = 4,
        .primitive_type = IR_TYPE_PRIMITIVE_S32,
    },
    [7] =
    {
        .kind = TYPE_KIND_PRIMITIVE,
        .size = 8,
        .primitive_type = IR_TYPE_PRIMITIVE_S64,
    },
    [8] =
    {
        .kind = TYPE_KIND_PRIMITIVE,
        .size = 4,
        .primitive_type = IR_TYPE_PRIMITIVE_F32,
    },
    [9] =
    {
        .kind = TYPE_KIND_PRIMITIVE,
        .size = 8,
        .primitive_type = IR_TYPE_PRIMITIVE_F64,
    },
    [10] =
    {
        .kind = TYPE_KIND_PRIMITIVE,
        .size = 16,
        .primitive_type = IR_TYPE_PRIMITIVE_F128,
    },
};

static inline IRType ast_to_ir_resolve_type(ASTNode* node, IRFunctionDefinition* parent_fn, IRModule* ir_module);
static inline bool red_type_is_invalid(IRType* type)
{
    return memcmp(type, &(const IRType)ZERO_INIT, sizeof(IRType)) == 0;
}

static inline IRType resolve_basic_type(ASTNode* node)
{
    redassert(node->type_expr.kind == TYPE_KIND_PRIMITIVE);
    char* type_name = sb_ptr(node->type_expr.name);

    static_assert(array_length(primitive_types) == array_length(primitive_types_str), "Error in primitive type count");
    for (s32 i = 0; i < array_length(primitive_types); i++)
    {
        if (strcmp(type_name, primitive_types_str[i]) == 0)
        {
            return primitive_types[i];
        }
    }

    return (const IRType)ZERO_INIT;
}

static inline IRType resolve_array_type(ASTNode* node, IRFunctionDefinition* parent_fn, IRModule* ir_module)
{
    ASTArrayType* array_type = &node->type_expr.array;
    redassert(array_type->type->node_id == AST_TYPE_TYPE_EXPR);
    IRType* base_type = NEW(IRType, 1);
    IRExpression* elem_count_expr = NEW(IRExpression, 1);
    *base_type = ast_to_ir_resolve_type(array_type->type, parent_fn, ir_module);
    *elem_count_expr = ast_to_ir_expression(array_type->element_count_expr, ir_module, parent_fn, LOAD);

    IRType type;
    type.kind = TYPE_KIND_ARRAY;
    type.size = 0;
    type.array_type.base_type = base_type;
    type.array_type.elem_count_expr = elem_count_expr;
    return type;
}

static inline IRType resolve_struct_type(ASTNode* node, IRModule* ir_tree)
{
    IRStructDeclBuffer* struct_decls = &ir_tree->struct_decls;
    u64 struct_decl_count = struct_decls->len;
    IRStructDecl* struct_decl_ptr = struct_decls->ptr;

    if (struct_decl_count > 0)
    {
        for (u64 i = 0; i < struct_decl_count; i++)
        {
            IRStructDecl* struct_decl = &struct_decl_ptr[i];
            SB* name = &struct_decl->name;
            if (sb_cmp(name, node->type_expr.name))
            {
                IRType type;
                type.struct_type = struct_decl;
                type.kind = TYPE_KIND_STRUCT;
                type.size = 0;
                return type;
            }
        }
    }

    return (const IRType)ZERO_INIT;
}

static inline IRType ast_to_ir_resolve_enum_type(ASTNode* node, IRModule* module)
{
    IREnumDeclBuffer* eb = &module->enum_decls;
    u64 enum_count = eb->len;
    if (enum_count > 0)
    {
        IREnumDecl* enum_decl_ptr = eb->ptr;
        for (u64 i = 0; i < enum_count; i++)
        {
            IREnumDecl* enum_decl = &enum_decl_ptr[i];
            SB* name = enum_decl->name;
            if (sb_cmp(name, node->type_expr.name))
            {
                IRType type;
                type.enum_type = enum_decl;
                type.kind = TYPE_KIND_ENUM;
                type.size = 0;
                return type;
            }
        }
    }
    return (const IRType) { 0 };
}

static inline IRType ast_to_ir_resolve_union_type(ASTNode* node, IRModule* module)
{
    RED_NOT_IMPLEMENTED;
    return (const IRType) { 0 };
}

static inline IRType ast_to_ir_resolve_complex_type(ASTNode* node, IRModule* ir_tree)
{
    IRType type = resolve_struct_type(node, ir_tree);
    if (!red_type_is_invalid(&type))
    {
        return type;
    }

    type = ast_to_ir_resolve_enum_type(node, ir_tree);
    if (!red_type_is_invalid(&type))
    {
        return type;
    }

    type = ast_to_ir_resolve_union_type(node, ir_tree);
    if (!red_type_is_invalid(&type))
    {
        return type;
    }

    return (const IRType)ZERO_INIT;
}

static inline IRType ast_to_ir_resolve_pointer_type(ASTNode* node, IRFunctionDefinition* parent_fn, IRModule* module)
{
    ASTNode* pointer_type = node->type_expr.pointer_.type;
    redassert(pointer_type->node_id == AST_TYPE_TYPE_EXPR);
    IRType pointer_type_ir = ast_to_ir_resolve_type(pointer_type, parent_fn, module);
    
    IRType type;
    type.kind = TYPE_KIND_POINTER;
    type.pointer_type.base_type = NEW(IRType, 1);
    *type.pointer_type.base_type = pointer_type_ir;

    return type;
}

static inline IRType ast_to_ir_resolve_type(ASTNode* node, IRFunctionDefinition* parent_fn, IRModule* ir_tree)
{
    redassert(node->node_id == AST_TYPE_TYPE_EXPR);
    TypeKind type_kind = node->type_expr.kind;
    switch (type_kind)
    {
        case TYPE_KIND_PRIMITIVE:
            return resolve_basic_type(node);
        case TYPE_KIND_ARRAY:
            return resolve_array_type(node, parent_fn, ir_tree);
        case TYPE_KIND_STRUCT:
            return resolve_struct_type(node, ir_tree);
        case TYPE_KIND_ENUM:
            return ast_to_ir_resolve_enum_type(node, ir_tree);
        case TYPE_KIND_COMPLEX_TO_BE_DETERMINED:
            return ast_to_ir_resolve_complex_type(node, ir_tree);
        case TYPE_KIND_POINTER:
            return ast_to_ir_resolve_pointer_type(node, parent_fn, ir_tree);
        default:
            RED_NOT_IMPLEMENTED;
            return (const IRType)ZERO_INIT;
    }
}

static inline SB* param_name(ASTNode* node)
{
    redassert(node->node_id == AST_TYPE_PARAM_DECL);
    return node->param_decl.sym->sym_expr.name;
}

static inline bool param_name_unique(IRParamDecl* param_arr, u32 param_count, SB* current_param_name)
{
    for (u32 i = 0; i < param_count; i++)
    {
        if (strcmp(sb_ptr(param_arr[i].name), sb_ptr(current_param_name)) == 0)
        {
            return false;
        }
    }

    return true;
}

static inline bool type_matches_primitive(IRType* red_type, ASTNode* node)
{
    redassert(red_type->kind == TYPE_KIND_PRIMITIVE);

    bool matches = false;
    IRTypePrimitive primitive_type = red_type->primitive_type;
    switch (primitive_type)
    {
        case IR_TYPE_PRIMITIVE_S32:
            matches = node->node_id == AST_TYPE_INT_LIT;
            break;
        default:
            RED_NOT_IMPLEMENTED;
            break;
    }
    return matches;
}
static inline bool type_matches(IRType* red_type, ASTNode* node)
{
    TypeKind kind = red_type->kind;
    switch (kind)
    {
        case TYPE_KIND_INVALID:
            RED_UNREACHABLE;
            return false;
        case TYPE_KIND_PRIMITIVE:
            return type_matches_primitive(red_type, node);
        case TYPE_KIND_FUNCTION:
            RED_NOT_IMPLEMENTED;
            return false;
        case TYPE_KIND_VOID:
            RED_NOT_IMPLEMENTED;
            return false;
        default:
            RED_NOT_IMPLEMENTED;
            return false;
    }
}

static inline IRBinaryExpr ast_to_ir_binary_expr(ASTBinExpr* bin_expr, IRModule* module, IRFunctionDefinition* parent_fn);
static inline IRIntLiteral ast_to_ir_int_lit_expr(ASTNode* node)
{
    IRIntLiteral lit;
    redassert(node->node_id == AST_TYPE_INT_LIT);

    lit.bigint = *node->int_lit.bigint;
    return lit;
}

static inline IRSymExpr find_symbol(SB* symbol, IRModule* module, IRFunctionDefinition* fn_definition, IRLoadStoreCfg use_type)
{
    IRFunctionPrototype* proto = &fn_definition->proto;
    u8 param_count = proto->param_count;
    for (usize i = 0; i < param_count; i++)
    {
        SB* name_it = proto->params[i].name;
        if (sb_cmp(name_it, symbol))
        {
            IRSymExpr result = ZERO_INIT;
            result.type = IR_SYM_EXPR_TYPE_PARAM;
            result.param_decl = &proto->params[i];
            result.use_type = use_type;
            return result;
        }
    }
    
    IRSymDeclStatementBuffer* sym_decl_bf = &fn_definition->sym_declarations;
    u32 sym_decl_count = sym_decl_bf->len;
    for (u32 i = 0; i < sym_decl_count; i++)
    {
        IRSymDeclStatement* decl = &sym_decl_bf->ptr[i];
        if (sb_cmp(decl->name, symbol))
        {
            IRSymExpr result = ZERO_INIT;
            result.type = IR_SYM_EXPR_TYPE_SYM;
            result.sym_decl = decl;
            result.use_type = use_type;
            return result;
        }
    }
    
    IRStructDeclBuffer* strb = &module->struct_decls;
    u64 struct_count = strb->len;
    if (struct_count > 0)
    {
        IRStructDecl* ptr = strb->ptr;
        for (u64 i = 0; i < struct_count; i++)
        {
            IRStructDecl* struct_decl = &ptr[i];
            SB* struct_name = &struct_decl->name;
            if (sb_cmp(symbol, struct_name))
            {
                IRSymExpr result = ZERO_INIT;
                result.type = IR_SYM_EXPR_TYPE_SYM;
                result.struct_decl = struct_decl;
                RED_NOT_IMPLEMENTED;
                result.use_type = use_type;
                return result;
            }
        }
    }

    IREnumDeclBuffer* eb = &module->enum_decls;
    u64 enum_count = eb->len;
    if (enum_count > 0)
    {
        IREnumDecl* ptr = eb->ptr;
        
        for (u64 i = 0; i < enum_count; i++)
        {
            IREnumDecl* enum_decl = &ptr[i];
            SB* enum_name = enum_decl->name;
            if (sb_cmp(enum_name, symbol))
            {
                IRSymExpr result = ZERO_INIT;
                result.type = IR_SYM_EXPR_TYPE_ENUM;
                result.enum_decl = enum_decl;
                result.use_type = use_type;
                return result;
            }
        }
    }

    return (const IRSymExpr)ZERO_INIT;
}

static inline IRArrayLiteral ast_to_ir_array_lit(ASTNode* node, IRModule* module, IRFunctionDefinition* parent_fn) 
{
    redassert(node->node_id == AST_TYPE_ARRAY_LIT);
    IRArrayLiteral array_lit = ZERO_INIT;
    u64 lit_count = node->array_lit.values.len;
    if (lit_count > 0)
    {
        array_lit.expressions = NEW(IRExpression, lit_count);
        array_lit.expression_count = lit_count;
        ASTNode** lit_arr = node->array_lit.values.ptr;
        for (u64 i = 0; i < lit_count; i++)
        {
            ASTNode* lit = lit_arr[i];
            array_lit.expressions[i] = ast_to_ir_expression(lit, module, parent_fn, LOAD);
        }
    }

    return array_lit;
}

static inline void ast_to_ir_field_use(ASTNode* node, IRExpression* expr, IRFunctionDefinition* parent_fn, IRLoadStoreCfg use_type)
{
    AST_ID id = node->node_id;
    redassert(id == AST_TYPE_SYM_EXPR);
    redassert(node->sym_expr.subscript->node_id == AST_TYPE_SYM_EXPR);
    redassert(expr->type == IR_EXPRESSION_TYPE_SYM_EXPR);

    ASTNode* ast_it = node->sym_expr.subscript;
    IRExpression* ir_it = expr;
    ASTSymbolSubscriptType subscript_type = node->sym_expr.subscript_type;
    IRExpression** pp_subscript = &ir_it->sym_expr.subscript;

    while (ast_it)
    {
        IRExpression* new_ir_expr = NEW(IRExpression, 1);
        *pp_subscript = new_ir_expr;
        redassert(ast_it->node_id == AST_TYPE_SYM_EXPR);
        new_ir_expr->type = IR_EXPRESSION_TYPE_SUBSCRIPT_ACCESS;
        IRSymExprType sym_type = ir_it->sym_expr.type;
        switch (sym_type)
        {
            case IR_SYM_EXPR_TYPE_PARAM:
                RED_NOT_IMPLEMENTED;
                break;
            case IR_SYM_EXPR_TYPE_SYM:
            {
                IRSymDeclStatement* sym_decl = ir_it->sym_expr.sym_decl;
                redassert(sym_decl);
                TypeKind type_kind = sym_decl->type.kind;
                new_ir_expr->subscript_access.parent.type = type_kind;
                switch (type_kind)
                {
                    case TYPE_KIND_STRUCT:
                        new_ir_expr->subscript_access.parent.struct_p = sym_decl->type.struct_type;
                        break;
                    default:
                        RED_NOT_IMPLEMENTED;
                        break;
                }

                break;
            }
            case IR_SYM_EXPR_TYPE_ENUM:
            {
                IREnumDecl* enum_decl = ir_it->sym_expr.enum_decl;
                redassert(enum_decl);
                new_ir_expr->subscript_access.parent.type = TYPE_KIND_ENUM;
                new_ir_expr->subscript_access.parent.enum_p = enum_decl;
                break;
            }
            case IR_SYM_EXPR_TYPE_ENUM_FIELD:
            {
                IREnumField* enum_field = ir_it->sym_expr.enum_field;
                redassert(enum_field);
                memset(&new_ir_expr->subscript_access.parent, 0, sizeof(new_ir_expr->subscript_access.parent));
                break;
            }
            default:
                RED_NOT_IMPLEMENTED;
                break;
        }

        new_ir_expr->subscript_access.name = ast_it->sym_expr.name;
        new_ir_expr->subscript_access.subscript = NULL;
        new_ir_expr->subscript_access.subscript_type = subscript_type;
        //subscript_type = ast_it->sym_expr.subscript_type;
        ast_it = ast_it->sym_expr.subscript;
        if (ast_it)
        {
            subscript_type = ast_it->sym_expr.subscript_type;
            ir_it = new_ir_expr;
            pp_subscript = &ir_it->subscript_access.subscript;
        }
    }
}

static inline IRExpression ast_to_ir_expression(ASTNode* node, IRModule* module, IRFunctionDefinition* parent_fn, IRLoadStoreCfg use_type)
{
    IRExpression expression = ZERO_INIT;
    if (node)
    {
        AST_ID id = node->node_id;
        switch (id)
        {
            case AST_TYPE_INT_LIT:
                expression.type = IR_EXPRESSION_TYPE_INT_LIT;
                expression.int_literal = ast_to_ir_int_lit_expr(node);
                return expression;
            case AST_TYPE_ARRAY_LIT:
                expression.type = IR_EXPRESSION_TYPE_ARRAY_LIT;
                expression.array_literal = ast_to_ir_array_lit(node, module, parent_fn);
                return expression;
            case AST_TYPE_SYM_EXPR:
                expression.type = IR_EXPRESSION_TYPE_SYM_EXPR;
                expression.sym_expr = find_symbol(node->sym_expr.name, module, parent_fn, use_type);
                
                if (node->sym_expr.subscript)
                {
                    switch (node->sym_expr.subscript_type)
                    {
                        case AST_SYMBOL_SUBSCRIPT_TYPE_FIELD_ACCESS:
                            ast_to_ir_field_use(node, &expression, parent_fn, use_type);
                            break;
                        case AST_SYMBOL_SUBSCRIPT_TYPE_ARRAY_ACCESS:
                            expression.sym_expr.subscript = NEW(IRExpression, 1);
                            expression.sym_expr.subscript->subscript_access.subscript_type = AST_SYMBOL_SUBSCRIPT_TYPE_ARRAY_ACCESS;
                            *expression.sym_expr.subscript = ast_to_ir_expression(node->sym_expr.subscript, module, parent_fn, use_type);
                            break;
                        default:
                            RED_NOT_IMPLEMENTED;
                            break;
                    }
                }

                return expression;
            case AST_TYPE_BIN_EXPR:
                expression.type = IR_EXPRESSION_TYPE_BIN_EXPR;
                expression.bin_expr = ast_to_ir_binary_expr(&node->bin_expr, module, parent_fn);
                return expression;
            default:
                RED_NOT_IMPLEMENTED;
                return (IRExpression)ZERO_INIT;
        }
    }
    else
    {
        expression.type = IR_EXPRESSION_TYPE_VOID;
        return expression;
    }
}

IRType ast_to_ir_find_expression_type(IRExpression* expression)
{
    redassert(expression);
    IRExpressionType type = expression->type;
    switch (type)
    {
        case IR_EXPRESSION_TYPE_INT_LIT:
            return primitive_types[IR_TYPE_PRIMITIVE_S32];
        case IR_EXPRESSION_TYPE_SYM_EXPR:
        {
            IRSymExpr* sym_expr = &expression->sym_expr;
            IRSymExprType sym_type = sym_expr->type;

            if (sym_expr->subscript)
            {
                switch (sym_expr->subscript->subscript_access.subscript_type)
                {
                    case AST_SYMBOL_SUBSCRIPT_TYPE_ARRAY_ACCESS:
                    {
                        switch (sym_type)
                        {
                            case IR_SYM_EXPR_TYPE_PARAM:
                                return *sym_expr->param_decl->type.array_type.base_type;
                            case IR_SYM_EXPR_TYPE_SYM:
                                return *sym_expr->sym_decl->type.array_type.base_type;
                            default:
                                RED_NOT_IMPLEMENTED;
                        }
                    }
                    case AST_SYMBOL_SUBSCRIPT_TYPE_FIELD_ACCESS:
                    {
                        RED_NOT_IMPLEMENTED;
                    }
                    default:
                        RED_NOT_IMPLEMENTED;
                }
            }
            else
            {
                switch (sym_type)
                {
                    case IR_SYM_EXPR_TYPE_PARAM:
                        return sym_expr->param_decl->type;
                    case IR_SYM_EXPR_TYPE_SYM:
                        return sym_expr->sym_decl->type;
                    default:
                        RED_NOT_IMPLEMENTED;
                }
            }
            return (const IRType)ZERO_INIT;
        }
        case IR_EXPRESSION_TYPE_BIN_EXPR:
            return ast_to_ir_find_expression_type(expression->bin_expr.left);
        default:
            RED_NOT_IMPLEMENTED;
            return (const IRType)ZERO_INIT;
    }
}

static inline IRReturnStatement ast_to_ir_return_st(ASTNode* node, IRModule* module, IRFunctionDefinition* parent_fn)
{
    redassert(node->node_id == AST_TYPE_RETURN_STATEMENT);
    IRReturnStatement ret_st = ZERO_INIT;
    ASTNode* expr_node = node->return_expr.expr;
    AST_ID expr_type = expr_node->node_id;
    IRType ret_type = parent_fn->proto.ret_type;
    // TODO: control this
    //redassert(ret_type.kind == TYPE_KIND_PRIMITIVE);
    //redassert(ret_type.primitive_type == IR_TYPE_PRIMITIVE_S32);
    switch (expr_type)
    {
        case AST_TYPE_INT_LIT:
            if (type_matches(&ret_type, expr_node))
            {
                ret_st.red_type = ret_type;
                ret_st.expression = ast_to_ir_expression(expr_node, module, parent_fn, LOAD);
                return ret_st;
            }
            else
            {
                return ret_st;
            }
        case AST_TYPE_SYM_EXPR:
        {
            IRExpression sym_expr = ast_to_ir_expression(expr_node, module, parent_fn, LOAD);
            redassert(sym_expr.type == IR_EXPRESSION_TYPE_SYM_EXPR);
            IRSymExpr result = sym_expr.sym_expr;
            if (memcmp(&(const IRSymExpr)ZERO_INIT, &result, sizeof(IRSymExpr)) == 0)
            {
                os_exit_with_message("symbol not found");
                return ret_st;
            }
            IRType red_type = ast_to_ir_find_expression_type(&sym_expr);
            if (red_type_is_invalid(&red_type))
            {
                os_exit_with_message("could not infere type");
                return ret_st;
            }
            // TODO: do this better
            //if (red_type.kind != ret_type.kind || red_type.primitive != ret_type.primitive)
            //{
            //    os_exit_with_message("type mismatch");
            //    return ret_st;
            //}
            ret_st.red_type = red_type;
            ret_st.expression = sym_expr;

            return ret_st;
        }
        case AST_TYPE_BIN_EXPR:
        {
            ret_st.expression.type = IR_EXPRESSION_TYPE_BIN_EXPR;
            ret_st.expression.bin_expr = ast_to_ir_binary_expr(&expr_node->bin_expr, module, parent_fn);
            // TODO: modify this. We now get the type of the left
            ret_st.red_type = ast_to_ir_find_expression_type(&ret_st.expression);
            return ret_st;
        }
        case AST_TYPE_FN_CALL:
        {
            ret_st.expression.type = IR_EXPRESSION_TYPE_FN_CALL_EXPR;
            ret_st.expression.fn_call_expr.name = expr_node->fn_call.name;
            // TODO: change, because we will be supporting arguments
            ret_st.expression.fn_call_expr.arg_count = expr_node->fn_call.arg_count;
            if (expr_node->fn_call.arg_count > 0)
            {
                ret_st.expression.fn_call_expr.args = NEW(IRExpression, expr_node->fn_call.arg_count);
                for (u32 i = 0; i < expr_node->fn_call.arg_count; i++)
                {
                    // TODO: LOAD is probably buggy
                    ret_st.expression.fn_call_expr.args[i] = ast_to_ir_expression(expr_node->fn_call.args[i], module, parent_fn, LOAD);
                }
            }
            else
            {
                ret_st.expression.fn_call_expr.args = NULL;
            }
            return ret_st;
        }
        default:
            RED_NOT_IMPLEMENTED;
            return ret_st;
    }
}

// TODO: sophisticate this more into the future
static inline bool is_equal_type(IRType* type1, IRType* type2)
{
    if (type1 == type2)
    {
        return true;
    }

    return memcmp(type1, type2, sizeof(IRType)) == 0;
}

static inline bool is_operation_allowed(TokenID op, IRType* type)
{
    RED_NOT_IMPLEMENTED;
    return true;
}

static inline bool is_suitable_operation(TokenID op, IRType* type1, IRType* type2)
{
    if (!is_equal_type(type1, type2))
    {
        return false;
    }

    return is_operation_allowed(op, type1);
}

static inline IRBinaryExpr ast_to_ir_binary_expr(ASTBinExpr* bin_expr, IRModule* module, IRFunctionDefinition* parent_fn)
{
    ASTNode* left = bin_expr->left;
    ASTNode* right = bin_expr->right;
    TokenID op = bin_expr->op;

    IRBinaryExpr result = ZERO_INIT;

    IRExpression ir_left = ast_to_ir_expression(left, module, parent_fn, LOAD);
    IRExpression ir_right = ast_to_ir_expression(right, module, parent_fn, LOAD);

    result.left = NEW(IRExpression, 1);
    result.right = NEW(IRExpression, 1);
    *(result.left) = ir_left;
    *(result.right) = ir_right;
    result.op = op;

    return result;
}

static inline IRSymAssignStatement ast_to_ir_assign_st(ASTBinExpr* bin_expr, IRModule* module, IRFunctionDefinition* parent_fn)
{
    IRExpression left_expr = ast_to_ir_expression(bin_expr->left, module, parent_fn, STORE);
    redassert(left_expr.type == IR_EXPRESSION_TYPE_SYM_EXPR);
    IRExpression right_expr = ast_to_ir_expression(bin_expr->right, module, parent_fn, LOAD);

    IRSymAssignStatement assign_st;
    assign_st.left = NEW(IRExpression, 1);
    *assign_st.left = left_expr;
    assign_st.right = NEW(IRExpression, 1);
    *assign_st.right = right_expr;

    return assign_st;
}

static inline IRCompoundStatement ast_to_ir_compound_st(ASTNode* node, IRFunctionDefinition* parent_fn, IRModule* module);
static inline IRBranchStatement ast_to_ir_branch_st(ASTNode* node, IRFunctionDefinition* parent_fn, IRModule* module)
{
    redassert(node->node_id == AST_TYPE_BRANCH_EXPR);

    IRBranchStatement result = ZERO_INIT;

    ASTBranchExpr* ast_branch_expr = &node->branch_expr;
    ASTNode* ast_condition_node = ast_branch_expr->condition;
    ASTNode* ast_if_block_node = ast_branch_expr->if_block;
    ASTNode* ast_else_block_node = ast_branch_expr->else_block;

    redassert(ast_condition_node);
    redassert(ast_if_block_node);
    AST_ID ast_condition_node_id = ast_condition_node->node_id;
    switch (ast_condition_node_id)
    {
        case AST_TYPE_BIN_EXPR:
        {
            ASTBinExpr* bin_expr = &ast_condition_node->bin_expr;
            result.condition.type = IR_EXPRESSION_TYPE_BIN_EXPR;
            result.condition.bin_expr = ast_to_ir_binary_expr(bin_expr, module, parent_fn);
            break;
        }
        default:
            RED_NOT_IMPLEMENTED;
            return result;
    }

    AST_ID ast_if_block_node_id = ast_if_block_node->node_id;
    switch (ast_if_block_node_id)
    {
        case AST_TYPE_COMPOUND_STATEMENT:
            result.if_block = ast_to_ir_compound_st(ast_if_block_node, parent_fn, module);
            break;
        default:
            RED_NOT_IMPLEMENTED;
            break;
    }

    if (ast_else_block_node != null)
    {
        AST_ID ast_else_block_node_id = ast_else_block_node->node_id;
        IRStatement* statement = NEW(IRStatement, 1);
        result.else_block = statement;

        switch (ast_else_block_node_id)
        {
            case AST_TYPE_COMPOUND_STATEMENT:
            {
                //result.else_block = ast_to_ir_compound_st(ast_else_block_node, parent_fn, module);
                statement->type = IR_ST_TYPE_COMPOUND_ST;
                statement->compound_st = ast_to_ir_compound_st(ast_else_block_node, parent_fn, module);
                break;
            }
            case AST_TYPE_BRANCH_EXPR:
            {
                statement->type = IR_ST_TYPE_BRANCH_ST;
                statement->branch_st = ast_to_ir_branch_st(ast_else_block_node, parent_fn, module);
                break;
            }
            default:
                RED_NOT_IMPLEMENTED;
                break;
        }
    }
    else
    {
        result.else_block = null;
    }

    return result;
}

static inline IRSymDeclStatement ast_to_ir_sym_decl_st(ASTNode* node, IRFunctionDefinition* parent_fn, IRModule* module)
{
    IRSymDeclStatement st;
    st.is_const = node->sym_decl.is_const;
    st.name = node->sym_decl.sym->sym_expr.name;
    st.value = ast_to_ir_expression(node->sym_decl.value, module, parent_fn, LOAD);
    st.type = ast_to_ir_resolve_type(node->sym_decl.type, parent_fn, module);

    return st;
}

static inline IRCompoundStatement ast_to_ir_compound_st(ASTNode* node, IRFunctionDefinition* parent_fn, IRModule* module)
{
    redassert(node->node_id == AST_TYPE_COMPOUND_STATEMENT);

    IRCompoundStatement result = ZERO_INIT;
    ASTNodeBuffer* sb = &node->compound_statement.statements;
    u32 st_count = sb->len;
    if (st_count > 0)
    {
        ir_stmtb_resize(&result.stmts, st_count);

        for (usize i = 0; i < st_count; i++)
        {
            IRStatement* st_it = ir_stmtb_add_one(&result.stmts);
            ASTNode* st_node = sb->ptr[i];
            redassert(st_node);
            AST_ID type = st_node->node_id;

            switch (type)
            {
                case AST_TYPE_BRANCH_EXPR:
                    st_it->type = IR_ST_TYPE_BRANCH_ST;
                    st_it->branch_st = ast_to_ir_branch_st(st_node, parent_fn, module);
                    break;
                case AST_TYPE_RETURN_STATEMENT:
                    st_it->type = IR_ST_TYPE_RETURN_ST;
                    st_it->return_st = ast_to_ir_return_st(st_node, module, parent_fn);
                    break;
                case AST_TYPE_SYM_DECL:
                    st_it->type = IR_ST_TYPE_SYM_DECL_ST;
                    st_it->sym_decl_st = ast_to_ir_sym_decl_st(st_node, parent_fn, module);
                    decl_append(&parent_fn->sym_declarations, st_it->sym_decl_st);
                    break;
                case AST_TYPE_BIN_EXPR:
                {
                    ASTBinExpr* ast_bin_expr = &st_node->bin_expr;
                    TokenID op = ast_bin_expr->op;

                    switch (op)
                    {
                        case TOKEN_ID_EQ:
                            st_it->type = IR_ST_TYPE_ASSIGN_ST;
                            st_it->sym_assign_st = ast_to_ir_assign_st(ast_bin_expr, module, parent_fn);
                            break;
                        default:
                            RED_NOT_IMPLEMENTED;
                            break;
                    }
                    break;
                }
                case AST_TYPE_LOOP_EXPR:
                    st_it->type = IR_ST_TYPE_LOOP_ST;
                    st_it->loop_st.condition = ast_to_ir_expression(st_node->loop_expr.condition, module, parent_fn, LOAD);
                    st_it->loop_st.body = ast_to_ir_compound_st(st_node->loop_expr.body, parent_fn, module);
                    break;
                default:
                    RED_NOT_IMPLEMENTED;
                    break;
            }
        }
    }

    return result;
}

IRFunctionPrototype ast_to_ir_fn_proto(ASTNode* node, IRModule* module)
{
    redassert(node->node_id == AST_TYPE_FN_PROTO);

    ASTFnProto* fn_proto = &node->fn_proto;
    ASTNode** param_data = fn_proto->params.ptr;
    redassert(fn_proto->params.len < UINT8_MAX);
    u8 param_count = fn_proto->params.len;
    SB* fn_name = fn_proto->sym->sym_expr.name;
    IRParamDecl* params = null;

    if (param_count > 0)
    {
        params = NEW(IRParamDecl, param_count);

        for (u8 i = 0; i < param_count; i++)
        {
            ASTNode* param = param_data[i];

            IRType red_type = ast_to_ir_resolve_type(param->param_decl.type, NULL, module);

            if (red_type_is_invalid(&red_type))
            {
                os_exit_with_message("unknown type for %s:\n", sb_ptr(param_name(param)));
            }

            params[i].type = red_type;

            if (!param_name_unique(params, i, param_name(param)))
            {
                os_exit_with_message("param name %s already used\n", sb_ptr(param_name(param)));
            }

            params[i].name = param_name(param);
        }
    }

    IRType ret_red_type = ZERO_INIT;

    if (fn_proto->ret_type)
    {
        ret_red_type = ast_to_ir_resolve_type(fn_proto->ret_type, NULL, module);
        if (red_type_is_invalid(&ret_red_type))
        {
            os_exit_with_message("Unknown type for return type in function %s\n", sb_ptr(fn_name));
        }
    }
    else
    {
        ret_red_type = (const IRType)
        {
            .kind = TYPE_KIND_VOID,
        };
    }

    IRFunctionPrototype ir_proto =
    {
        .name = fn_name,
        .param_count = param_count,
        .params = params,
        .ret_type = ret_red_type,
    };

    return ir_proto;
}

static inline IRFunctionDefinition* ir_find_fn_definition(IRFunctionDefinitionBuffer* fn_definitions, SB* fn_name);

static inline void ast_to_ir_fn_definitions(IRModule* ir_module, ASTNodeBuffer* fb)
{
    for (usize i = 0; i < fb->len; i++)
    {
        ASTNode* fn_def_node = fb->ptr[i];
        ASTNode* fn_proto_node = fn_def_node->fn_def.proto;
        ASTNode* fn_body_node = fn_def_node->fn_def.body;
        SB* fn_name = fn_proto_node->fn_proto.sym->sym_expr.name;

        if (ir_find_fn_definition(&ir_module->fn_definitions, fn_name))
        {
            os_exit_with_message("Function %s has already been defined\n", sb_ptr(fn_name));
        }
        IRFunctionDefinition* fn_def = ir_fn_def_add_one(&ir_module->fn_definitions);
        fn_def->proto = ast_to_ir_fn_proto(fn_proto_node, ir_module);
        fn_def->body = ast_to_ir_compound_st(fn_body_node, fn_def, ir_module);
    }
}

static inline IRFunctionDefinition* ir_find_fn_definition(IRFunctionDefinitionBuffer* fn_definitions, SB* fn_name)
{
    u32 fn_definition_count = fn_definitions->len;
    for (s32 i = 0; i < fn_definition_count; i++)
    {
        IRFunctionDefinition* it = &fn_definitions->ptr[i];
        SB* it_fn_name = it->proto.name;
        if (sb_cmp(fn_name, it_fn_name))
        {
            return it;
        }
    }

    return null;
}

static inline void print_fn_cfg(IRFunctionConfig* fn_cfg)
{
    print("Function linkage: %s\n", fn_cfg->link_type == EXTERN ? "extern" : "intern");
}

static inline void print_param_decl(IRParamDecl* param)
{
    redassert(param->type.kind == TYPE_KIND_PRIMITIVE);
    print("Param %s, type: %s\n", sb_ptr(param->name), primitive_type_str(param->type.primitive_type));
}

static inline void print_fn_proto(IRFunctionPrototype* fn_proto)
{
    print("Function name: %s\n", sb_ptr(fn_proto->name));
    print_fn_cfg(&fn_proto->fn_cfg);
    u8 param_count = fn_proto->param_count;
    print("(\n");
    if (param_count > 0)
    {
        for (u32 i = 0; i < param_count; i++)
        {
            print_param_decl(&fn_proto->params[i]);
        }
    }
    print(")\n");

    if (fn_proto->ret_type.kind != TYPE_KIND_VOID)
    {
        print("Return type: %s\n", primitive_type_str(fn_proto->ret_type.primitive_type));
    }
    else
    {
        print("Return type: void\n");
    }
}

static inline void print_branch_st(IRBranchStatement* branch_st);
static inline void print_return_st(IRReturnStatement* return_st);
static inline void print_compount_st(IRCompoundStatement* compound_st)
{
    print("{\n");
    IRStatement* statement_it = compound_st->stmts.ptr;
    u32 statement_count = compound_st->stmts.len;
    if (statement_count > 0)
    {
        for (u32 i = 0; i < statement_count; i++)
        {
            IRStatement* st = &statement_it[i];
            IRStatementType st_type = st->type;
            switch (st_type)
            {
                case IR_ST_TYPE_COMPOUND_ST:
                    print_compount_st(&st->compound_st);
                    break;
                case IR_ST_TYPE_RETURN_ST:
                    print_return_st(&st->return_st);
                    break;
                case IR_ST_TYPE_BRANCH_ST:
                    print_branch_st(&st->branch_st);
                    break;
                default:
                    RED_NOT_IMPLEMENTED;
                    break;
            }
        }
    }

    print("}\n");
}

static inline void print_expression(IRExpression* expr);

static inline void print_binary_expr(IRBinaryExpr* bin_expr)
{
    print_expression(bin_expr->left);
    print("Operator: %s\n", token_name(bin_expr->op));
    print_expression(bin_expr->right);
}

static inline void print_param_expr(IRParamDecl* param)
{
    redassert(param->type.kind == TYPE_KIND_PRIMITIVE);
    print("Param name: %s; param type: %s\n", sb_ptr(param->name), primitive_type_str(param->type.primitive_type));
}

static inline void print_sym_expr(IRSymExpr* sym_expr)
{
    IRSymExprType type = sym_expr->type;
    switch (type)
    {
        case IR_SYM_EXPR_TYPE_PARAM:
            print_param_expr(sym_expr->param_decl);
            break;
        case IR_SYM_EXPR_TYPE_SYM:
            RED_NOT_IMPLEMENTED;
            break;
        default:
            RED_NOT_IMPLEMENTED;
            break;
    }
}

static inline void print_int_literal(IRIntLiteral* int_lit)
{
    redassert(int_lit->bigint.digit_count == 1);
    bool is_negative = int_lit->bigint.is_negative;
    if (is_negative)
    {
        print("Int lit: %lld\n", -(s64)(int_lit->bigint.digit));
    }
    else
    {
        print("Int lit: %llu\n", int_lit->bigint.digit);
    }
}

static inline void print_expression(IRExpression* expr)
{
    IRExpressionType type = expr->type;
    switch (type)
    {
        case IR_EXPRESSION_TYPE_BIN_EXPR:
            print_binary_expr(&expr->bin_expr);
            break;
        case IR_EXPRESSION_TYPE_SYM_EXPR:
            print_sym_expr(&expr->sym_expr);
            break;
        case IR_EXPRESSION_TYPE_INT_LIT:
            print_int_literal(&expr->int_literal);
            break;
        default:
            RED_NOT_IMPLEMENTED;
            break;
    }
}
static inline void print_branch_st(IRBranchStatement* branch_st)
{
    print("if\n(\n");
    print_expression(&branch_st->condition);
    print(")\n");
    print_compount_st(&branch_st->if_block);
    if (branch_st)
    {
        print("else\n");
        switch (branch_st->else_block->type)
        {
            case IR_ST_TYPE_COMPOUND_ST:
                print_compount_st(&branch_st->else_block->compound_st);
            default:
                RED_NOT_IMPLEMENTED;
                break;
        }

    }
}
static inline void print_return_st(IRReturnStatement* return_st)
{
    print("Return statement: ");
    print_expression(&return_st->expression);
}

static inline void print_fn_body(IRFunctionDefinition* fn)
{
    print_compount_st(&fn->body);
}

static inline void print_fn_definition(IRFunctionDefinition* fn)
{
    print("Function definition:\n");
    print_fn_proto(&fn->proto);
    print_fn_body(fn);
}

void print_ir_tree(IRModule* tree)
{
    IRFunctionDefinitionBuffer* fb = &tree->fn_definitions;
    u32 fn_count = tree->fn_definitions.len;
    for (u32 i = 0; i < fn_count; i++)
    {
        IRFunctionDefinition* fn = &fb->ptr[i];
        print_fn_definition(fn);
    }
}

static inline bool field_name_unique(ASTNode* node, ASTNode* parent_container)
{
    u32 instance_count = 0;
    AST_ID type = parent_container->node_id;
    switch (type)
    {
        case AST_TYPE_STRUCT_DECL:
        {
            ASTStructDecl* struct_decl = &parent_container->struct_decl;
            ASTNode** field_ptr = struct_decl->fields.ptr;
            u32 field_count = struct_decl->fields.len;

            for (u32 i = 0; i < field_count && instance_count < 2; i++)
            {
                ASTNode* field = field_ptr[i];
                redassert(field->field_decl.sym->node_id == AST_TYPE_SYM_EXPR);
                if (sb_cmp(field->field_decl.sym->sym_expr.name, node->field_decl.sym->sym_expr.name))
                {
                    instance_count++;
                }
            }
            break;
        }
        case AST_TYPE_UNION_DECL:
        case AST_TYPE_ENUM_DECL:
        default:
            RED_NOT_IMPLEMENTED;
            break;
    }

    return instance_count == 1;
}

static inline IRFieldDecl ast_to_ir_field_decl(ASTNode* node, ASTNode* parent_container, IRModule* module)
{
    redassert(node->node_id == AST_TYPE_FIELD_DECL);
    ASTFieldDecl* field_decl = &node->field_decl;
    IRFieldDecl ir_field = ZERO_INIT;
    ir_field.type = ast_to_ir_resolve_type(field_decl->type, NULL, module);
    redassert(ir_field.type.kind == TYPE_KIND_PRIMITIVE);
    ir_field.name = node->field_decl.sym->sym_expr.name;

    if (red_type_is_invalid(&ir_field.type))
    {
        os_exit_with_message("unknown type for %s\n", sb_ptr(ir_field.name));
    }

    if (!field_name_unique(node, parent_container))
    {
        os_exit_with_message("field name %s already used\n", sb_ptr(ir_field.name));
    }

    return ir_field;
}

static inline IRStructDecl ast_to_ir_struct_decl(ASTNode* node, IRModule* module)
{
    IRStructDecl ir_struct = ZERO_INIT;
    redassert(node->node_id == AST_TYPE_STRUCT_DECL);
    ASTStructDecl* struct_decl = &node->struct_decl;
    ir_struct.name = struct_decl->name;
    u32 field_count = struct_decl->fields.len;
    redassert(field_count > 0);
    if (field_count > 0)
    {
        ir_struct.fields = NEW(IRFieldDecl, field_count);
        ASTNode** field_ptr = struct_decl->fields.ptr;

        for (u32 i = 0; i < field_count; i++)
        {
            ASTNode* field = field_ptr[i];
            ir_struct.fields[i] = ast_to_ir_field_decl(field, node, module);
        }
        ir_struct.field_count = field_count;
    }

    return ir_struct;
}

static inline void ast_to_ir_union_decl(ASTNode* node)
{
    RED_NOT_IMPLEMENTED;
}

static inline bool primitive_type_is_signed(IRTypePrimitive primitive_type)
{
    redassert(!(primitive_type > IR_TYPE_PRIMITIVE_S64));
    return primitive_type >= IR_TYPE_PRIMITIVE_S8 && primitive_type <= IR_TYPE_PRIMITIVE_S64;
}

static inline void ast_to_ir_enum_decl(IREnumDecl* enum_decl, IRModule* module, ASTNode* node)
{
    AST_ID id = node->node_id;
    redassert(id == AST_TYPE_ENUM_DECL);
    enum_decl->name = node->enum_decl.name;
    IRTypePrimitive primitive_type = (IRTypePrimitive)node->enum_decl.type;
    enum_decl->type = primitive_types[primitive_type];
    //bool is_signed = primitive_type_is_signed(primitive_type);
    
    u32 field_count = node->enum_decl.fields.len;
    if (field_count > 0)
    {
        ASTNode** field_ptr = node->enum_decl.fields.ptr;
        for (u32 i = 0; i < field_count; i++)
        {
            ASTNode* field = field_ptr[i];
            ASTEnumField* enum_field = &field->enum_field;
            SB* enum_field_name = enum_field->name;
            IREnumField ir_field;
            ir_field.name = enum_field_name;
            ir_field.parent = enum_decl;
            u64 value;

            bool is_negative = false;
            if (enum_field->field_value)
            {
                IRExpression expr = ast_to_ir_expression(enum_field->field_value, module, NULL, LOAD);
                redassert(expr.type == IR_EXPRESSION_TYPE_INT_LIT);
                is_negative = expr.int_literal.bigint.is_negative;
                redassert(expr.int_literal.bigint.digit_count == 1);

                value = expr.int_literal.bigint.digit;
            }
            else
            {
                value = i;
            }

            switch (primitive_type)
            {
                case IR_TYPE_PRIMITIVE_U8:
                    RED_NOT_IMPLEMENTED;
                    break;
                case IR_TYPE_PRIMITIVE_U16:
                    RED_NOT_IMPLEMENTED;
                    break;
                case IR_TYPE_PRIMITIVE_U32:
                    redassert(!is_negative);
                    ir_field.value.unsigned32 = (u32)value;
                    break;
                case IR_TYPE_PRIMITIVE_U64:
                    RED_NOT_IMPLEMENTED;
                    break;
                case IR_TYPE_PRIMITIVE_S8:
                    RED_NOT_IMPLEMENTED;
                    break;
                case IR_TYPE_PRIMITIVE_S16:
                    RED_NOT_IMPLEMENTED;
                    break;
                case IR_TYPE_PRIMITIVE_S32:
                    RED_NOT_IMPLEMENTED;
                    break;
                case IR_TYPE_PRIMITIVE_S64:
                    RED_NOT_IMPLEMENTED;
                    break;
                default:
                    RED_NOT_IMPLEMENTED;
                    break;
            }

            ir_enum_field_append(&enum_decl->fields, ir_field);
        }
    }
}

static inline void ast_to_ir_type_declarations(IRModule* ir_tree, RedAST* ast)
{
    ASTNodeBuffer* struct_decls = &ast->struct_decls;
    u64 struct_count = struct_decls->len;
    ASTNode** struct_decl_ptr = struct_decls->ptr;
    for (u64 i = 0; i < struct_count; i++)
    {
        ASTNode* struct_node = struct_decl_ptr[i];
        ir_struct_append(&ir_tree->struct_decls, ast_to_ir_struct_decl(struct_node, ir_tree));
    }

    ASTNodeBuffer* union_decls = &ast->union_decls;
    u64 union_count = union_decls->len;
    ASTNode** union_decl_ptr = union_decls->ptr;
    for (u64 i = 0; i < union_count; i++)
    {
        ASTNode* union_node = union_decl_ptr[i];
        ast_to_ir_union_decl(union_node);
    }

    ASTNodeBuffer* enum_decls = &ast->enum_decls;
    u64 enum_count = enum_decls->len;
    ASTNode** enum_decl_ptr = enum_decls->ptr;
    for (u64 i = 0; i < enum_count; i++)
    {
        ASTNode* enum_node = enum_decl_ptr[i];
        IREnumDecl enum_decl = ZERO_INIT;
        ast_to_ir_enum_decl(&enum_decl, ir_tree, enum_node);
        ir_enum_append(&ir_tree->enum_decls, enum_decl);
    }
}

IRModule transform_ast_to_ir(RedAST* ast)
{
    IRModule ir_tree = ZERO_INIT;
    ast_to_ir_type_declarations(&ir_tree, ast);
    ast_to_ir_fn_definitions(&ir_tree, &ast->fn_definitions);

#if RED_IR_VERBOSE
    print_ir_tree(&ir_tree);
#endif

    return ir_tree;
}
