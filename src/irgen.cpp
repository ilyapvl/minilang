#include "irgen.hpp"
#include "llvm/IR/InlineAsm.h"
#include <iostream>
#include <cassert>

namespace minilang
{

    IRGenerator::IRGenerator(llvm::LLVMContext& context, const std::string& moduleName)
        : 
        m_context(context),
        m_builder(context),
        m_mainFunction(nullptr),
        m_entryBlock(nullptr),
        m_sysWriteFunc(nullptr),
        m_sysReadFunc(nullptr),
        m_intToStrFunc(nullptr),
        m_strToIntFunc(nullptr),
        m_printIntFunc(nullptr),
        m_printBoolFunc(nullptr),
        m_readIntFunc(nullptr),
        m_collecting(false)
    {
        m_module = new llvm::Module(moduleName, context);
    }

 
    llvm::AllocaInst* IRGenerator::createAllocaInEntry(llvm::Type* type, const std::string& name)
    {
        llvm::IRBuilder<>::InsertPoint savedIP = m_builder.saveIP();
        m_builder.SetInsertPoint(m_currentEntryBlock);
        auto alloca = m_builder.CreateAlloca(type, nullptr, name);
        m_builder.restoreIP(savedIP);
        return alloca;
    }

    bool IRGenerator::generate(Program* ast)
    {

        declareSysWrappers();

        // builtin
        createIntToString();
        createStringToInt();
        createPrintInt();
        createPrintBool();
        createReadInt();

        m_collecting = true;
        ast->accept(*this);
        m_collecting = false;
        ast->accept(*this);

        bool hasError = llvm::verifyModule(*m_module, &llvm::errs());
        if (hasError)
        {
            std::cerr << "Module verification failed" << std::endl;
            return false;
        }
        return true;
    }




    // BUILTIN FUNCTIONS ==================================================================================================

    void IRGenerator::declareSysWrappers()
    {
        llvm::Type* i32Ty = llvm::Type::getInt32Ty(m_context);
        llvm::Type* i8PtrTy = llvm::PointerType::getUnqual(m_context);

        llvm::FunctionType* writeType = llvm::FunctionType::get(i32Ty, {i32Ty, i8PtrTy, i32Ty}, false);
        m_sysWriteFunc = llvm::Function::Create(writeType, llvm::Function::ExternalLinkage, "__sys_write", m_module);

        llvm::FunctionType* readType = llvm::FunctionType::get(i32Ty, {i32Ty, i8PtrTy, i32Ty}, false);
        m_sysReadFunc = llvm::Function::Create(readType, llvm::Function::ExternalLinkage, "__sys_read", m_module);
    }

    void IRGenerator::createIntToString()
    {
        llvm::Type* i32Ty = llvm::Type::getInt32Ty(m_context);
        llvm::Type* i8Ty = llvm::Type::getInt8Ty(m_context);
        llvm::Type* i8PtrTy = llvm::PointerType::getUnqual(m_context);

        // int __int_to_str(int val, char* buf)
        llvm::FunctionType* funcType = llvm::FunctionType::get(i32Ty, {i32Ty, i8PtrTy}, false);
        m_intToStrFunc = llvm::Function::Create(funcType, llvm::Function::InternalLinkage, "__int_to_str", m_module);


        llvm::BasicBlock* entryBB       = llvm::BasicBlock::Create(m_context, "entry", m_intToStrFunc);
        llvm::BasicBlock* negCheckBB    = llvm::BasicBlock::Create(m_context, "negcheck", m_intToStrFunc);
        llvm::BasicBlock* zeroBB        = llvm::BasicBlock::Create(m_context, "zero", m_intToStrFunc);
        llvm::BasicBlock* nonZeroBB     = llvm::BasicBlock::Create(m_context, "nonzero", m_intToStrFunc);
        llvm::BasicBlock* loopHeaderBB  = llvm::BasicBlock::Create(m_context, "loopheader", m_intToStrFunc);
        llvm::BasicBlock* loopBodyBB    = llvm::BasicBlock::Create(m_context, "loopbody", m_intToStrFunc);
        llvm::BasicBlock* loopEndBB     = llvm::BasicBlock::Create(m_context, "loopend", m_intToStrFunc);
        llvm::BasicBlock* afterGenBB    = llvm::BasicBlock::Create(m_context, "aftergen", m_intToStrFunc);
        llvm::BasicBlock* negBB         = llvm::BasicBlock::Create(m_context, "neg", m_intToStrFunc);
        llvm::BasicBlock* posBB         = llvm::BasicBlock::Create(m_context, "pos", m_intToStrFunc);
        llvm::BasicBlock* copyHeaderBB  = llvm::BasicBlock::Create(m_context, "copyheader", m_intToStrFunc);
        llvm::BasicBlock* copyBodyBB    = llvm::BasicBlock::Create(m_context, "copybody", m_intToStrFunc);
        llvm::BasicBlock* copyEndBB     = llvm::BasicBlock::Create(m_context, "copyend", m_intToStrFunc);
        llvm::BasicBlock* returnBB      = llvm::BasicBlock::Create(m_context, "return", m_intToStrFunc);


        m_builder.SetInsertPoint(entryBB);
        auto args = m_intToStrFunc->arg_begin();
        llvm::Value* val = args++;
        llvm::Value* buf = args++;

        llvm::Value* zero = llvm::ConstantInt::get(i32Ty, 0);
        llvm::Value* one = llvm::ConstantInt::get(i32Ty, 1);
        llvm::Value* ten = llvm::ConstantInt::get(i32Ty, 10);
        llvm::Value* charZero = llvm::ConstantInt::get(i8Ty, '0');
        llvm::Value* charMinus = llvm::ConstantInt::get(i8Ty, '-');

        // negative?
        llvm::Value* isNeg = m_builder.CreateICmpSLT(val, zero, "isneg");
        llvm::Value* absVal = m_builder.CreateSelect(isNeg, m_builder.CreateNeg(val), val, "absval");

        // zero?
        llvm::Value* isZero = m_builder.CreateICmpEQ(absVal, zero, "iszero");
        m_builder.CreateCondBr(isZero, zeroBB, negCheckBB);

        // zeroBB
        m_builder.SetInsertPoint(zeroBB);
        m_builder.CreateStore(charZero, m_builder.CreateGEP(i8Ty, buf, zero));
        m_builder.CreateBr(returnBB);

        // negCheckBB
        m_builder.SetInsertPoint(negCheckBB);

        // temp buffer
        llvm::AllocaInst* temp = m_builder.CreateAlloca(llvm::ArrayType::get(i8Ty, 12), nullptr, "temp");
        llvm::Value* tempPtr = m_builder.CreateBitCast(temp, i8PtrTy);
        m_builder.CreateBr(nonZeroBB);

        // nonZeroBB
        m_builder.SetInsertPoint(nonZeroBB);
        m_builder.CreateBr(loopHeaderBB);

        // loopHeaderBB
        m_builder.SetInsertPoint(loopHeaderBB);
        llvm::PHINode* phiVal = m_builder.CreatePHI(i32Ty, 2, "val");
        phiVal->addIncoming(absVal, nonZeroBB);
        llvm::PHINode* phiIdx = m_builder.CreatePHI(i32Ty, 2, "idx");
        phiIdx->addIncoming(zero, nonZeroBB);
        llvm::Value* loopCond = m_builder.CreateICmpUGT(phiVal, zero, "loopcond");
        m_builder.CreateCondBr(loopCond, loopBodyBB, loopEndBB);

        // loopBodyBB
        m_builder.SetInsertPoint(loopBodyBB);
        llvm::Value* digit = m_builder.CreateURem(phiVal, ten, "digit");
        llvm::Value* ch = m_builder.CreateAdd(charZero, m_builder.CreateTrunc(digit, i8Ty), "ch");
        llvm::Value* tempElem = m_builder.CreateGEP(i8Ty, tempPtr, phiIdx);
        m_builder.CreateStore(ch, tempElem);
        llvm::Value* nextVal = m_builder.CreateUDiv(phiVal, ten, "nextval");
        llvm::Value* nextIdx = m_builder.CreateAdd(phiIdx, one, "nextidx");
        phiVal->addIncoming(nextVal, loopBodyBB);
        phiIdx->addIncoming(nextIdx, loopBodyBB);
        m_builder.CreateBr(loopHeaderBB);

        // loopEndBB
        m_builder.SetInsertPoint(loopEndBB);
        llvm::Value* numDigits = phiIdx;
        m_builder.CreateBr(afterGenBB);

        // afterGenBB
        m_builder.SetInsertPoint(afterGenBB);
        m_builder.CreateCondBr(isNeg, negBB, posBB);

        // negBB
        m_builder.SetInsertPoint(negBB);
        m_builder.CreateStore(charMinus, m_builder.CreateGEP(i8Ty, buf, zero));
        llvm::Value* bufAfterMinus = m_builder.CreateGEP(i8Ty, buf, one, "bufafterminus");
        m_builder.CreateBr(copyHeaderBB);

        // posBB
        m_builder.SetInsertPoint(posBB);
        llvm::Value* bufNoMinus = buf;
        m_builder.CreateBr(copyHeaderBB);

        // copyHeaderBB
        m_builder.SetInsertPoint(copyHeaderBB);
        llvm::PHINode* phiDest = m_builder.CreatePHI(i8PtrTy, 2, "dest");
        phiDest->addIncoming(bufAfterMinus, negBB);
        phiDest->addIncoming(bufNoMinus, posBB);
        llvm::PHINode* phiSrcIdx = m_builder.CreatePHI(i32Ty, 2, "srcidx");
        phiSrcIdx->addIncoming(numDigits, negBB);
        phiSrcIdx->addIncoming(numDigits, posBB);
        llvm::PHINode* phiRemaining = m_builder.CreatePHI(i32Ty, 2, "remaining");
        phiRemaining->addIncoming(numDigits, negBB);
        phiRemaining->addIncoming(numDigits, posBB);
        llvm::Value* copyCond = m_builder.CreateICmpUGT(phiRemaining, zero, "copycond");
        m_builder.CreateCondBr(copyCond, copyBodyBB, copyEndBB);

        // copyBodyBB
        m_builder.SetInsertPoint(copyBodyBB);
        llvm::Value* srcIdx = m_builder.CreateSub(phiSrcIdx, one, "srcidxm1");
        llvm::Value* srcCharPtr = m_builder.CreateGEP(i8Ty, tempPtr, srcIdx);
        llvm::Value* srcChar = m_builder.CreateLoad(i8Ty, srcCharPtr, "srcchar");
        m_builder.CreateStore(srcChar, phiDest);
        llvm::Value* nextDest = m_builder.CreateGEP(i8Ty, phiDest, one, "nextdest");
        llvm::Value* nextRemaining = m_builder.CreateSub(phiRemaining, one, "nextremaining");
        phiDest->addIncoming(nextDest, copyBodyBB);
        phiSrcIdx->addIncoming(srcIdx, copyBodyBB);
        phiRemaining->addIncoming(nextRemaining, copyBodyBB);
        m_builder.CreateBr(copyHeaderBB);

        // copyEndBB
        m_builder.SetInsertPoint(copyEndBB);
        llvm::Value* extra = m_builder.CreateZExt(isNeg, i32Ty);
        llvm::Value* finalLen = m_builder.CreateAdd(numDigits, extra, "finallen");
        m_builder.CreateBr(returnBB);

        // returnBB
        m_builder.SetInsertPoint(returnBB);
        llvm::PHINode* phiRetLen = m_builder.CreatePHI(i32Ty, 2, "retlen");
        phiRetLen->addIncoming(one, zeroBB);
        phiRetLen->addIncoming(finalLen, copyEndBB);
        m_builder.CreateRet(phiRetLen);
    }

    void IRGenerator::createStringToInt()
    {
        llvm::Type* i32Ty = llvm::Type::getInt32Ty(m_context);
        llvm::Type* i8Ty = llvm::Type::getInt8Ty(m_context);
        llvm::Type* i8PtrTy = llvm::PointerType::getUnqual(m_context);
        llvm::Type* i1Ty = llvm::Type::getInt1Ty(m_context);

        // int __str_to_int(char* buf, int len)
        llvm::FunctionType* funcType = llvm::FunctionType::get(i32Ty, {i8PtrTy, i32Ty}, false);
        m_strToIntFunc = llvm::Function::Create(funcType, llvm::Function::InternalLinkage, "__str_to_int", m_module);


        llvm::BasicBlock* entryBB       = llvm::BasicBlock::Create(m_context, "entry", m_strToIntFunc);
        llvm::BasicBlock* checkSignBB   = llvm::BasicBlock::Create(m_context, "checksign", m_strToIntFunc);
        llvm::BasicBlock* positiveBB    = llvm::BasicBlock::Create(m_context, "positive", m_strToIntFunc);
        llvm::BasicBlock* negativeBB    = llvm::BasicBlock::Create(m_context, "negative", m_strToIntFunc);
        llvm::BasicBlock* negValidBB    = llvm::BasicBlock::Create(m_context, "negvalid", m_strToIntFunc);
        llvm::BasicBlock* negInvalidBB  = llvm::BasicBlock::Create(m_context, "neginvalid", m_strToIntFunc);
        llvm::BasicBlock* loopHeaderBB  = llvm::BasicBlock::Create(m_context, "loopheader", m_strToIntFunc);
        llvm::BasicBlock* loopBodyBB    = llvm::BasicBlock::Create(m_context, "loopbody", m_strToIntFunc);
        llvm::BasicBlock* loopEndBB     = llvm::BasicBlock::Create(m_context, "loopend", m_strToIntFunc);
        llvm::BasicBlock* posEndBB      = llvm::BasicBlock::Create(m_context, "posend", m_strToIntFunc);
        llvm::BasicBlock* negEndBB      = llvm::BasicBlock::Create(m_context, "negend", m_strToIntFunc);
        llvm::BasicBlock* returnBB      = llvm::BasicBlock::Create(m_context, "return", m_strToIntFunc);

        // entryBB
        m_builder.SetInsertPoint(entryBB);
        auto args = m_strToIntFunc->arg_begin();
        llvm::Value* buf = args++;
        llvm::Value* len = args++;

        llvm::Value* zero = llvm::ConstantInt::get(i32Ty, 0);
        llvm::Value* one = llvm::ConstantInt::get(i32Ty, 1);
        llvm::Value* ten = llvm::ConstantInt::get(i32Ty, 10);
        llvm::Value* charZero = llvm::ConstantInt::get(i8Ty, '0');
        llvm::Value* charMinus = llvm::ConstantInt::get(i8Ty, '-');

        llvm::Value* isEmpty = m_builder.CreateICmpEQ(len, zero, "isempty");
        m_builder.CreateCondBr(isEmpty, returnBB, checkSignBB);

        // checkSignBB
        m_builder.SetInsertPoint(checkSignBB);
        llvm::Value* firstCharPtr = m_builder.CreateGEP(i8Ty, buf, zero);
        llvm::Value* firstChar = m_builder.CreateLoad(i8Ty, firstCharPtr, "firstchar");
        llvm::Value* isMinus = m_builder.CreateICmpEQ(firstChar, charMinus, "isminus");
        m_builder.CreateCondBr(isMinus, negativeBB, positiveBB);

        // positiveB
        m_builder.SetInsertPoint(positiveBB);
        m_builder.CreateBr(loopHeaderBB);

        // negativeBB
        m_builder.SetInsertPoint(negativeBB);
        llvm::Value* lenMinusOne = m_builder.CreateSub(len, one, "lenminusone");
        llvm::Value* isValidNeg = m_builder.CreateICmpSGT(len, one, "isvalidneg");
        m_builder.CreateCondBr(isValidNeg, negValidBB, negInvalidBB);

        // negValidBB
        m_builder.SetInsertPoint(negValidBB);
        m_builder.CreateBr(loopHeaderBB);

        // negInvalidBB 
        m_builder.SetInsertPoint(negInvalidBB);
        m_builder.CreateBr(returnBB);

        // loopHeaderBB 
        m_builder.SetInsertPoint(loopHeaderBB);

        // phi node for index
        llvm::PHINode* phiI = m_builder.CreatePHI(i32Ty, 2, "i");
        phiI->addIncoming(zero, positiveBB);
        phiI->addIncoming(one, negValidBB);

        // phi for remaining
        llvm::PHINode* phiRemaining = m_builder.CreatePHI(i32Ty, 2, "remaining");
        phiRemaining->addIncoming(len, positiveBB);
        phiRemaining->addIncoming(len, negValidBB);

        // phi for result
        llvm::PHINode* phiRes = m_builder.CreatePHI(i32Ty, 2, "res");
        phiRes->addIncoming(zero, positiveBB);
        phiRes->addIncoming(zero, negValidBB);

        // phi for sign
        llvm::PHINode* phiIsMinus = m_builder.CreatePHI(i1Ty, 2, "isminus");
        phiIsMinus->addIncoming(llvm::ConstantInt::getFalse(m_context), positiveBB);
        phiIsMinus->addIncoming(llvm::ConstantInt::getTrue(m_context), negValidBB);

        llvm::Value* loopCond = m_builder.CreateICmpSLT(phiI, phiRemaining, "loopcond");
        m_builder.CreateCondBr(loopCond, loopBodyBB, loopEndBB);

        // loopBodyBB
        m_builder.SetInsertPoint(loopBodyBB);
        llvm::Value* charPtr = m_builder.CreateGEP(i8Ty, buf, phiI);
        llvm::Value* ch = m_builder.CreateLoad(i8Ty, charPtr, "ch");
        llvm::Value* digit = m_builder.CreateSub(m_builder.CreateZExt(ch, i32Ty),
                                                m_builder.CreateZExt(charZero, i32Ty), "digit");
        llvm::Value* newRes = m_builder.CreateAdd(m_builder.CreateMul(phiRes, ten), digit, "newres");
        llvm::Value* nextI = m_builder.CreateAdd(phiI, one, "nexti");
        phiI->addIncoming(nextI, loopBodyBB);
        phiRes->addIncoming(newRes, loopBodyBB);
        phiRemaining->addIncoming(phiRemaining, loopBodyBB);
        phiIsMinus->addIncoming(phiIsMinus, loopBodyBB);
        m_builder.CreateBr(loopHeaderBB);

        // loopEndBB
        m_builder.SetInsertPoint(loopEndBB);
        llvm::Value* posResult = phiRes;
        m_builder.CreateCondBr(phiIsMinus, negEndBB, posEndBB);

        // posEndBB 
        m_builder.SetInsertPoint(posEndBB);
        m_builder.CreateBr(returnBB);

        // negEndBB
        m_builder.SetInsertPoint(negEndBB);
        llvm::Value* negResult = m_builder.CreateNeg(posResult, "negresult");
        m_builder.CreateBr(returnBB);

        // returnBB
        m_builder.SetInsertPoint(returnBB);
        llvm::PHINode* phiRet = m_builder.CreatePHI(i32Ty, 4, "retval");
        phiRet->addIncoming(zero, entryBB);       // empty
        phiRet->addIncoming(zero, negInvalidBB);  // "-"
        phiRet->addIncoming(posResult, posEndBB); // pos
        phiRet->addIncoming(negResult, negEndBB); // neg
        m_builder.CreateRet(phiRet);
    }


    void IRGenerator::createPrintInt()
    {
        llvm::Type* i32Ty = llvm::Type::getInt32Ty(m_context);
        llvm::Type* i8Ty = llvm::Type::getInt8Ty(m_context);
        llvm::Type* i8PtrTy = llvm::PointerType::getUnqual(m_context);

        // void @printInt(i32 %val)
        llvm::FunctionType* funcType = llvm::FunctionType::get(llvm::Type::getVoidTy(m_context), {i32Ty}, false);
        m_printIntFunc = llvm::Function::Create(funcType, llvm::Function::InternalLinkage, "printInt", m_module);
        auto* entry = llvm::BasicBlock::Create(m_context, "entry", m_printIntFunc);
        m_builder.SetInsertPoint(entry);

        auto args = m_printIntFunc->arg_begin();
        llvm::Value* val = args++;

        llvm::AllocaInst* buf = m_builder.CreateAlloca(llvm::ArrayType::get(i8Ty, 16), nullptr, "buf");
        llvm::Value* bufPtr = m_builder.CreateBitCast(buf, i8PtrTy);

        
        llvm::Value* len = m_builder.CreateCall(m_intToStrFunc, {val, bufPtr}, "len");

        // add '\n'
        llvm::Value* newlinePtr = m_builder.CreateGEP(i8Ty, bufPtr, len);
        m_builder.CreateStore(llvm::ConstantInt::get(i8Ty, '\n'), newlinePtr);
        llvm::Value* lenWithNewline = m_builder.CreateAdd(len, llvm::ConstantInt::get(i32Ty, 1), "len_nl");

        // syswrite to stdout
        llvm::Value* fd = llvm::ConstantInt::get(i32Ty, 1);
        m_builder.CreateCall(m_sysWriteFunc, {fd, bufPtr, lenWithNewline});

        m_builder.CreateRetVoid();
    }


    void IRGenerator::createPrintBool()
    {
        llvm::Type* i32Ty = llvm::Type::getInt32Ty(m_context);
        llvm::Type* i1Ty = llvm::Type::getInt1Ty(m_context);
        llvm::Type* i8PtrTy = llvm::PointerType::getUnqual(m_context);

        // void @printBool(i1 %val)
        llvm::FunctionType* funcType = llvm::FunctionType::get(llvm::Type::getVoidTy(m_context), {i1Ty}, false);
        m_printBoolFunc = llvm::Function::Create(funcType, llvm::Function::InternalLinkage, "printBool", m_module);
        auto* entry = llvm::BasicBlock::Create(m_context, "entry", m_printBoolFunc);
        m_builder.SetInsertPoint(entry);

        auto args = m_printBoolFunc->arg_begin();
        llvm::Value* val = args++;


        llvm::Constant* trueStr = llvm::ConstantDataArray::getString(m_context, "true\n");
        llvm::Constant* falseStr = llvm::ConstantDataArray::getString(m_context, "false\n");

        auto trueGlobal = new llvm::GlobalVariable(*m_module, trueStr->getType(), true,
                                                llvm::GlobalValue::PrivateLinkage, trueStr, "str_true");
        auto falseGlobal = new llvm::GlobalVariable(*m_module, falseStr->getType(), true,
                                                    llvm::GlobalValue::PrivateLinkage, falseStr, "str_false");

        llvm::Value* truePtr = llvm::ConstantExpr::getBitCast(trueGlobal, i8PtrTy);
        llvm::Value* falsePtr = llvm::ConstantExpr::getBitCast(falseGlobal, i8PtrTy);

        llvm::Value* strPtr = m_builder.CreateSelect(val, truePtr, falsePtr, "strptr");
        llvm::Value* len = llvm::ConstantInt::get(i32Ty, 5);

        llvm::Value* fd = llvm::ConstantInt::get(i32Ty, 1);
        m_builder.CreateCall(m_sysWriteFunc, {fd, strPtr, len});

        m_builder.CreateRetVoid();
    }


    void IRGenerator::createReadInt()
    {
        llvm::Type* i32Ty = llvm::Type::getInt32Ty(m_context);
        llvm::Type* i8Ty = llvm::Type::getInt8Ty(m_context);
        llvm::Type* i8PtrTy = llvm::PointerType::getUnqual(m_context);

        // i32 @readInt()
        llvm::FunctionType* funcType = llvm::FunctionType::get(i32Ty, {}, false);
        m_readIntFunc = llvm::Function::Create(funcType, llvm::Function::InternalLinkage, "readInt", m_module);
        auto* entry = llvm::BasicBlock::Create(m_context, "entry", m_readIntFunc);
        m_builder.SetInsertPoint(entry);


        llvm::AllocaInst* buf = m_builder.CreateAlloca(llvm::ArrayType::get(i8Ty, 32), nullptr, "buf");
        llvm::Value* bufPtr = m_builder.CreateBitCast(buf, i8PtrTy);

        // from stdin
        llvm::Value* fd = llvm::ConstantInt::get(i32Ty, 0);
        llvm::Value* maxLen = llvm::ConstantInt::get(i32Ty, 32);
        llvm::Value* bytesRead = m_builder.CreateCall(m_sysReadFunc, {fd, bufPtr, maxLen}, "bytes_read");

        llvm::Value* lenWithoutNewline = m_builder.CreateSub(bytesRead, llvm::ConstantInt::get(i32Ty, 1), "len");

        llvm::Value* result = m_builder.CreateCall(m_strToIntFunc, {bufPtr, lenWithoutNewline}, "result");

        m_builder.CreateRet(result);
    }

    // ================================================================================================================









    void IRGenerator::printModule()
    {
        m_module->print(llvm::outs(), nullptr);
    }

    // Visitor implementation

    void IRGenerator::visit(Program& node)
    {
        for (auto& decl : node.declarations) decl->accept(*this);
        for (auto& stmt : node.statements) stmt->accept(*this);
    }

    void IRGenerator::visit(VarDecl& node)
    {
        if (m_collecting) return;
 

        if (!m_currentFunction)
        {
            std::cerr << "VarDecl outside function not supported yet" << std::endl;
            return;
        }

        llvm::Type* llvmType = nullptr;
        if (node.type == Type::INT)
            llvmType = llvm::Type::getInt32Ty(m_context);
        else if (node.type == Type::BOOL)
            llvmType = llvm::Type::getInt1Ty(m_context);
        else
        {
            std::cerr << "Unknown variable type" << std::endl;
            return;
        }

        auto savedIP = m_builder.saveIP();
        m_builder.SetInsertPoint(m_currentEntryBlock);
        llvm::AllocaInst* alloca = m_builder.CreateAlloca(llvmType, nullptr, node.name);
        m_builder.restoreIP(savedIP);

        if (node.symbol)
            m_allocaMap[node.symbol] = alloca;
        else
            std::cerr << "VarDecl without symbol entry" << std::endl;

        if (node.initializer)
        {
            node.initializer->accept(*this);
            llvm::Value* initVal = popValue();
            if (initVal)
                m_builder.CreateStore(initVal, alloca);
        }
    }

    void IRGenerator::visit(FuncDecl& node)
    {
        if (m_collecting)
        {
            llvm::Type* retType = (node.returnType == Type::INT) ?
                llvm::Type::getInt32Ty(m_context) : llvm::Type::getInt1Ty(m_context);

            std::vector<llvm::Type*> paramTypes;
            for (auto& param : node.parameters)
            {
                if (param->type == Type::INT)
                    paramTypes.push_back(llvm::Type::getInt32Ty(m_context));
                else
                    paramTypes.push_back(llvm::Type::getInt1Ty(m_context));
            }

            llvm::FunctionType* funcType = llvm::FunctionType::get(retType, paramTypes, false);

            std::string funcName = (node.name == "main") ? "_main" : node.name;
            llvm::Function* func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, funcName, m_module);

            m_functionMap[node.symbol] = func;
            return;
        }

        auto it = m_functionMap.find(node.symbol);
        if (it == m_functionMap.end())
        {
            std::cerr << "Function not found: " << node.name << std::endl;
            return;
        }
        llvm::Function* func = it->second;

        llvm::BasicBlock* entry = llvm::BasicBlock::Create(m_context, "entry", func);
        m_builder.SetInsertPoint(entry);
        m_currentFunction = func;
        m_currentEntryBlock = entry;

        auto argIt = func->arg_begin();
        for (size_t i = 0; i < node.parameters.size(); ++i, ++argIt)
        {
            VarDecl* paramDecl = node.parameters[i].get();
            argIt->setName(paramDecl->name);

            llvm::AllocaInst* alloca = createAllocaInEntry(argIt->getType(), paramDecl->name);

            m_builder.CreateStore(argIt, alloca);

            m_allocaMap[paramDecl->symbol] = alloca;
        }

        if (node.body) node.body->accept(*this);

        if (!m_builder.GetInsertBlock()->getTerminator())
        {
            if (node.returnType == Type::INT)
                m_builder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_context), 0));
            else
                m_builder.CreateRet(llvm::ConstantInt::getBool(m_context, false));
        }

        m_currentFunction = nullptr;
        m_currentEntryBlock = nullptr;
    }

    void IRGenerator::visit(Assignment& node)
    {
        if (m_collecting)
        {
            if (node.target) node.target->accept(*this);
            if (node.expr) node.expr->accept(*this);
            return;
        }

        node.expr->accept(*this);
        llvm::Value* val = popValue();
        if (!val) return;

        auto it = m_allocaMap.find(node.target->symbol);
        if (it == m_allocaMap.end())
        {
            std::cerr << "Assignment to unknown variable: " << node.target->toString() << std::endl;
            return;
        }
        m_builder.CreateStore(val, it->second);
    }

    void IRGenerator::visit(IfStmt& node)
    {
        if (m_collecting)
        {
            if (node.condition) node.condition->accept(*this);
            if (node.thenBranch) node.thenBranch->accept(*this);
            if (node.elseBranch) node.elseBranch->accept(*this);
            return;
        }

        node.condition->accept(*this);
        llvm::Value* cond = popValue();
        if (!cond) return;

        llvm::Function* func = m_builder.GetInsertBlock()->getParent();
        llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(m_context, "then", func);
        llvm::BasicBlock* elseBB = nullptr;
        llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(m_context, "ifmerge", func);

        if (node.elseBranch)
        {
            elseBB = llvm::BasicBlock::Create(m_context, "else", func);
            m_builder.CreateCondBr(cond, thenBB, elseBB);
        }
        else
        {
            m_builder.CreateCondBr(cond, thenBB, mergeBB);
        }

        m_builder.SetInsertPoint(thenBB);
        node.thenBranch->accept(*this);
        if (!m_builder.GetInsertBlock()->getTerminator())
        {
            m_builder.CreateBr(mergeBB);
        }

        if (elseBB)
        {
            m_builder.SetInsertPoint(elseBB);
            node.elseBranch->accept(*this);
            if (!m_builder.GetInsertBlock()->getTerminator())
            {
                m_builder.CreateBr(mergeBB);
            }
        }

        m_builder.SetInsertPoint(mergeBB);
    }

    void IRGenerator::visit(WhileStmt& node)
    {
        if (m_collecting)
        {
            if (node.condition) node.condition->accept(*this);
            if (node.body) node.body->accept(*this);
            return;
        }

        llvm::Function* func = m_builder.GetInsertBlock()->getParent();
        llvm::BasicBlock* condBB = llvm::BasicBlock::Create(m_context, "whilecond", func);
        llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(m_context, "whilebody", func);
        llvm::BasicBlock* afterBB = llvm::BasicBlock::Create(m_context, "whileafter", func);

        m_builder.CreateBr(condBB);

        m_builder.SetInsertPoint(condBB);
        node.condition->accept(*this);
        llvm::Value* cond = popValue();
        if (!cond) cond = llvm::ConstantInt::getBool(m_context, false);
        m_builder.CreateCondBr(cond, bodyBB, afterBB);

        m_builder.SetInsertPoint(bodyBB);
        node.body->accept(*this);
        if (!m_builder.GetInsertBlock()->getTerminator())
        {
            m_builder.CreateBr(condBB);
        }

        m_builder.SetInsertPoint(afterBB);
    }

    void IRGenerator::visit(Block& node)
    {
        for (auto& decl : node.declarations) decl->accept(*this);
        for (auto& stmt : node.statements) stmt->accept(*this);
    }

    void IRGenerator::visit(BinaryOperation& node)
    {
        if (m_collecting)
        {
            if (node.left) node.left->accept(*this);
            if (node.right) node.right->accept(*this);
            return;
        }
        using Op = BinaryOperation::Op;


        if (node.op == Op::AND || node.op == Op::OR)
        {
            node.left->accept(*this);
            llvm::Value* leftVal = popValue();
            node.right->accept(*this);
            llvm::Value* rightVal = popValue();
            if (!leftVal || !rightVal)
            {
                pushValue(llvm::ConstantInt::getBool(m_context, false));
                return;
            }
            llvm::Value* result = nullptr;
            if (node.op == Op::AND)
            {
                result = m_builder.CreateAnd(leftVal, rightVal, "andtmp");
            }
            else
            {
                result = m_builder.CreateOr(leftVal, rightVal, "ortmp");
            }
            pushValue(result);
            return;
        }

        // binary operations
        node.left->accept(*this);
        node.right->accept(*this);

        llvm::Value* rightVal = popValue();
        llvm::Value* leftVal = popValue();

        if (!leftVal || !rightVal)
        {
            pushValue(nullptr);
            return;
        }

        llvm::Value* result = nullptr;

        switch (node.op)
        {
            case Op::PLUS:
                result = m_builder.CreateAdd(leftVal, rightVal, "addtmp");
                break;
            case Op::MINUS:
                result = m_builder.CreateSub(leftVal, rightVal, "subtmp");
                break;
            case Op::MUL:
                result = m_builder.CreateMul(leftVal, rightVal, "multmp");
                break;
            case Op::DIV:
                result = m_builder.CreateSDiv(leftVal, rightVal, "divtmp");
                break;
            case Op::MOD:
                result = m_builder.CreateSRem(leftVal, rightVal, "modtmp");
                break;
            case Op::EQ:
                result = m_builder.CreateICmpEQ(leftVal, rightVal, "eqtmp");
                break;
            case Op::NE:
                result = m_builder.CreateICmpNE(leftVal, rightVal, "netmp");
                break;
            case Op::LT:
                result = m_builder.CreateICmpSLT(leftVal, rightVal, "lttmp");
                break;
            case Op::GT:
                result = m_builder.CreateICmpSGT(leftVal, rightVal, "gttmp");
                break;
            case Op::LE:
                result = m_builder.CreateICmpSLE(leftVal, rightVal, "letmp");
                break;
            case Op::GE:
                result = m_builder.CreateICmpSGE(leftVal, rightVal, "getmp");
                break;
            default:
                break;
        }

        pushValue(result);
    }

    void IRGenerator::visit(UnaryOperation& node)
    {
        if (m_collecting)
        {
            if (node.operand) node.operand->accept(*this);
            return;
        }
        node.operand->accept(*this);
        llvm::Value* operand = popValue();
        if (!operand)
        {
            pushValue(nullptr);
            return;
        }

        llvm::Value* result = nullptr;
        if (node.op == UnaryOperation::Op::NEG)
        {
            result = m_builder.CreateNeg(operand, "negtmp");
        }
        else if (node.op == UnaryOperation::Op::NOT)
        {
            result = m_builder.CreateNot(operand, "nottmp");
        }

        pushValue(result);
    }

    void IRGenerator::visit(IntegerLiteral& node)
    {
        if (m_collecting) return;
        llvm::Value* val = llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_context), node.value);
        pushValue(val);
    }

    void IRGenerator::visit(BooleanLiteral& node)
    {
        if (m_collecting) return;
        llvm::Value* val = llvm::ConstantInt::getBool(m_context, node.value);
        pushValue(val);
    }

    void IRGenerator::visit(Variable& node)
    {
        if (m_collecting) return;
        auto it = m_allocaMap.find(node.symbol);
        if (it == m_allocaMap.end())
        {
            std::cerr << "Variable '" << node.name << "' has no alloca!" << std::endl;
            pushValue(nullptr);
            return;
        }
        llvm::AllocaInst* alloca = it->second;
        llvm::Value* val = m_builder.CreateLoad(alloca->getAllocatedType(), alloca, node.name);
        pushValue(val);
    }

    void IRGenerator::visit(QualifiedIdentifier& node)
    {
        if (m_collecting) return;
        auto it = m_allocaMap.find(node.symbol);
        if (it == m_allocaMap.end())
        {
            std::cerr << "Qualified identifier '" << node.toString() << "' has no alloca!" << std::endl;
            pushValue(nullptr);
            return;
        }
        llvm::AllocaInst* alloca = it->second;
        llvm::Value* val = m_builder.CreateLoad(alloca->getAllocatedType(), alloca, node.toString());
        pushValue(val);
    }

    void IRGenerator::visit(TypeInt&) {}
    void IRGenerator::visit(TypeBool&) {}

    void IRGenerator::visit(DeclList& node)
    {
        for (auto& decl : node.declarations) decl->accept(*this);
    }

    void IRGenerator::visit(StmtList& node)
    {
        for (auto& stmt : node.statements) stmt->accept(*this);
    }

    void IRGenerator::visit(DeclOrStmtList& node)
    {
        for (auto& decl : node.declarations) decl->accept(*this);
        for (auto& stmt : node.statements) stmt->accept(*this);
    }

    void IRGenerator::visit(NamespaceDecl& node)
    {
        for (auto& decl : node.declarations) decl->accept(*this);
        for (auto& stmt : node.statements) stmt->accept(*this);
    }

    void IRGenerator::visit(CallExpr& node)
    {
        if (m_collecting)
        {
            node.callee->accept(*this);
            for (auto& arg : node.arguments)
                arg->accept(*this);
            return;
        }

        std::string calleeName = node.callee->toString();

        // Встроенные функции
        if (calleeName == "printInt")
        {
            if (node.arguments.size() != 1)
            {
                std::cerr << "printInt expects 1 argument" << std::endl;
                pushValue(nullptr);
                return;
            }
            node.arguments[0]->accept(*this);
            llvm::Value* arg = popValue();
            if (!arg) { pushValue(nullptr); return; }
            m_builder.CreateCall(m_printIntFunc, {arg});
            pushValue(llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_context), 0));
            return;
        }
        else if (calleeName == "printBool")
        {
            if (node.arguments.size() != 1)
            {
                std::cerr << "printBool expects 1 argument" << std::endl;
                pushValue(nullptr);
                return;
            }
            node.arguments[0]->accept(*this);
            llvm::Value* arg = popValue();
            if (!arg) { pushValue(nullptr); return; }
            m_builder.CreateCall(m_printBoolFunc, {arg});
            pushValue(llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_context), 0));
            return;
        }
        else if (calleeName == "readInt")
        {
            if (!node.arguments.empty())
            {
                std::cerr << "readInt expects no arguments" << std::endl;
                pushValue(nullptr);
                return;
            }
            llvm::Value* val = m_builder.CreateCall(m_readIntFunc, {}, "readval");
            pushValue(val);
            return;
        }

        // Обычный вызов пользовательской функции
        auto it = m_functionMap.find(node.symbol);
        if (it == m_functionMap.end())
        {
            std::cerr << "Function not found: " << calleeName << std::endl;
            pushValue(nullptr);
            return;
        }
        llvm::Function* callee = it->second;

        std::vector<llvm::Value*> args;
        for (auto& arg : node.arguments)
        {
            arg->accept(*this);
            llvm::Value* argVal = popValue();
            if (!argVal)
            {
                pushValue(nullptr);
                return;
            }
            args.push_back(argVal);
        }

        llvm::Value* result = m_builder.CreateCall(callee, args, calleeName + "_call");
        pushValue(result);
    }

    void IRGenerator::visit(ReturnStmt& node)
    {
        if (m_collecting) return;
        node.value->accept(*this);
        llvm::Value* val = popValue();
        if (val)
            m_builder.CreateRet(val);
    }

    void IRGenerator::visit(ExpressionStmt& node)
    {
        if (m_collecting)
        {
            node.expr->accept(*this);
            return;
        }
        node.expr->accept(*this);
        popValue();
    }

    void IRGenerator::visit(ParameterList& node)
    {

    }

    void IRGenerator::visit(ArgumentList& node)
    {

    }


} // namespace minilang
