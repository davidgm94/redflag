#pragma once

#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Target.h>
#include <llvm-c/Initialization.h>
#include <llvm-c/TargetMachine.h>

#include <llvm/InitializePasses.h>
#include <llvm/PassRegistry.h>
#include <llvm/MC/SubtargetFeature.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Support/Host.h>


namespace red_llvm
{
	static inline void initialize_loop_strength_reduce_pass(LLVMPassRegistryRef r)
	{
		llvm::initializeLoopStrengthReducePass(*llvm::unwrap(r));
	}

	static inline void initialize_lower_intrinsics_pass(LLVMPassRegistryRef r)
	{
		llvm::initializeLowerIntrinsicsPass(*llvm::unwrap(r));
	}

	static inline void initialize_unreachable_block_elim_pass(LLVMPassRegistryRef r)
	{
		llvm::initializeUnreachableBlockElimLegacyPassPass(*llvm::unwrap(r));
	}

	static inline char* get_host_CPU_name(void)
	{
		return _strdup(llvm::sys::getHostCPUName().str().c_str());
	}
	static inline char* get_native_features(void)
	{
		llvm::SubtargetFeatures features;
		llvm::StringMap<bool> host_features;
		if (llvm::sys::getHostCPUFeatures(host_features))
		{
			for (auto& feature : host_features)
			{
				features.AddFeature(feature.first(), feature.second);
			}
		}

		return _strdup((const char*)llvm::StringRef(features.getString()).bytes_begin());
	}
}
