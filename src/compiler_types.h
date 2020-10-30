//
// Created by david on 8/10/20.
//
#pragma once
#include "types.h"
#include <string.h>
#include <ctype.h>
#include <malloc.h>

/* Forward declaration of structs */
struct RedType;
struct ASTNode;
//struct RedPackage;
/* Enum declaration */

enum Error
{
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
    TOKEN_ID_AMPERSAND = '&',
    TOKEN_ID_ARROW,
    TOKEN_ID_AT = '@',
    TOKEN_ID_BANG = '!',
    TOKEN_ID_BIT_OR = '|',
    TOKEN_ID_BIT_XOR = '^',
    TOKEN_ID_BIT_AND = '&',
    TOKEN_ID_CMP_GREATER = '>',
    TOKEN_ID_CMP_LESS = '<',
    TOKEN_ID_COLON = ':',
    TOKEN_ID_COMMA = ',',
    TOKEN_ID_DASH = '-',
    TOKEN_ID_HASH = '#',
    TOKEN_ID_EQ = '=',
    TOKEN_ID_DOT = '.',
    TOKEN_ID_LEFT_BRACE = '{',
    TOKEN_ID_LEFT_BRACKET = '[',
    TOKEN_ID_LEFT_PARENTHESIS = '(',
    TOKEN_ID_QUESTION = '?',
    TOKEN_ID_PERCENT = '%',
    TOKEN_ID_PLUS = '+',
    TOKEN_ID_RIGHT_BRACE = '}',
    TOKEN_ID_RIGHT_BRACKET = ']',
    TOKEN_ID_RIGHT_PARENTHESIS = ')',
    TOKEN_ID_SEMICOLON = ';',
    TOKEN_ID_SLASH = '/',
    TOKEN_ID_STAR = '*',
    TOKEN_ID_TILDE = '~',
    TOKEN_ID_BIT_OR_EQ,
    TOKEN_ID_BIT_XOR_EQ,
    TOKEN_ID_BIT_AND_EQ,
    TOKEN_ID_BIT_SHL,
    TOKEN_ID_BIT_SHL_EQ,
    TOKEN_ID_BIT_SHR,
    TOKEN_ID_BIT_SHR_EQ,
    TOKEN_ID_CHAR_LIT,
    TOKEN_ID_CMP_EQ,
    TOKEN_ID_CMP_GREATER_OR_EQ,
    TOKEN_ID_CMP_LESS_OR_EQ,
    TOKEN_ID_CMP_NOT_EQ,
    TOKEN_ID_DIV_EQ,
    TOKEN_ID_END_OF_FILE,
    TOKEN_ID_FAT_ARROW,
    TOKEN_ID_FLOAT_LIT,
    TOKEN_ID_INT_LIT,
    TOKEN_ID_KEYWORD_ALIGN,
    TOKEN_ID_KEYWORD_ALLOW_ZERO,
    TOKEN_ID_KEYWORD_AND,
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
    TOKEN_ID_KEYWORD_OR,
    TOKEN_ID_KEYWORD_PACKED,
    TOKEN_ID_KEYWORD_PUB,
    TOKEN_ID_KEYWORD_RETURN,
    TOKEN_ID_KEYWORD_SECTION,
    TOKEN_ID_KEYWORD_STRUCT,
    TOKEN_ID_KEYWORD_SWITCH,
    TOKEN_ID_KEYWORD_TEST,
    TOKEN_ID_KEYWORD_THREAD_LOCAL,
    TOKEN_ID_KEYWORD_TRUE,
    TOKEN_ID_KEYWORD_UNDEFINED,
    TOKEN_ID_KEYWORD_UNION,
    TOKEN_ID_KEYWORD_UNREACHABLE,
    TOKEN_ID_KEYWORD_VAR,
    TOKEN_ID_KEYWORD_VOID,
    TOKEN_ID_KEYWORD_VOLATILE,
    TOKEN_ID_KEYWORD_WHILE,
    // ...
    TOKEN_ID_MINUS_EQ,
    TOKEN_ID_MOD_EQ,
    TOKEN_ID_PLUS_EQ,
    TOKEN_ID_SYMBOL,
    TOKEN_ID_TIMES_EQ,
    TOKEN_ID_STRING_LIT,
    TOKEN_ID_MULTILINE_STRING_LIT,
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
    BIN_OP_TYPE_CMP_LESS,
    BIN_OP_TYPE_CMP_GREATER,
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

enum BinaryOpChain
{
    BINARY_OP_CHAIN_ONCE,
    BINARY_OP_CHAIN_INFINITE,
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
    NODE_TYPE_TYPE,
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

void print(const char* format, ...);
void logger(LogType log_type, const char *format, ...);
template <typename T>
static inline T* NEW(size_t element_count)
{
#if RED_CUSTOM_ALLOCATOR
    return reinterpret_cast<T*>(allocate_chunk(element_count * sizeof(T)));
#else
    size_t byte_count = element_count * sizeof(T);
    T* address = reinterpret_cast<T*>(malloc(byte_count));
    u8* it = (u8*)address;
    for (usize i = 0; i < byte_count; i++)
    {
        *it++ = 0;
    }
    return address;
#endif
}
template <typename T>
static inline T* RENEW(T* old_address, usize element_count)
{
#if RED_CUSTOM_ALLOCATOR
    return reinterpret_cast<T*>(reallocate_chunk(old_address, element_count * sizeof(T)));
#else
    return reinterpret_cast<T*>(realloc(old_address, element_count * sizeof(T)));
#endif
}

void* allocate_chunk(size_t size);
void* reallocate_chunk(void* allocated_address, usize size);
void mem_init(void);

static inline bool mem_eql_mem(const char* a, size_t a_len, const char* b, size_t b_len)
{
    if (a_len != b_len)
    {
        return false;
    }
    return memcmp(a, b, a_len) == 0;
}

static inline bool mem_eql_mem_ignore_case(const char* a, size_t a_len, const char* b, size_t b_len)
{
    if (a_len != b_len)
    {
        return false;
    }

    for (size_t i = 0; i < a_len; i++)
    {
        if (tolower(a[i]) != tolower(b[i]))
            return false;
    }
    return true;
}

static inline bool mem_eql_str(const char* mem, size_t mem_len, const char* str)
{
    return mem_eql_mem(mem, mem_len, str, strlen(str));
}

static inline bool str_eql_str(const char* a, const char* b)
{
    return mem_eql_mem(a, strlen(a), b, strlen(b));
}

static inline bool str_eql_str_ignore_case(const char* a, const char* b)
{
    return mem_eql_mem_ignore_case(a, strlen(a), b, strlen(b));
}

static inline bool is_power_of_2(u64 x)
{
    return x != 0 && ((x & (~x + 1)) == x);
}

static inline bool mem_ends_with_mem(const char* mem, size_t mem_len, const char* end, size_t end_len)
{
    if (mem_len < end_len)
    {
        return false;
    }
    return memcmp(mem + mem_len - end_len, end, end_len) == 0;
}

static inline bool mem_ends_with_str(const char* mem, size_t mem_len, const char* str)
{
    return mem_ends_with_mem(mem, mem_len, str, strlen(str));
}

template<typename T>
struct RedList
{
    T* items;
    usize length;
    usize capacity;

    void deinit()
    {
        RED_NOT_IMPLEMENTED;
    }
    void append(T item)
    {
        ensure_capacity(length + 1);
        items[length++] = item;
    }

    void append_assuming_capacity(T item)
    {
        items[length++] = item;
    }

    T& operator[](size_t index)
    {
        redassert(index >= 0);
        redassert(index < length);
        return items[index];
    }
    T& at(size_t index)
    {
        redassert(index != SIZE_MAX);
        redassert(index < length);
        return items[index];
    }
    T pop()
    {
        redassert(length >= 1);
        return items[--length];
    }

    void add_one()
    {
        return resize(length+1);
    }
    const T& last() const
    {
        redassert(length +1);
        return items[length - 1];
    }
    T& last()
    {
        redassert(length +1);
        return items[length - 1];
    }

    void resize(size_t new_length)
    {
        redassert(new_length >= 0);
        ensure_capacity(new_length);
        length = new_length;
    }

    void clear()
    {
        length = 0;
    }

    void ensure_capacity(size_t new_capacity)
    {
        size_t better_capacity = max(capacity, (size_t)16);
        while (better_capacity < new_capacity)
        {
            better_capacity *= 2;
        }
        if (better_capacity != capacity)
        {
#if RED_CUSTOM_ALLOCATOR
            items = RENEW(items, better_capacity);
            capacity = better_capacity;
#else
            usize m_capacity = this->capacity;
            items = RENEW(items, better_capacity);
            u8* to_be_initialized = (u8*)&items[m_capacity];
            memset(to_be_initialized, 0, sizeof(T) * better_capacity - m_capacity);
            capacity = better_capacity;
#endif
        }
    }
};

template <typename T>
using List = RedList<T>;
using Buffer = List<char>;


static inline Buffer buf_create_in_stack()
{
    return {};
}

Buffer *buf_sprintf(const char *format, ...)
__attribute__ ((format (printf, 1, 2)));

static inline int buf_len(Buffer *buf)
{
    redassert(buf->length);
    return buf->length - 1;
}

static inline char *buf_ptr(Buffer *buf)
{
    redassert(buf->length);
    return buf->items;
}

static inline void buf_resize(Buffer *buf, int new_len)
{
    buf->resize(new_len + 1);
    buf->at(buf_len(buf)) = 0;
}


static inline Buffer *buf_alloc_fixed(int size)
{
    Buffer *buf = NEW<Buffer>(1);
    redassert(buf->length == 0);
    redassert(buf->capacity == 0);
    buf_resize(buf, size);
    return buf;
}

static inline Buffer *buf_alloc(void)
{
    return buf_alloc_fixed(0);
}

static inline void buf_deinit(Buffer *buf)
{
    buf->deinit();
}

static inline void buf_init_from_mem(Buffer *buf, const char *ptr, int len)
{
    redassert(len != SIZE_MAX);
    buf->resize(len + 1);
    memcpy(buf_ptr(buf), ptr, len);
    buf->at(buf_len(buf)) = 0;
}

static inline void buf_init_from_str(Buffer *buf, const char *str)
{
    buf_init_from_mem(buf, str, strlen(str));
}

static inline void buf_init_from_buf(Buffer *buf, Buffer *other)
{
    buf_init_from_mem(buf, buf_ptr(other), buf_len(other));
}

static inline Buffer *buf_create_from_mem(const char *ptr, int len)
{
    Buffer* buf = NEW<Buffer>(1);
    buf_init_from_mem(buf, ptr, len);
    return buf;
}

static inline Buffer *buf_create_from_str(const char *str) {
    return buf_create_from_mem(str, strlen(str));
}

static inline Buffer *buf_slice(Buffer *in_buf, int start, int end) {
    redassert(in_buf->length);
    redassert(start >= 0);
    redassert(end >= 0);
    redassert(start < buf_len(in_buf));
    redassert(end <= buf_len(in_buf));
    Buffer* out_buf = NEW<Buffer>(1);
    out_buf->resize(end - start + 1);
    memcpy(buf_ptr(out_buf), buf_ptr(in_buf) + start, end - start);
    (*out_buf)[buf_len(out_buf)] = 0;
    return out_buf;
}

static inline void buf_append_mem(Buffer *buf, const char *mem, int mem_len) {
    redassert(buf->length);
    redassert(mem_len >= 0);
    int old_len = buf_len(buf);
    buf_resize(buf, old_len + mem_len);
    memcpy(buf_ptr(buf) + old_len, mem, mem_len);
    (*buf)[buf_len(buf)] = 0;
}

static inline void buf_append_str(Buffer *buf, const char *str) {
    redassert(buf->length);
    buf_append_mem(buf, str, strlen(str));
}

static inline void buf_append_buf(Buffer *buf, Buffer *append_buf) {
    redassert(buf->length);
    buf_append_mem(buf, buf_ptr(append_buf), buf_len(append_buf));
}

static inline void buf_append_char(Buffer *buf, uint8_t c) {
    redassert(buf->length);
    buf_append_mem(buf, (const char *)&c, 1);
}

void buf_appendf(Buffer *buf, const char *format, ...)
__attribute__ ((format (printf, 2, 3)));

static inline bool buf_eql_mem(Buffer *buf, const char *mem, int mem_len) {
    redassert(buf->length);
    if (buf_len(buf) != mem_len)
        return false;
    return memcmp(buf_ptr(buf), mem, mem_len) == 0;
}

bool buf_eql_str(Buffer* buf, const char* str);

static inline bool buf_starts_with_mem(Buffer *buf, const char *mem, size_t mem_len) {
    if (buf_len(buf) < mem_len) {
        return false;
    }
    return memcmp(buf_ptr(buf), mem, mem_len) == 0;
}

bool buf_eql_buf(Buffer *buf, Buffer *other);
uint32_t buf_hash(Buffer *buf);

Buffer* buf_vprintf(const char *format, va_list ap);

template <typename K, typename V, u32 (*HashFunction)(K key), bool (*EqualFn)(K a, K b)>
class HashMap
{
public:
    void init(int capacity) {
        init_capacity(capacity);
    }
    void deinit(void) {
        _entries.deinit();
        free(_index_bytes);
    }

    struct Entry {
        uint32_t hash;
        uint32_t distance_from_start_index;
        K key;
        V value;
    };

    void clear() {
        _entries.clear();
        memset(_index_bytes, 0, _indexes_len * capacity_index_size(_indexes_len));
        _max_distance_from_start_index = 0;
        _modification_count += 1;
    }

    size_t size() const {
        return _entries.length;
    }

    void put(const K &key, const V &value) {
        _modification_count += 1;

        // This allows us to take a pointer to an entry in `internal_put` which
        // will not become a dead pointer when the array list is appended.
        _entries.ensure_capacity(_entries.length + 1);

        if (_index_bytes == nullptr) {
            if (_entries.length < 16) {
                _entries.append({HashFunction(key), 0, key, value});
                return;
            } else {
                _indexes_len = 32;
                _index_bytes = NEW<uint8_t>(_indexes_len);
                _max_distance_from_start_index = 0;
                for (size_t i = 0; i < _entries.length; i += 1) {
                    Entry *entry = &_entries.items[i];
                    put_index(entry, i, _index_bytes);
                }
                return internal_put(key, value, _index_bytes);
            }
        }

        // if we would get too full (60%), double the indexes size
        if ((_entries.length + 1) * 5 >= _indexes_len * 3) {
            // TODO: FREE
            _index_bytes = nullptr;
            _indexes_len *= 2;
            size_t sz = capacity_index_size(_indexes_len);
            // This zero initializes the bytes, setting them all empty.
            //_index_bytes = heap::c_allocator.allocate<uint8_t>(_indexes_len * sz);
#if RED_CUSTOM_ALLOCATOR
            _index_bytes = (u8*)allocate_chunk(_indexes_len * sz);
#else
            _index_bytes = (u8*)malloc(_indexes_len * sz);
#endif
            _max_distance_from_start_index = 0;
            for (size_t i = 0; i < _entries.length; i += 1) {
                Entry *entry = &_entries.items[i];
                switch (sz) {
                    case 1:
                        put_index(entry, i, (uint8_t*)_index_bytes);
                        continue;
                    case 2:
                        put_index(entry, i, (uint16_t*)_index_bytes);
                        continue;
                    case 4:
                        put_index(entry, i, (uint32_t*)_index_bytes);
                        continue;
                    default:
                        put_index(entry, i, (size_t*)_index_bytes);
                        continue;
                }
            }
        }

        switch (capacity_index_size(_indexes_len)) {
            case 1: return internal_put(key, value, (uint8_t*)_index_bytes);
            case 2: return internal_put(key, value, (uint16_t*)_index_bytes);
            case 4: return internal_put(key, value, (uint32_t*)_index_bytes);
            default: return internal_put(key, value, (size_t*)_index_bytes);
        }
    }

    Entry *put_unique(const K &key, const V &value) {
        // TODO make this more efficient
        Entry *entry = internal_get(key);
        if (entry)
            return entry;
        put(key, value);
        return nullptr;
    }

    const V &get(const K &key) const {
        Entry *entry = internal_get(key);
        if (!entry)
            RED_PANIC("key not found");
        return entry->value;
    }

    Entry *maybe_get(const K &key) const {
        return internal_get(key);
    }

    bool remove(const K &key) {
        bool deleted_something = maybe_remove(key);
        if (!deleted_something)
            RED_PANIC("key not found");
        return deleted_something;
    }

    bool maybe_remove(const K &key) {
        _modification_count += 1;
        if (_index_bytes == nullptr) {
            uint32_t hash = HashFunction(key);
            for (size_t i = 0; i < _entries.length; i += 1) {
                if (_entries.items[i].hash == hash && EqualFn(_entries.items[i].key, key)) {
                    _entries.swap_remove(i);
                    return true;
                }
            }
            return false;
        }
        switch (capacity_index_size(_indexes_len)) {
            case 1: return internal_remove(key, (uint8_t*)_index_bytes);
            case 2: return internal_remove(key, (uint16_t*)_index_bytes);
            case 4: return internal_remove(key, (uint32_t*)_index_bytes);
            default: return internal_remove(key, (size_t*)_index_bytes);
        }
    }

    class Iterator {
    public:
        Entry *next() {
            if (_inital_modification_count != _table->_modification_count)
                RED_PANIC("concurrent modification");
            if (_index >= _table->_entries.length)
                return nullptr;
            Entry *entry = &_table->_entries.items[_index];
            _index += 1;
            return entry;
        }
    private:
        const HashMap * _table;
        // iterator through the entry array
        size_t _index = 0;
        // used to detect concurrent modification
        uint32_t _inital_modification_count;
        Iterator(const HashMap * table) :
                _table(table), _inital_modification_count(table->_modification_count) {
        }
        friend HashMap;
    };

    // you must not modify the underlying HashMap while this iterator is still in use
    Iterator entry_iterator() const {
        return Iterator(this);
    }

private:
    // Maintains insertion order.
    List<Entry> _entries;
    // If _indexes_len is less than 2**8, this is an array of uint8_t.
    // If _indexes_len is less than 2**16, it is an array of uint16_t.
    // If _indexes_len is less than 2**32, it is an array of uint32_t.
    // Otherwise it is size_t.
    // It's off by 1. 0 means empty slot, 1 means index 0, etc.
    uint8_t *_index_bytes;
    // This is the number of indexes. When indexes are bytes, it equals number of bytes.
    // When indexes are uint16_t, _indexes_len is half the number of bytes.
    size_t _indexes_len;

    size_t _max_distance_from_start_index;
    // This is used to detect bugs where a hashtable is edited while an iterator is running.
    uint32_t _modification_count;

    void init_capacity(size_t capacity) {
        _entries = {};
        _entries.ensure_capacity(capacity);
        _indexes_len = 0;
        if (capacity >= 16) {
            // So that at capacity it will only be 60% full.
            _indexes_len = capacity * 5 / 3;
            size_t sz = capacity_index_size(_indexes_len);
            // This zero initializes _index_bytes which sets them all to empty.
            // _index_bytes = heap::c_allocator.allocate<uint8_t>(_indexes_len * sz);
#if RED_CUSTOM_ALLOCATOR
            _index_bytes = (u8*)allocate_chunk(_indexes_len * sz);
#else
            _index_bytes = (u8*)malloc(_indexes_len * sz);
#endif
        } else {
            _index_bytes = nullptr;
        }

        _max_distance_from_start_index = 0;
        _modification_count = 0;
    }

    static size_t capacity_index_size(size_t len) {
        if (len < UINT8_MAX)
            return 1;
        if (len < UINT16_MAX)
            return 2;
        if (len < UINT32_MAX)
            return 4;
        return sizeof(size_t);
    }

    template <typename I>
    void internal_put(const K &key, const V &value, I *indexes) {
        uint32_t hash = HashFunction(key);
        uint32_t distance_from_start_index = 0;
        size_t start_index = hash_to_index(hash);
        for (size_t roll_over = 0; roll_over < _indexes_len;
             roll_over += 1, distance_from_start_index += 1)
        {
            size_t index_index = (start_index + roll_over) % _indexes_len;
            I index_data = indexes[index_index];
            if (index_data == 0) {
                _entries.append_assuming_capacity({ hash, distance_from_start_index, key, value });
                indexes[index_index] = _entries.length;
                if (distance_from_start_index > _max_distance_from_start_index)
                    _max_distance_from_start_index = distance_from_start_index;
                return;
            }
            // This pointer survives the following append because we call
            // _entries.ensure_capacity before internal_put.
            Entry *entry = &_entries.items[index_data - 1];
            if (entry->hash == hash && EqualFn(entry->key, key)) {
                *entry = {hash, distance_from_start_index, key, value};
                if (distance_from_start_index > _max_distance_from_start_index)
                    _max_distance_from_start_index = distance_from_start_index;
                return;
            }
            if (entry->distance_from_start_index < distance_from_start_index) {
                // In this case, we did not find the item. We will put a new entry.
                // However, we will use this index for the new entry, and move
                // the previous index down the line, to keep the _max_distance_from_start_index
                // as small as possible.
                _entries.append_assuming_capacity({ hash, distance_from_start_index, key, value });
                indexes[index_index] = _entries.length;
                if (distance_from_start_index > _max_distance_from_start_index)
                    _max_distance_from_start_index = distance_from_start_index;

                distance_from_start_index = entry->distance_from_start_index;

                // Find somewhere to put the index we replaced by shifting
                // following indexes backwards.
                roll_over += 1;
                distance_from_start_index += 1;
                for (; roll_over < _indexes_len; roll_over += 1, distance_from_start_index += 1) {
                    size_t index_index = (start_index + roll_over) % _indexes_len;
                    I next_index_data = indexes[index_index];
                    if (next_index_data == 0) {
                        if (distance_from_start_index > _max_distance_from_start_index)
                            _max_distance_from_start_index = distance_from_start_index;
                        entry->distance_from_start_index = distance_from_start_index;
                        indexes[index_index] = index_data;
                        return;
                    }
                    Entry *next_entry = &_entries.items[next_index_data - 1];
                    if (next_entry->distance_from_start_index < distance_from_start_index) {
                        if (distance_from_start_index > _max_distance_from_start_index)
                            _max_distance_from_start_index = distance_from_start_index;
                        entry->distance_from_start_index = distance_from_start_index;
                        indexes[index_index] = index_data;
                        distance_from_start_index = next_entry->distance_from_start_index;
                        entry = next_entry;
                        index_data = next_index_data;
                    }
                }
                RED_UNREACHABLE;
            }
        }
        RED_UNREACHABLE;
    }

    template <typename I>
    void put_index(Entry *entry, size_t entry_index, I *indexes) {
        size_t start_index = hash_to_index(entry->hash);
        size_t index_data = entry_index + 1;
        for (size_t roll_over = 0, distance_from_start_index = 0;
             roll_over < _indexes_len; roll_over += 1, distance_from_start_index += 1)
        {
            size_t index_index = (start_index + roll_over) % _indexes_len;
            size_t next_index_data = indexes[index_index];
            if (next_index_data == 0) {
                if (distance_from_start_index > _max_distance_from_start_index)
                    _max_distance_from_start_index = distance_from_start_index;
                entry->distance_from_start_index = distance_from_start_index;
                indexes[index_index] = index_data;
                return;
            }
            Entry *next_entry = &_entries.items[next_index_data - 1];
            if (next_entry->distance_from_start_index < distance_from_start_index) {
                if (distance_from_start_index > _max_distance_from_start_index)
                    _max_distance_from_start_index = distance_from_start_index;
                entry->distance_from_start_index = distance_from_start_index;
                indexes[index_index] = index_data;
                distance_from_start_index = next_entry->distance_from_start_index;
                entry = next_entry;
                index_data = next_index_data;
            }
        }
        RED_UNREACHABLE;
    }

    Entry *internal_get(const K &key) const {
        if (_index_bytes == nullptr) {
            uint32_t hash = HashFunction(key);
            for (size_t i = 0; i < _entries.length; i += 1) {
                if (_entries.items[i].hash == hash && EqualFn(_entries.items[i].key, key)) {
                    return &_entries.items[i];
                }
            }
            return nullptr;
        }
        switch (capacity_index_size(_indexes_len)) {
            case 1: return internal_get2(key, (uint8_t*)_index_bytes);
            case 2: return internal_get2(key, (uint16_t*)_index_bytes);
            case 4: return internal_get2(key, (uint32_t*)_index_bytes);
            default: return internal_get2(key, (size_t*)_index_bytes);
        }
    }

    template <typename I>
    Entry *internal_get2(const K &key, I *indexes) const {
        uint32_t hash = HashFunction(key);
        size_t start_index = hash_to_index(hash);
        for (size_t roll_over = 0; roll_over <= _max_distance_from_start_index; roll_over += 1) {
            size_t index_index = (start_index + roll_over) % _indexes_len;
            size_t index_data = indexes[index_index];
            if (index_data == 0)
                return nullptr;

            Entry *entry = &_entries.items[index_data - 1];
            if (entry->hash == hash && EqualFn(entry->key, key))
                return entry;
        }
        return nullptr;
    }

    size_t hash_to_index(uint32_t hash) const {
        return ((size_t)hash) % _indexes_len;
    }

    template <typename I>
    bool internal_remove(const K &key, I *indexes) {
        uint32_t hash = HashFunction(key);
        size_t start_index = hash_to_index(hash);
        for (size_t roll_over = 0; roll_over <= _max_distance_from_start_index; roll_over += 1) {
            size_t index_index = (start_index + roll_over) % _indexes_len;
            size_t index_data = indexes[index_index];
            if (index_data == 0)
                return false;

            size_t index = index_data - 1;
            Entry *entry = &_entries.items[index];
            if (entry->hash != hash || !EqualFn(entry->key, key))
                continue;

            size_t prev_index = index_index;
            _entries.swap_remove(index);
            if (_entries.length > 0 && _entries.length != index) {
                // Because of the swap remove, now we need to update the index that was
                // pointing to the last entry and is now pointing to this removed item slot.
                update_entry_index(_entries.length, index, indexes);
            }

            // Now we have to shift over the following indexes.
            roll_over += 1;
            for (; roll_over < _indexes_len; roll_over += 1) {
                size_t next_index = (start_index + roll_over) % _indexes_len;
                if (indexes[next_index] == 0) {
                    indexes[prev_index] = 0;
                    return true;
                }
                Entry *next_entry = &_entries.items[indexes[next_index] - 1];
                if (next_entry->distance_from_start_index == 0) {
                    indexes[prev_index] = 0;
                    return true;
                }
                indexes[prev_index] = indexes[next_index];
                prev_index = next_index;
                next_entry->distance_from_start_index -= 1;
            }
            RED_UNREACHABLE;
        }
        return false;
    }

    template <typename I>
    void update_entry_index(size_t old_entry_index, size_t new_entry_index, I *indexes) {
        size_t start_index = hash_to_index(_entries.items[new_entry_index].hash);
        for (size_t roll_over = 0; roll_over <= _max_distance_from_start_index; roll_over += 1) {
            size_t index_index = (start_index + roll_over) % _indexes_len;
            if (indexes[index_index] == old_entry_index + 1) {
                indexes[index_index] = new_entry_index + 1;
                return;
            }
        }
        RED_UNREACHABLE;
    }
};

template<typename T>
struct Slice {
    T *ptr;
    size_t len;

    inline T &at(size_t i) {
        redassert(i < len);
        return ptr[i];
    }

    inline Slice<T> slice(size_t start, size_t end) {
        redassert(end <= len);
        redassert(end >= start);
        return {
                ptr + start,
                end - start,
        };
    }

    inline Slice<T> sliceFrom(size_t start) {
        redassert(start <= len);
        return {
                ptr + start,
                len - start,
        };
    }

    static inline Slice<T> alloc(size_t n) {
        return {heap::c_allocator.allocate_nonzero<T>(n), n};
    }
};

template<typename T>
static inline bool slice_eql(Slice<T> a, Slice<T> b) {
    if (a.len != b.len)
        return false;
    for (size_t i = 0; i < a.len; i += 1) {
        if (a.ptr[i] != b.ptr[i])
            return false;
    }
    return true;
}

template<typename T>
static inline bool slice_starts_with(Slice<T> haystack, Slice<T> needle) {
    if (needle.len > haystack.len)
        return false;
    return slice_eql(haystack.slice(0, needle.len), needle);
}
struct BigInt
{
    size_t digit_count;
    union
    {
        u64 digit;
        u64* digits; // least significant digit first
    };
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
    char value;
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
    };
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
    bool external_linkage;
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
    List<ASTNode*> parameters;
    Buffer* name;
    ASTNode* scope_function;
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
    ASTNode* type_node;
    ContainerType type_kind;
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

struct ASTNodeType
{
    Buffer* type;
    bool is_void;
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

struct ASTNodeReturnExpression
{
    ASTNode* expression;
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
    };
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
    };
};

struct CodeGenConfig
{
    bool release;
    bool strip_debug_symbols;
    bool is_static;
};

template<typename T>
struct Optional {
    T value;
    bool is_some;

    static inline Optional<T> some(T x) {
        return {x, true};
    }

    static inline Optional<T> none() {
        return {{}, false};
    }

    inline bool unwrap(T *res) {
        *res = value;
        return is_some;
    }
};

struct TypeTableEntry;

struct TypeNode
{
    TypeTableEntry* entry;
};

struct CodeGenNode
{
    union
    {
        TypeNode type_node;
    };
};

struct ASTNode
{
    NodeType type;
    bool already_traced_this_node;
    size_t line;
    size_t column;
    RedType* owner;
    CodeGenNode* codegen_node;
    union
    {
        ASTNodeFunctionDefinition fn_definition;
        ASTNodeFunctionPrototype fn_prototype;
        ASTNodeParameterDeclaration param_decl;
        ASTNodeBlock block;
        ASTNode* grouped_expr;
        ASTNodeReturnExpression return_expr;
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
        ASTNodeType type_expr;
        ASTNodeBoolLiteral bool_literal;
        ASTNodeBreakExpression break_expr;
        ASTNodeContinueExpression continue_expr;
        ASTNodeUnreachableExpression unreachable_expr;
        ASTNodeArrayType array_type;
        ASTNodeEnumLiteral enum_literal;
    };
};

#if RED_NEW_PARSER
#include <vector>
namespace RedAST
{
    static inline Buffer* token_buffer(Token* token)
    {
        if (token == nullptr)
        {
            return nullptr;
        }
        redassert(token->id == TOKEN_ID_STRING_LIT || token->id == TOKEN_ID_MULTILINE_STRING_LIT || token->id == TOKEN_ID_SYMBOL);
        return &token->str_lit.str;
    }

    static inline BigInt* token_bigint(Token* token)
    {
        if (token == nullptr)
        {
            return nullptr;
        }
        redassert(token->id == TOKEN_ID_INT_LIT);
        return &token->int_lit.big_int;
    }

    static inline BigFloat* token_bigfloat(Token* token)
    {
        if (token == nullptr)
        {
            return nullptr;
        }
        redassert(token->id == TOKEN_ID_FLOAT_LIT);
        return &token->float_lit.big_float;
    }
    struct Expression
    {
        usize line;
        usize column;

        Expression(Token* token)
        {
            line = token->start_line;
            column = token->start_column;
        }
        Expression(usize line, usize column)
        {
            this->line = line;
            this->column = column;
        }
    };

    struct IntExpr: public Expression
    {
        BigInt* bigint;
        IntExpr(Token* token)
            :  Expression(token), bigint(token_bigint(token))
        {}
    };

    struct FloatExpr : public Expression
    {
        BigFloat* bigfloat;
        FloatExpr(Token* token)
            :  Expression(token), bigfloat(token_bigfloat(token))
        {}
    };

    struct SymbolExpr : public Expression
    {
        Buffer* name;
        SymbolExpr(Token* name_token)
            : Expression(name_token), name(token_buffer(name_token))
        {}
    };

    struct VariableExpr : public Expression
    {
        Buffer* name;
        Buffer* type;

        VariableExpr(Token* name_token, Token* type_token)
            : Expression(name_token), name(token_buffer(name_token)), type(token_buffer(type_token))
        { }
    };

    struct BinaryExpr : public Expression
    {
        u8 op;
        Expression* left;
        Expression* right;

        BinaryExpr(Token* bin_op_token, Expression* left, Expression* right)
            : Expression(bin_op_token), op(bin_op_token->id), left(left), right(right)
        { }
    };

    struct ReturnExpr : public Expression
    {
        Expression* return_expr;

        ReturnExpr(Token* ret_keyword_tok, Expression* ret_expr)
            : Expression(ret_keyword_tok), return_expr(ret_expr)
        { }
    };

    struct CallExpr : public Expression
    {
        Buffer* callee;
        std::vector<Expression*> args;
    };

    struct Prototype
    {
        Buffer* name;
        std::vector<Expression*> args;
        Buffer* return_type;

        Prototype(Token* name_token, std::vector<Expression*> args, Token* return_type_token)
            : name(token_buffer(name_token)), args(args), return_type(token_buffer(return_type_token))
        { }
    };

    struct Function
    {
        Prototype* proto;
        Expression* body;

        Function(Prototype* proto, Expression* body)
            : proto(proto), body(body)
        {}
    };

    template <typename T>
    struct LLVMMap
    {
        Buffer* keys[10000];
        T* values[10000];
        size_t count;

        T* find_value(Buffer* key)
        {
            for (usize i = 0; i < count; i++)
            {
                bool found = buf_eql_buf(key, keys[i]);
                if (found)
                {
                    return values[i];
                }
            }

            return nullptr;
        }

        void append(Buffer* key, T* value)
        {
            redassert(count + 1 < 10000);
            keys[count] = key;
            values[count] = value;
            count++;
        }

        void clear()
        {
            memset(keys, 0, sizeof(Buffer*) * count);
            memset(values, 0, sizeof(T*) * count);
            count = 0;
        }
    };
}
#endif
