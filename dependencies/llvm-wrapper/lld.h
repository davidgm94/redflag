typedef enum LLDBinaryFormat
{
    LLD_BINARY_FORMAT_COFF,
    LLD_BINARY_FORMAT_ELF,
} LLDBinaryFormat;

#ifdef __cplusplus
extern "C"
{
#endif
    bool lld_linker_driver(const char** args, size_t arg_count, LLDBinaryFormat format);
#ifdef __cplusplus
}
#endif
