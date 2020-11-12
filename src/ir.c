#include "compiler_types.h"
#include "ir.h"
#include "os.h"

const char* primitive_types_str[] =
{
    "u8", "u16", "u32", "u64",
    "s8", "s16", "s32", "s64",
    "f32", "f64", "f128",
};

static const RedType primitive_types[] = {
    [0] =
    {
        .kind = PRIMITIVE,
        .size = 1,
        .primitive = RED_TYPE_PRIMITIVE_U8,
    },
    [1] =
    {
        .kind = PRIMITIVE,
        .size = 2,
        .primitive = RED_TYPE_PRIMITIVE_U16,
    },
    [2] =
    {
        .kind = PRIMITIVE,
        .size = 4,
        .primitive = RED_TYPE_PRIMITIVE_U32,
    },
    [3] =
    {
        .kind = PRIMITIVE,
        .size = 8,
        .primitive = RED_TYPE_PRIMITIVE_U64,
    },
    [4] =
    {
        .kind = PRIMITIVE,
        .size = 1,
        .primitive = RED_TYPE_PRIMITIVE_S8,
    },
    [5] =
    {
        .kind = PRIMITIVE,
        .size = 2,
        .primitive = RED_TYPE_PRIMITIVE_S16,
    },
    [6] =
    {
        .kind = PRIMITIVE,
        .size = 4,
        .primitive = RED_TYPE_PRIMITIVE_S32,
    },
    [7] =
    {
        .kind = PRIMITIVE,
        .size = 8,
        .primitive = RED_TYPE_PRIMITIVE_S64,
    },
    [8] =
    {
        .kind = PRIMITIVE,
        .size = 4,
        .primitive = RED_TYPE_PRIMITIVE_F32,
    },
    [9] =
    {
        .kind = PRIMITIVE,
        .size = 8,
        .primitive = RED_TYPE_PRIMITIVE_F64,
    },
    [10] =
    {
        .kind = PRIMITIVE,
        .size = 16,
        .primitive = RED_TYPE_PRIMITIVE_F128,
    },
};

RedType* resolve_type(Node* node)
{
    redassert(node->node_id == AST_TYPE_TYPE_EXPR);
    char* type_name = node->type_expr.type_name->ptr;

    static_assert(array_length(primitive_types) == array_length(primitive_types_str), "Error in primitive type count");
    for (s32 i = 0; i < array_length(primitive_types); i++)
    {
        if (strcmp(type_name, primitive_types_str[i]) == 0)
        {
            return (RedType*)&primitive_types[i];
        }
    }

    return null;
}



GEN_BUFFER_FUNCTIONS(ir_stmtb, sb, IRStatementBuffer, IRStatement)
GEN_BUFFER_FUNCTIONS(ir_fn_def, fb, IRFunctionDefinitionBuffer, IRFunctionDefinition)

static inline SB* param_name(Node* node)
{
    redassert(node->node_id == AST_TYPE_PARAM_DECL);
    return node->param_decl.sym->sym_expr.name;
}

static inline SB* param_type(Node* node)
{
    redassert(node->node_id == AST_TYPE_PARAM_DECL);
    return node->param_decl.type->type_expr.type_name;
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

static inline bool type_matches_primitive(RedType* red_type, Node* node)
{
    redassert(red_type->kind == PRIMITIVE);

    bool matches = false;
    RedTypePrimitive primitive_type = red_type->primitive;
    switch (primitive_type)
    {
        case RED_TYPE_PRIMITIVE_S32:
            matches = node->node_id == AST_TYPE_INT_LIT;
            break;
        default:
            RED_NOT_IMPLEMENTED;
            break;
    }
    return matches;
}
static inline bool type_matches(RedType* red_type, Node* node)
{
    switch (red_type->kind)
    {
        case PRIMITIVE:
            return type_matches_primitive(red_type, node);
        case FUNCTION:
            RED_NOT_IMPLEMENTED;
            return false;
    }
}

static inline IRIntLiteral ast_to_ir_int_lit_expr(Node* node)
{
    IRIntLiteral lit;
    redassert(node->node_id == AST_TYPE_INT_LIT);

    lit.bigint = *node->int_lit.bigint;
    return lit;
}

static inline IRExpression ast_to_ir_expression(Node* node)
{
    ASTType id = node->node_id;
    IRExpression expression;
    switch (id)
    {
        case AST_TYPE_INT_LIT:
            expression.type = IR_EXPRESSION_TYPE_INT_LIT;
            expression.int_literal = ast_to_ir_int_lit_expr(node);
            return expression;
        case AST_TYPE_SYM_EXPR:
            //expression.type = IR_EXPRESSION_TYPE_SYM_EXPR;
            //expression.sym_expr.type = IR_SYM_EXPR_TYPE_PARAM;
            //expression.sym_expr.param_decl.
            RED_NOT_IMPLEMENTED;
        default:
            RED_NOT_IMPLEMENTED;
            return (IRExpression)ZERO_INIT;
    }
}

static inline IRSymExpr find_symbol(SB* symbol, IRFunctionPrototype* proto)
{
    u8 param_count = proto->param_count;
    for (usize i = 0; i < param_count; i++)
    {
        SB* name_it = proto->params[i].name;
        if (sb_cmp(name_it, symbol))
        {
            IRSymExpr result;
            result.type = IR_SYM_EXPR_TYPE_PARAM;
            result.param_decl = &proto->params[i];
            return result;
        }
    }

    return (const IRSymExpr)ZERO_INIT;
}

static inline IRReturnStatement ast_to_ir_return_st(Node* node, IRFunctionDefinition* parent_fn)
{
    redassert(node->node_id == AST_TYPE_RETURN_STATEMENT);
    IRReturnStatement ret_st;
    Node* expr_node = node->return_expr.expr;
    ASTType expr_type = expr_node->node_id;
    RedType* ret_type = parent_fn->proto.ret_type;
    redassert(ret_type->kind == PRIMITIVE);
    redassert(ret_type->primitive == RED_TYPE_PRIMITIVE_S32);
    switch (expr_type)
    {
        case AST_TYPE_INT_LIT:
            if (type_matches(ret_type, expr_node))
            {
                ret_st.red_type = *ret_type;
                ret_st.expression = ast_to_ir_expression(expr_node);
                return ret_st;
            }
        case AST_TYPE_SYM_EXPR:
        {
            SB* sym_name = expr_node->sym_expr.name;
            IRSymExpr result = find_symbol(sym_name, &parent_fn->proto);
            redassert(result.type == IR_SYM_EXPR_TYPE_PARAM);
            if (memcmp(&(const IRSymExpr)ZERO_INIT, &result, sizeof(IRSymExpr)) == 0)
            {
                os_exit_with_message("symbol not found");
            }
            RedType* red_type = &result.param_decl->type;
            if (!red_type)
            {
                os_exit_with_message("could not infere type");
                return (IRReturnStatement)ZERO_INIT;
            }
            redassert(red_type->kind == PRIMITIVE);
            if (red_type->kind != ret_type->kind || red_type->primitive != ret_type->primitive)
            {
                os_exit_with_message("type mismatch");
            }
            ret_st.red_type = *red_type;
            ret_st.expression.type = IR_EXPRESSION_TYPE_SYM_EXPR;
            ret_st.expression.sym_expr = result;

            return ret_st;
        }
        default:
            RED_NOT_IMPLEMENTED;
            
            return (IRReturnStatement)ZERO_INIT;
    }
}

static inline IRCompoundStatement ast_to_ir_compount_st(Node* node, IRFunctionDefinition* parent_fn)
{
    redassert(node->node_id == AST_TYPE_COMPOUND_STATEMENT);

    IRCompoundStatement result = ZERO_INIT;
    NodeBuffer* sb = &node->compound_statement.statements;
    u32 st_count = sb->len;
    if (st_count > 0)
    {
        ir_stmtb_resize(&result.stmts, st_count);

        for (usize i = 0; i < st_count; i++)
        {
            IRStatement* st_it = ir_stmtb_add_one(&result.stmts);
            Node* st_node = sb->ptr[i];
            redassert(st_node);
            ASTType type = st_node->node_id;

            switch (type)
            {
                case AST_TYPE_RETURN_STATEMENT:
                    st_it->type = IR_ST_TYPE_RETURN_ST;
                    st_it->return_st = ast_to_ir_return_st(st_node, parent_fn);
                    break;
                default:
                    RED_NOT_IMPLEMENTED;
                    break;
            }
        }
    }

    return result;
}

IRFunctionPrototype ast_to_ir_fn_proto(Node* node)
{
    redassert(node->node_id == AST_TYPE_FN_PROTO);

    FnProto* fn_proto = &node->fn_proto;
    Node** param_data = fn_proto->params.ptr;
    redassert(fn_proto->params.len < UINT8_MAX);
    u8 param_count = fn_proto->params.len;
    RedType** param_types = NULL;
    SB* fn_name = fn_proto->sym->sym_expr.name;
    IRParamDecl* params = null;

    if (param_count > 0)
    {
        params = NEW(IRParamDecl, param_count);

        for (u8 i = 0; i < param_count; i++)
        {
            Node* param = param_data[i];

            RedType* red_type = resolve_type(param->param_decl.type);

            if (!red_type)
            {
                os_exit_with_message("unknown type for %s: %s\n", sb_ptr(param_name(param)), sb_ptr(param_type(param)));
            }

            params[i].type = *red_type;

            if (!param_name_unique(params, i, param_name(param)))
            {
                os_exit_with_message("param name %s already used\n", sb_ptr(param_name(param)));
            }

            params[i].name = param_name(param);
        }
    }

    RedType* ret_red_type = NULL;

    if (fn_proto->ret_type)
    {
        ret_red_type = resolve_type(fn_proto->ret_type);
        if (!ret_red_type)
        {
            os_exit_with_message("Unknown type for return type in function %s\n", sb_ptr(fn_name));
        }
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

static inline void ast_to_ir_fn_definitions(RedModuleIR* ir_module, NodeBuffer* fb)
{
    for (usize i = 0; i < fb->len; i++)
    {
        Node* fn_def_node = fb->ptr[i];
        Node* fn_proto_node = fn_def_node->fn_def.proto;
        Node* fn_body_node = fn_def_node->fn_def.body;
        SB* fn_name = fn_proto_node->fn_proto.sym->sym_expr.name;

        if (ir_find_fn_definition(&ir_module->fn_definitions, fn_name))
        {
            os_exit_with_message("Function %s has already been defined\n", sb_ptr(fn_name));
        }
        IRFunctionDefinition* fn_def = ir_fn_def_add_one(&ir_module->fn_definitions);
        fn_def->proto = ast_to_ir_fn_proto(fn_proto_node);
        fn_def->body = ast_to_ir_compount_st(fn_body_node, fn_def);
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

RedModuleIR transform_ast_to_ir(RedModuleTree* ast)
{
    RedModuleIR ir_tree = ZERO_INIT;
    ast_to_ir_fn_definitions(&ir_tree, &ast->fn_definitions);

    return ir_tree;
}
