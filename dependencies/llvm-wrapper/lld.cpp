#include "lld.h"
#include <lld/Common/Driver.h>
#include <stdio.h>

struct FuckCPPStream : public llvm::raw_ostream
{
    FuckCPPStream() : raw_ostream(true), pos(0) { }
    void write_impl(const char* ptr, size_t len) override
    {
		printf("%s", ptr);
		this->pos += len;
    }
	uint64_t current_pos() const override { return pos; }
	size_t pos;
};

bool lld_linker_driver(const char** args, size_t arg_count, LLDBinaryFormat format)
{
	llvm::ArrayRef<const char*> arguments(args, arg_count);
	FuckCPPStream  stdout_stream{};
	FuckCPPStream  stderr_stream{};
	bool result = false;
	switch (format)
	{
		case LLD_BINARY_FORMAT_COFF:
			result = lld::coff::link(arguments, false, stdout_stream, stderr_stream);
			break;
		case LLD_BINARY_FORMAT_ELF:
			assert(0);
			break;
		default:
			assert(0);
			break;
	}

    return result;
}