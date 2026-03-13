#ifndef MINILANG_CODEGEN_HPP
#define MINILANG_CODEGEN_HPP

#include "llvm/IR/Module.h"

#include <string>

namespace minilang
{
    void optimizeModule(llvm::Module* Mod);
    bool generateARMCode(llvm::Module* Mod, const std::string& outputFilename);
} // namespace minilang

#endif // MINILANG_CODEGEN_HPP
