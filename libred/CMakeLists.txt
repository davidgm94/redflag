project(libred)

set(
        LIBRED_SOURCE
        src/os.c
        src/compiler.c
        src/lexer.c
        src/parser.c
        src/bigint.c
        #src/ir.c
        src/bytecode.c
        src/main.c
        #src/llvm.c
)


set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

set(LLVM_ROOT_DIR ../dependencies/llvm)
set(LLVM_LIB_DIR ${LLVM_ROOT_DIR}/lib)
set(LLVM_INCLUDE_DIR ${LLVM_ROOT_DIR}/include)
set(LLD_ROOT_DIR ../dependencies/lld)
set(LLD_INCLUDE_DIR ${LLD_ROOT_DIR}/include)
set(LLD_LIB_DIR ${LLD_ROOT_DIR}/lib)

add_executable(libred ${LIBRED_SOURCE})
target_compile_definitions(libred PUBLIC RED_DEBUG=1)
target_include_directories(libred PUBLIC ${LLVM_INCLUDE_DIR})
target_link_libraries(libred llvm-wrapper)
