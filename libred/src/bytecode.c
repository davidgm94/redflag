//
// Created by David on 03/12/2020.
//
#include "bytecode.h"
#include "types.h"
#include "ir.h"

typedef struct BCModule
{
    char* name;
    char* filename;
    BIT_FIELD_4(is_debug, pad1, pad2, pad3);
} BCModule;

typedef enum BCInstructionID
{
    FN_PROTO, // No function definition
    FN_DEF, // Full function definition
} BCInstructionID;

typedef enum BCScopeID
{
    MAIN,
    MODULE,
    BLABLABLA,
} BCScopeID;
typedef struct BCScope
{
    int foo;
} BCScope;

typedef struct BCBuilder
{
    int foo;
} BCBuilder;

typedef struct BCTarget
{
    char* triple;
} BCTarget;

void fooo(BCModule* root_module, IRFunctionPrototypeBuffer* fn_proto_bf)
{
    IRFunctionPrototype* fn_proto_it = fn_proto_bf->ptr;
    u32 fn_proto_count = fn_proto_bf->len;

    for (u32 i = 0; i < fn_proto_count; i++)
    {
        struct IRFunctionPrototype* it = &fn_proto_it[i];
        if (it->has_body)
        {
            break;
        }
        else
        {

        }
    }
}

