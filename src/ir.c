#include "compiler_types.h"
#include "ir.h"
#include "os.h"
#include "lexer.h"

GEN_BUFFER_FUNCTIONS(decl, db, IRSymDeclStatementBuffer, IRSymDeclStatement)

const char* primitive_types_str[] =
{
    "u8", "u16", "u32", "u64",
    "s8", "s16", "s32", "s64",
    "f32", "f64", "f128",
};

static inline IRExpression ast_to_ir_expression(Node* node, IRFunctionDefinition* parent_fn, LoadStoreCfg use_type);
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

static inline RedType resolve_type(Node* node, IRFunctionDefinition* parent_fn);
static inline bool red_type_is_invalid(RedType* type)
{
    return memcmp(type, &(const RedType)ZERO_INIT, sizeof(RedType)) == 0;
}

static inline RedType resolve_basic_type(Node* node)
{
    redassert(node->type_expr.kind == PRIMITIVE);
    char* type_name = sb_ptr(node->type_expr.basic.name);

    static_assert(array_length(primitive_types) == array_length(primitive_types_str), "Error in primitive type count");
    for (s32 i = 0; i < array_length(primitive_types); i++)
    {
        if (strcmp(type_name, primitive_types_str[i]) == 0)
        {
            return primitive_types[i];
        }
    }

    return (const RedType)ZERO_INIT;
}

static inline RedType resolve_array_type(Node* node, IRFunctionDefinition* parent_fn)
{
    ArrayType* array_type = &node->type_expr.array;
    redassert(array_type->type->node_id == AST_TYPE_TYPE_EXPR);
    RedType* base_type = NEW(RedType, 1);
    IRExpression* elem_count_expr = NEW(IRExpression, 1);
    *base_type = resolve_type(array_type->type, parent_fn);
    *elem_count_expr = ast_to_ir_expression(array_type->element_count_expr, parent_fn, LOAD);

    RedType type;
    type.kind = ARRAY;
    type.size = 0;
    type.array.type = base_type;
    type.array.elem_count_expr = elem_count_expr;
    return type;
}

static inline RedType resolve_type(Node* node, IRFunctionDefinition* parent_fn)
{
    redassert(node->node_id == AST_TYPE_TYPE_EXPR);
    TypeKind type_kind = node->type_expr.kind;
    switch (type_kind)
    {
        case PRIMITIVE:
            return resolve_basic_type(node);
        case ARRAY:
            return resolve_array_type(node, parent_fn);
        default:
            RED_NOT_IMPLEMENTED;
            return (const RedType)ZERO_INIT;
    }
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
    redassert(node->param_decl.type->type_expr.kind == PRIMITIVE);
    return node->param_decl.type->type_expr.basic.name;
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
    TypeKind kind = red_type->kind;
    switch (kind)
    {
        case INVALID:
            RED_UNREACHABLE;
            return false;
        case PRIMITIVE:
            return type_matches_primitive(red_type, node);
        case FUNCTION:
            RED_NOT_IMPLEMENTED;
            return false;
        case VOID:
            RED_NOT_IMPLEMENTED;
            return false;
        default:
            RED_NOT_IMPLEMENTED;
            return false;
    }
}

static inline IRBinaryExpr ast_to_ir_binary_expr(BinExpr* bin_expr, IRFunctionDefinition* parent_fn);
static inline IRIntLiteral ast_to_ir_int_lit_expr(Node* node)
{
    IRIntLiteral lit;
    redassert(node->node_id == AST_TYPE_INT_LIT);

    lit.bigint = *node->int_lit.bigint;
    return lit;
}

static inline IRSymExpr find_symbol(SB* symbol, IRFunctionDefinition* fn_definition, LoadStoreCfg use_type)
{
    IRFunctionPrototype* proto = &fn_definition->proto;
    u8 param_count = proto->param_count;
    for (usize i = 0; i < param_count; i++)
    {
        SB* name_it = proto->params[i].name;
        if (sb_cmp(name_it, symbol))
        {
            IRSymExpr result;
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
            IRSymExpr result;
            result.type = IR_SYM_EXPR_TYPE_SYM;
            result.sym_decl = decl;
            result.use_type = use_type;
            return result;
        }
    }
    
    // Look for local variables
    RED_NOT_IMPLEMENTED;

    return (const IRSymExpr)ZERO_INIT;
}

static inline IRArrayLiteral ast_to_ir_array_lit(Node* node, IRFunctionDefinition* parent_fn) 
{
    redassert(node->node_id == AST_TYPE_ARRAY_LIT);
    IRArrayLiteral array_lit = ZERO_INIT;
    u64 lit_count = node->array_lit.values.len;
    if (lit_count > 0)
    {
        array_lit.expressions = NEW(IRExpression, lit_count);
        array_lit.expression_count = lit_count;
        Node** lit_arr = node->array_lit.values.ptr;
        for (u64 i = 0; i < lit_count; i++)
        {
            Node* lit = lit_arr[i];
            array_lit.expressions[i] = ast_to_ir_expression(lit, parent_fn, LOAD);
        }
    }

    return array_lit;
}

static inline IRExpression ast_to_ir_expression(Node* node, IRFunctionDefinition* parent_fn, LoadStoreCfg use_type)
{
    IRExpression expression = ZERO_INIT;
    if (node)
    {
        ASTType id = node->node_id;
        switch (id)
        {
            case AST_TYPE_INT_LIT:
                expression.type = IR_EXPRESSION_TYPE_INT_LIT;
                expression.int_literal = ast_to_ir_int_lit_expr(node);
                return expression;
            case AST_TYPE_ARRAY_LIT:
                expression.type = IR_EXPRESSION_TYPE_ARRAY_LIT;
                expression.array_literal = ast_to_ir_array_lit(node, parent_fn);
                return expression;
            case AST_TYPE_SYM_EXPR:
                expression.type = IR_EXPRESSION_TYPE_SYM_EXPR;
                expression.sym_expr = find_symbol(node->sym_expr.name, parent_fn, use_type);
                if (!node->sym_expr.index_node_if_array_expr)
                {
                    expression.sym_expr.index_expr_if_array_suffix = NULL;
                }
                else
                {
                    expression.sym_expr.index_expr_if_array_suffix = NEW(IRExpression, 1);
                    *expression.sym_expr.index_expr_if_array_suffix = ast_to_ir_expression(node->sym_expr.index_node_if_array_expr, parent_fn, use_type);
                }
                return expression;
            case AST_TYPE_BIN_EXPR:
                expression.type = IR_EXPRESSION_TYPE_BIN_EXPR;
                expression.bin_expr = ast_to_ir_binary_expr(&node->bin_expr, parent_fn);
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

RedType ast_to_ir_find_expression_type(IRExpression* expression)
{
    redassert(expression);
    IRExpressionType type = expression->type;
    switch (type)
    {
        case IR_EXPRESSION_TYPE_INT_LIT:
            return primitive_types[RED_TYPE_PRIMITIVE_S32];
        case IR_EXPRESSION_TYPE_SYM_EXPR:
        {
            IRSymExpr* sym_expr = &expression->sym_expr;
            IRSymExprType sym_type = sym_expr->type;
            switch (sym_type)
            {
                case IR_SYM_EXPR_TYPE_PARAM:
                    return sym_expr->index_expr_if_array_suffix ? *sym_expr->param_decl->type.array.type : sym_expr->param_decl->type;
                case IR_SYM_EXPR_TYPE_SYM:
                    return sym_expr->index_expr_if_array_suffix ? *sym_expr->sym_decl->type.array.type : sym_expr->sym_decl->type;
                default:
                    RED_NOT_IMPLEMENTED;
                    break;
            }
        }
        case IR_EXPRESSION_TYPE_BIN_EXPR:
            return ast_to_ir_find_expression_type(expression->bin_expr.left);
        default:
            RED_NOT_IMPLEMENTED;
            return (const RedType)ZERO_INIT;
    }
}

static inline IRReturnStatement ast_to_ir_return_st(Node* node, IRFunctionDefinition* parent_fn)
{
    redassert(node->node_id == AST_TYPE_RETURN_STATEMENT);
    IRReturnStatement ret_st = ZERO_INIT;
    Node* expr_node = node->return_expr.expr;
    ASTType expr_type = expr_node->node_id;
    RedType ret_type = parent_fn->proto.ret_type;
    redassert(ret_type.kind == PRIMITIVE);
    redassert(ret_type.primitive == RED_TYPE_PRIMITIVE_S32);
    switch (expr_type)
    {
        case AST_TYPE_INT_LIT:
            if (type_matches(&ret_type, expr_node))
            {
                ret_st.red_type = ret_type;
                ret_st.expression = ast_to_ir_expression(expr_node, parent_fn, LOAD);
                return ret_st;
            }
            else
            {
                return ret_st;
            }
        case AST_TYPE_SYM_EXPR:
        {
            IRExpression sym_expr = ast_to_ir_expression(expr_node, parent_fn, LOAD);
            redassert(sym_expr.type == IR_EXPRESSION_TYPE_SYM_EXPR);
            IRSymExpr result = sym_expr.sym_expr;
            if (memcmp(&(const IRSymExpr)ZERO_INIT, &result, sizeof(IRSymExpr)) == 0)
            {
                os_exit_with_message("symbol not found");
                return ret_st;
            }
            RedType red_type = ast_to_ir_find_expression_type(&sym_expr);
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
            ret_st.expression.type = IR_EXPRESSION_TYPE_SYM_EXPR;
            ret_st.expression.sym_expr = result;

            return ret_st;
        }
        case AST_TYPE_BIN_EXPR:
        {
            ret_st.expression.type = IR_EXPRESSION_TYPE_BIN_EXPR;
            ret_st.expression.bin_expr = ast_to_ir_binary_expr(&expr_node->bin_expr, parent_fn);
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
                    ret_st.expression.fn_call_expr.args[i] = ast_to_ir_expression(expr_node->fn_call.args[i], parent_fn, LOAD);
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
static inline bool is_equal_type(RedType* type1, RedType* type2)
{
    if (type1 == type2)
    {
        return true;
    }

    return memcmp(type1, type2, sizeof(RedType)) == 0;
}

static inline bool is_operation_allowed(TokenID op, RedType* type)
{
    RED_NOT_IMPLEMENTED;
    return true;
}

static inline bool is_suitable_operation(TokenID op, RedType* type1, RedType* type2)
{
    if (!is_equal_type(type1, type2))
    {
        return false;
    }

    return is_operation_allowed(op, type1);
}

static inline IRBinaryExpr ast_to_ir_binary_expr(BinExpr* bin_expr, IRFunctionDefinition* parent_fn)
{
    Node* left = bin_expr->left;
    Node* right = bin_expr->right;
    TokenID op = bin_expr->op;

    IRBinaryExpr result = ZERO_INIT;

    IRExpression ir_left = ast_to_ir_expression(left, parent_fn, LOAD);
    IRExpression ir_right = ast_to_ir_expression(right, parent_fn, LOAD);

    result.left = NEW(IRExpression, 1);
    result.right = NEW(IRExpression, 1);
    *(result.left) = ir_left;
    *(result.right) = ir_right;
    result.op = op;

    return result;
}

static inline IRSymAssignStatement ast_to_ir_assign_st(BinExpr* bin_expr, IRFunctionDefinition* parent_fn)
{
    IRExpression left_expr = ast_to_ir_expression(bin_expr->left, parent_fn, STORE);
    redassert(left_expr.type == IR_EXPRESSION_TYPE_SYM_EXPR);
    IRExpression right_expr = ast_to_ir_expression(bin_expr->right, parent_fn, LOAD);

    IRSymAssignStatement assign_st;
    assign_st.left = left_expr.sym_expr;
    assign_st.right = NEW(IRExpression, 1);
    *(assign_st.right) = right_expr;

    return assign_st;
}

static inline IRCompoundStatement ast_to_ir_compound_st(Node* node, IRFunctionDefinition* parent_fn);
static inline IRBranchStatement ast_to_ir_branch_st(Node* node, IRFunctionDefinition* parent_fn)
{
    redassert(node->node_id == AST_TYPE_BRANCH_EXPR);

    IRBranchStatement result = ZERO_INIT;

    BranchExpr* ast_branch_expr = &node->branch_expr;
    Node* ast_condition_node = ast_branch_expr->condition;
    Node* ast_if_block_node = ast_branch_expr->if_block;
    Node* ast_else_block_node = ast_branch_expr->else_block;

    redassert(ast_condition_node);
    redassert(ast_if_block_node);
    ASTType ast_condition_node_id = ast_condition_node->node_id;
    switch (ast_condition_node_id)
    {
        case AST_TYPE_BIN_EXPR:
        {
            BinExpr* bin_expr = &ast_condition_node->bin_expr;
            result.condition.type = IR_EXPRESSION_TYPE_BIN_EXPR;
            result.condition.bin_expr = ast_to_ir_binary_expr(bin_expr, parent_fn);
            break;
        }
        default:
            RED_NOT_IMPLEMENTED;
            return result;
    }

    ASTType ast_if_block_node_id = ast_if_block_node->node_id;
    switch (ast_if_block_node_id)
    {
        case AST_TYPE_COMPOUND_STATEMENT:
            result.if_block = ast_to_ir_compound_st(ast_if_block_node, parent_fn);
            break;
        default:
            RED_NOT_IMPLEMENTED;
            break;
    }

    if (ast_else_block_node != null)
    {
        ASTType ast_else_block_node_id = ast_else_block_node->node_id;
        switch (ast_else_block_node_id)
        {
            case AST_TYPE_COMPOUND_STATEMENT:
            {
                result.else_block = ast_to_ir_compound_st(ast_else_block_node, parent_fn);
                break;
            }
            default:
                RED_NOT_IMPLEMENTED;
                break;
        }
    }
    else
    {
        result.else_block = (IRCompoundStatement)ZERO_INIT;
    }

    return result;
}

static inline IRSymDeclStatement ast_to_ir_sym_decl_st(Node* node, IRFunctionDefinition* parent_fn)
{
    IRSymDeclStatement st;
    st.is_const = node->sym_decl.is_const;
    st.name = node->sym_decl.sym->sym_expr.name;
    st.value = ast_to_ir_expression(node->sym_decl.value, parent_fn, LOAD);
    st.type = resolve_type(node->sym_decl.type, parent_fn);

    return st;
}

static inline IRCompoundStatement ast_to_ir_compound_st(Node* node, IRFunctionDefinition* parent_fn)
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
                case AST_TYPE_BRANCH_EXPR:
                    st_it->type = IR_ST_TYPE_BRANCH_ST;
                    st_it->branch_st = ast_to_ir_branch_st(st_node, parent_fn);
                    break;
                case AST_TYPE_RETURN_STATEMENT:
                    st_it->type = IR_ST_TYPE_RETURN_ST;
                    st_it->return_st = ast_to_ir_return_st(st_node, parent_fn);
                    break;
                case AST_TYPE_SYM_DECL:
                    st_it->type = IR_ST_TYPE_SYM_DECL_ST;
                    st_it->sym_decl_st = ast_to_ir_sym_decl_st(st_node, parent_fn);
                    decl_append(&parent_fn->sym_declarations, st_it->sym_decl_st);
                    break;
                case AST_TYPE_BIN_EXPR:
                {
                    BinExpr* ast_bin_expr = &st_node->bin_expr;
                    TokenID op = ast_bin_expr->op;

                    switch (op)
                    {
                        case TOKEN_ID_EQ:
                            st_it->type = IR_ST_TYPE_ASSIGN_ST;
                            st_it->sym_assign_st = ast_to_ir_assign_st(ast_bin_expr, parent_fn);
                            break;
                        default:
                            RED_NOT_IMPLEMENTED;
                            break;
                    }
                    break;
                }
                case AST_TYPE_LOOP_EXPR:
                    st_it->type = IR_ST_TYPE_LOOP_ST;
                    st_it->loop_st.condition = ast_to_ir_expression(st_node->loop_expr.condition, parent_fn, LOAD);
                    st_it->loop_st.body = ast_to_ir_compound_st(st_node->loop_expr.body, parent_fn);
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
    SB* fn_name = fn_proto->sym->sym_expr.name;
    IRParamDecl* params = null;

    if (param_count > 0)
    {
        params = NEW(IRParamDecl, param_count);

        for (u8 i = 0; i < param_count; i++)
        {
            Node* param = param_data[i];

            RedType red_type = resolve_type(param->param_decl.type, NULL);

            if (red_type_is_invalid(&red_type))
            {
                os_exit_with_message("unknown type for %s: %s\n", sb_ptr(param_name(param)), sb_ptr(param_type(param)));
            }

            params[i].type = red_type;

            if (!param_name_unique(params, i, param_name(param)))
            {
                os_exit_with_message("param name %s already used\n", sb_ptr(param_name(param)));
            }

            params[i].name = param_name(param);
        }
    }

    RedType ret_red_type = ZERO_INIT;

    if (fn_proto->ret_type)
    {
        ret_red_type = resolve_type(fn_proto->ret_type, NULL);
        if (red_type_is_invalid(&ret_red_type))
        {
            os_exit_with_message("Unknown type for return type in function %s\n", sb_ptr(fn_name));
        }
    }
    else
    {
        ret_red_type = (const RedType)
        {
            .kind = VOID,
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
        fn_def->body = ast_to_ir_compound_st(fn_body_node, fn_def);
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
    redassert(param->type.kind == PRIMITIVE);
    print("Param %s, type: %s\n", sb_ptr(param->name), primitive_type_str(param->type.primitive));
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

    if (fn_proto->ret_type.kind != VOID)
    {
        print("Return type: %s\n", primitive_type_str(fn_proto->ret_type.primitive));
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
    redassert(param->type.kind == PRIMITIVE);
    print("Param name: %s; param type: %s\n", sb_ptr(param->name), primitive_type_str(param->type.primitive));
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
    print("else\n");
    print_compount_st(&branch_st->else_block);
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
void print_ir_tree(RedModuleIR* tree)
{
    IRFunctionDefinitionBuffer* fb = &tree->fn_definitions;
    u32 fn_count = tree->fn_definitions.len;
    for (u32 i = 0; i < fn_count; i++)
    {
        IRFunctionDefinition* fn = &fb->ptr[i];
        print_fn_definition(fn);
    }
}

RedModuleIR transform_ast_to_ir(RedModuleTree* ast)
{
    RedModuleIR ir_tree = ZERO_INIT;
    ast_to_ir_fn_definitions(&ir_tree, &ast->fn_definitions);

#if RED_IR_VERBOSE
    print_ir_tree(&ir_tree);
#endif

    return ir_tree;
}
