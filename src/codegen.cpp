#include "codegen.hpp"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/TargetParser/Triple.h"
#include "llvm/Support/CodeGen.h"
#include <iostream>

namespace minilang
{

    void optimizeModule(llvm::Module* Mod)
    {
        llvm::legacy::PassManager passManager;

        passManager.add(llvm::createReassociatePass());
        passManager.add(llvm::createGVNPass());
        passManager.add(llvm::createCFGSimplificationPass());
        passManager.add(llvm::createDeadCodeEliminationPass());
        
        passManager.add(llvm::createLoopUnrollPass());

        passManager.run(*Mod);
    }

    bool generateARMCode(llvm::Module* Mod, const std::string& outputFilename)
    {

        llvm::DebugFlag = true;


        LLVMInitializeARMTargetInfo();
        LLVMInitializeARMTarget();
        LLVMInitializeARMTargetMC();
        LLVMInitializeARMAsmPrinter();

        LLVMInitializeAArch64Target();
        LLVMInitializeAArch64TargetInfo();
        LLVMInitializeAArch64TargetMC();
        LLVMInitializeAArch64AsmPrinter();

        std::string targetTriple = "arm64-apple-darwin";
        llvm::Triple triple(targetTriple);
        Mod->setTargetTriple(triple);

        std::string error;

        const llvm::Target *target = llvm::TargetRegistry::lookupTarget(triple, error);
        if (!target)
        {
            llvm::errs() << "Error looking up target: " << error << "\n";
            return false;
        }

        llvm::TargetOptions opt;
        opt.FloatABIType = llvm::FloatABI::Soft;

        llvm::TargetMachine* targetMachine = target->createTargetMachine(
            triple,
            "generic",
            "",
            opt,
            std::nullopt,
            std::nullopt,
            llvm::CodeGenOptLevel::Default,
            false
        );

        if (!targetMachine)
        {
            llvm::errs() << "Could not create target machine\n";
            return false;
        }

        std::error_code EC;
        llvm::raw_fd_ostream dest(outputFilename, EC, llvm::sys::fs::OF_None);
        if (EC)
        {
            llvm::errs() << "Could not open file: " << EC.message() << "\n";
            return false;
        }

        llvm::legacy::PassManager pass;
        if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, llvm::CodeGenFileType::AssemblyFile))
        {
            llvm::errs() << "TargetMachine can't emit a file of this type\n";
            return false;
        }

        pass.run(*Mod);
        dest.flush();
        return true;
    }

} // namespace minilang
