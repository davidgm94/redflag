#include "types.h"
#include "compiler_types.h"
#include <Windows.h>
#include <assert.h>
#include <stdio.h>
#include "x64_backend.h"


typedef enum Mod
{
    MOD_DISPLACEMENT_0 = 0b00,
    MOD_DISPLACEMENT_s8 = 0b01,
    MOD_DISPLACEMENT_s32 = 0b10,
    MOD_REGISTER = 0b11,
} Mod;

typedef enum Rex
{
    REX   = 0b01000000,
    REX_W = 0b01001000,
    REX_R = 0b01000100,
    REX_X = 0b01000010,
    REX_B = 0b01000001,
} Rex;

typedef enum OperandType
{
    OPERAND_TYPE_REGISTER = 1,
    OPERAND_TYPE_REGISTER_MEMORY,
} OperandType;

typedef struct Register
{
    u8 index;
} Register;

typedef struct Operand
{
    OperandType type;
    union
    {
        Register reg;
    };
} Operand;

const Operand rax =
{
    .type = OPERAND_TYPE_REGISTER,
    .reg.index = (u8)0xb0000,
};

const Operand rcx =
{
    .type = OPERAND_TYPE_REGISTER,
    .reg.index = (u8)0xb0001,
};

typedef enum x64_Mnemonic
{
    MOV,
} x64_Mnemonic;

typedef struct Instruction
{
    x64_Mnemonic mnemonic;
    Operand operands[2];
} Instruction;

typedef enum InstructionExtensionType
{
    INSTRUCTION_EXTENSION_TYPE_REGISTER = 1,
    INSTRUCTION_EXTENSION_TYPE_OP_CODE,
    INSTRUCTION_EXTENSION_TYPE_OP_CODE_PLUS_REGISTER,
} InstructionExtensionType;

typedef struct InstructionEncoding
{
    u16 op_code;
    InstructionExtensionType ext_type;
    OperandType operand_types[2];
} InstructionEncoding;

typedef U8Buffer U8B;
U8B make_buffer(s64 capacity, s32 mem_flags)
{
    void* memory = VirtualAlloc(nullptr, capacity, MEM_COMMIT | MEM_RESERVE, mem_flags);
    assert(memory);
    U8B u8b;
    u8b.ptr = memory;
    u8b.cap = capacity;
    u8b.len = 0;
    return u8b;
}

void u8_append_mem(U8B* b, void* mem, usize size)
{
    redassert(b->len + size < b->cap);
    memcpy(&b->ptr[b->len], mem, size);
    b->len += size;
}

void u8_append_u8(U8B* b, u8 c)
{
    redassert(b->len + 1 < b->cap);
    b->ptr[b->len++] = c;
}

void u8_append_s32(U8B* b, s32 c)
{
    u8_append_mem(b, &c, sizeof(s32));
}

void u8_append_sub_rsp_imm_8(U8B* b, s8 fn_handle)
{
    u8_append_u8(b, 0x48);
    u8_append_u8(b, 0x83);
    u8_append_u8(b, 0xec);
    u8_append_u8(b, fn_handle);
}

void u8_append_add_rsp_imm_8(U8B* b, s8 fn_handle)
{
    u8_append_u8(b, 0x48);
    u8_append_u8(b, 0x83);
    u8_append_u8(b, 0xc4);
    u8_append_u8(b, fn_handle);
}

void u8_append_mov_to_stack_offset_imm_32(U8B* b, s8 offset, s32 fn_handle)
{
    u8_append_u8(b, 0xc7);
    u8_append_u8(b, 0x44);
    u8_append_u8(b, 0x24);
    u8_append_u8(b, offset);
    u8_append_s32(b, fn_handle);
}

void u8_append_add_to_ecx_value_at_stack_offset(U8B* b, s8 fn_handle)
{
    u8_append_u8(b, 0x03);
    u8_append_u8(b, 0x4c);
    u8_append_u8(b, 0x24);
    u8_append_u8(b, fn_handle);
}

void encode(U8Buffer* b, Instruction instruction)
{
    // TODO: this should be a lookup table
    InstructionEncoding encoding =
    {
        .op_code = 0x89,
        .ext_type = INSTRUCTION_EXTENSION_TYPE_REGISTER,
        .operand_types =
        {
            [0] = OPERAND_TYPE_REGISTER_MEMORY,
            [1] = OPERAND_TYPE_REGISTER,
        },
    };

    // TODO: check that encoding matches the instruction
    // TODO: add REX.W prefix only if necessary
    u8_append_u8(b, REX_W);
    // TODO: if opcode is 2 byte need different append
    u8_append_u8(b, encoding.op_code);
    u8 mod_r_m = (
        (MOD_REGISTER << 6) |
        (instruction.operands[0].reg.index << 3) |
        (instruction.operands[1].reg.index)
    );

    u8_append_u8(b, mod_r_m);
}

void ptest(const char* text, bool expr)
{
    printf("%s %s\n", text, expr ? "OK" : "FAIL");
}

static const u8 x64_ret = 0xc3;

get_constant_s32* make_constant_s32(s32 fn_handle)
{
    U8B buffer = make_buffer(1024, PAGE_EXECUTE_READWRITE);
    u8_append_u8(&buffer, 0x48);
    u8_append_u8(&buffer, 0xc7);
    u8_append_u8(&buffer, 0xc0);
    u8_append_s32(&buffer, fn_handle);
    u8_append_u8(&buffer, x64_ret);
    return (get_constant_s32*)buffer.ptr;
}

identity_s64* make_identity_s64(void)
{
    U8B b = make_buffer(1024, PAGE_EXECUTE_READWRITE);
    encode(&b, (Instruction) { MOV, { rcx, rax } });
    u8_append_u8(&b, x64_ret);
    return (identity_s64*)b.ptr;
}

increment_s64* make_increment_s64(void)
{
    U8B b = make_buffer(1024, PAGE_EXECUTE_READWRITE);
    u8_append_sub_rsp_imm_8(&b, 24);
    u8_append_mov_to_stack_offset_imm_32(&b, 0, 1);
    u8_append_add_to_ecx_value_at_stack_offset(&b, 0);
    encode(&b, (Instruction) { MOV, { rcx, rax } });
    u8_append_add_rsp_imm_8(&b, 24);
    u8_append_u8(&b, x64_ret);
    return (increment_s64*)b.ptr;
}

