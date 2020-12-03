project(libred)

set(
        LIBRED_SOURCE
        src/os.c
        src/src_file.c
        src/lexer.c
        src/parser.c
        src/bigint.c
        src/ir.c
        src/llvm.c
        src/main.c
)



set(LLVM_ROOT_DIR ../dependencies/llvm)
set(LLVM_LIB_DIR ${LLVM_ROOT_DIR}/lib)
set(LLVM_INCLUDE_DIR ${LLVM_ROOT_DIR}/include)
set(LLD_ROOT_DIR ../dependencies/lld)
set(LLD_INCLUDE_DIR ${LLD_ROOT_DIR}/include)
set(LLD_LIB_DIR ${LLD_ROOT_DIR}/lib)

add_executable(libred ${LIBRED_SOURCE})
target_include_directories(libred PUBLIC ${LLVM_INCLUDE_DIR})
target_link_libraries(libred llvm-wrapper)
