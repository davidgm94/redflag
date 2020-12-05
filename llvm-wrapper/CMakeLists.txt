project(llvm-wrapper)
set(LLVM_WRAPPER_SOURCE
        src/lld.cpp
        src/microsoft_craziness.cpp
)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
set (LLD_LIBS
        lldCommon.lib lldDriver.lib lldMachO.lib lldMinGW.lib lldWasm.lib lldCOFF.lib lldCore.lib lldELF.lib lldMachO2.lib lldReaderWriter.lib lldYAML.lib
)

set (LLVM_LIBS
        LLVMXRay.lib LLVMWindowsManifest.lib LLVMTableGen.lib LLVMSymbolize.lib LLVMDebugInfoPDB.lib LLVMOrcJIT.lib LLVMOrcError.lib LLVMJITLink.lib LLVMObjectYAML.lib LLVMMCA.lib LLVMLTO.lib LLVMPasses.lib LLVMCoroutines.lib LLVMObjCARCOpts.lib LLVMExtensions.lib LLVMLineEditor.lib LLVMLibDriver.lib LLVMInterpreter.lib LLVMFuzzMutate.lib LLVMMCJIT.lib LLVMExecutionEngine.lib LLVMRuntimeDyld.lib LLVMDWARFLinker.lib LLVMDlltoolDriver.lib LLVMOption.lib LLVMDebugInfoGSYM.lib LLVMCoverage.lib LLVMXCoreDisassembler.lib LLVMXCoreCodeGen.lib LLVMXCoreDesc.lib LLVMXCoreInfo.lib LLVMX86Disassembler.lib LLVMX86AsmParser.lib LLVMX86CodeGen.lib LLVMX86Desc.lib LLVMX86Info.lib LLVMWebAssemblyDisassembler.lib LLVMWebAssemblyCodeGen.lib LLVMWebAssemblyDesc.lib LLVMWebAssemblyAsmParser.lib LLVMWebAssemblyInfo.lib LLVMSystemZDisassembler.lib LLVMSystemZCodeGen.lib LLVMSystemZAsmParser.lib LLVMSystemZDesc.lib LLVMSystemZInfo.lib LLVMSparcDisassembler.lib LLVMSparcCodeGen.lib LLVMSparcAsmParser.lib LLVMSparcDesc.lib LLVMSparcInfo.lib LLVMRISCVDisassembler.lib LLVMRISCVCodeGen.lib LLVMRISCVAsmParser.lib LLVMRISCVDesc.lib LLVMRISCVUtils.lib LLVMRISCVInfo.lib LLVMPowerPCDisassembler.lib LLVMPowerPCCodeGen.lib LLVMPowerPCAsmParser.lib LLVMPowerPCDesc.lib LLVMPowerPCInfo.lib LLVMNVPTXCodeGen.lib LLVMNVPTXDesc.lib LLVMNVPTXInfo.lib LLVMMSP430Disassembler.lib LLVMMSP430CodeGen.lib LLVMMSP430AsmParser.lib LLVMMSP430Desc.lib LLVMMSP430Info.lib LLVMMipsDisassembler.lib LLVMMipsCodeGen.lib LLVMMipsAsmParser.lib LLVMMipsDesc.lib LLVMMipsInfo.lib LLVMLanaiDisassembler.lib LLVMLanaiCodeGen.lib LLVMLanaiAsmParser.lib LLVMLanaiDesc.lib LLVMLanaiInfo.lib LLVMHexagonDisassembler.lib LLVMHexagonCodeGen.lib LLVMHexagonAsmParser.lib LLVMHexagonDesc.lib LLVMHexagonInfo.lib LLVMBPFDisassembler.lib LLVMBPFCodeGen.lib LLVMBPFAsmParser.lib LLVMBPFDesc.lib LLVMBPFInfo.lib LLVMAVRDisassembler.lib LLVMAVRCodeGen.lib LLVMAVRAsmParser.lib LLVMAVRDesc.lib LLVMAVRInfo.lib LLVMARMDisassembler.lib LLVMARMCodeGen.lib LLVMARMAsmParser.lib LLVMARMDesc.lib LLVMARMUtils.lib LLVMARMInfo.lib LLVMAMDGPUDisassembler.lib LLVMAMDGPUCodeGen.lib LLVMMIRParser.lib LLVMipo.lib LLVMInstrumentation.lib LLVMVectorize.lib LLVMLinker.lib LLVMIRReader.lib LLVMAsmParser.lib LLVMFrontendOpenMP.lib LLVMAMDGPUAsmParser.lib LLVMAMDGPUDesc.lib LLVMAMDGPUUtils.lib LLVMAMDGPUInfo.lib LLVMAArch64Disassembler.lib LLVMMCDisassembler.lib LLVMAArch64CodeGen.lib LLVMCFGuard.lib LLVMGlobalISel.lib LLVMSelectionDAG.lib LLVMAsmPrinter.lib LLVMDebugInfoDWARF.lib LLVMCodeGen.lib LLVMTarget.lib LLVMScalarOpts.lib LLVMInstCombine.lib LLVMAggressiveInstCombine.lib LLVMTransformUtils.lib LLVMBitWriter.lib LLVMAnalysis.lib LLVMProfileData.lib LLVMObject.lib LLVMTextAPI.lib LLVMBitReader.lib LLVMCore.lib LLVMRemarks.lib LLVMBitstreamReader.lib LLVMAArch64AsmParser.lib LLVMMCParser.lib LLVMAArch64Desc.lib LLVMMC.lib LLVMDebugInfoCodeView.lib LLVMDebugInfoMSF.lib LLVMBinaryFormat.lib LLVMAArch64Utils.lib LLVMAArch64Info.lib LLVMSupport.lib LLVMDemangle.lib
)

set(LLVM_ROOT_DIR ../dependencies/llvm)
set(LLVM_LIB_DIR ${LLVM_ROOT_DIR}/lib)
set(LLVM_INCLUDE_DIR ${LLVM_ROOT_DIR}/include)
set(LLD_ROOT_DIR ../dependencies/lld)
set(LLD_INCLUDE_DIR ${LLD_ROOT_DIR}/include)
set(LLD_LIB_DIR ${LLD_ROOT_DIR}/lib)
add_library(llvm-wrapper ${LLVM_WRAPPER_SOURCE})
target_compile_options(llvm-wrapper PUBLIC -D_ITERATOR_DEBUG_LEVEL=0 -MT)
target_include_directories(llvm-wrapper PUBLIC ${LLD_INCLUDE_DIR} ${LLVM_INCLUDE_DIR} src)
target_link_directories(llvm-wrapper PUBLIC ${LLD_LIB_DIR} ${LLVM_LIB_DIR})
target_link_libraries(llvm-wrapper PUBLIC ${LLD_LIBS} ${LLVM_LIBS})