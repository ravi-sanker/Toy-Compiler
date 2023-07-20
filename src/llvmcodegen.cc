#include "llvmcodegen.hh"
#include "ast.hh"
#include <iostream>
#include <llvm/Support/FileSystem.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/ADT/ArrayRef.h>
#include <vector>

#define MAIN_FUNC compiler->module.getFunction("main")
/*
The documentation for LLVM codegen, and how exactly this file works can be found
ins `docs/llvm.md`
*/

void LLVMCompiler::compile(Node *root) {
    /* Adding reference to print_i in the runtime library */
    // void printi();
    FunctionType *printi_func_type = FunctionType::get(
        builder.getVoidTy(),
        {builder.getInt64Ty()},
        false
    );
    Function::Create(
        printi_func_type,
        GlobalValue::ExternalLinkage,
        "printi",
        &module
    );
    /* we can get this later 
        module.getFunction("printi");
    */

    /* Main Function */
    // int main();
    FunctionType *main_func_type = FunctionType::get(
        builder.getInt32Ty(), {}, false /* is vararg */
    );
    Function *main_func = Function::Create(
        main_func_type,
        GlobalValue::ExternalLinkage,
        "main",
        &module
    );

    // create main function block
    BasicBlock *main_func_entry_bb = BasicBlock::Create(
        *context,
        "entry",
        main_func
    );
    main = main_func_entry_bb;
    curr_fun = "main";
    // move the builder to the start of the main function block
    builder.SetInsertPoint(main_func_entry_bb);

    root->llvm_codegen(this);

    // return 0;
    builder.CreateRet(builder.getInt32(0));
}

void LLVMCompiler::dump() {
    outs() << module;
}

void LLVMCompiler::write(std::string file_name) {
    std::error_code EC;
    raw_fd_ostream fout(file_name, EC, sys::fs::OF_None);
    WriteBitcodeToFile(module, fout);
    fout.flush();
    fout.close();
}

//  ┌―――――――――――――――――――――┐  //
//  │ AST -> LLVM Codegen │  //
// └―――――――――――――――――――――┘   //

// codegen for statements
Value *NodeStmts::llvm_codegen(LLVMCompiler *compiler) {
    Value *last = nullptr;
    for(auto node : list) {
        last = node->llvm_codegen(compiler);
    }

    return last;
}

Value *NodeDebug::llvm_codegen(LLVMCompiler *compiler) {
    Value *expr = expression->llvm_codegen(compiler);

    Function *printi_func = compiler->module.getFunction("printi");
    compiler->builder.CreateCall(printi_func, {expr});

    return expr;
}

Value *NodeInt::llvm_codegen(LLVMCompiler *compiler) {
    return compiler->builder.getInt64(value);
}

Value *NodeIf::llvm_codegen(LLVMCompiler *compiler) {
    Value* left_expr = left->llvm_codegen(compiler); 
    left_expr = compiler->builder.CreateICmpNE(left_expr, compiler->builder.getInt64(0), "condition");
    Function *funct = compiler->builder.GetInsertBlock()->getParent(); //
    BasicBlock *block1 = BasicBlock::Create(*compiler->context, "block1", funct);
    BasicBlock *block1r = BasicBlock::Create(*compiler->context, "block1_ret", funct);
    BasicBlock *block2 = BasicBlock::Create(*compiler->context, "block2", funct);
    BasicBlock *block2r = BasicBlock::Create(*compiler->context, "block2_ret", funct);
    BasicBlock *blockc = BasicBlock::Create(*compiler->context, "common", funct);
    compiler->builder.CreateCondBr(left_expr, block1, block2);
    compiler->builder.SetInsertPoint(block1);
    compiler->if_ret=block1r;
    middle->llvm_codegen(compiler);
    compiler->builder.CreateBr(blockc);
    block1 = compiler->builder.GetInsertBlock();
    if(compiler->emptyret){
        compiler->builder.SetInsertPoint(block1r);
        compiler->builder.CreateBr(blockc);
    }
    compiler->builder.SetInsertPoint(block2);
    compiler->if_ret=block2r;
    right->llvm_codegen(compiler);
    compiler->builder.CreateBr(blockc);
    block2 = compiler->builder.GetInsertBlock();
    if(compiler->emptyret){
        compiler->builder.SetInsertPoint(block2r);
        compiler->builder.CreateBr(blockc);
    }
    compiler->builder.SetInsertPoint(blockc);
    return NULL;
}

Value *NodeFun::llvm_codegen(LLVMCompiler *compiler) {
    compiler->isret=0;
    if(name->identifier=="main"){
        return stmts->llvm_codegen(compiler);
    }
    std::vector<Type*> Ints(args->list.size(),Type::getInt64Ty(*compiler->context));
    FunctionType *type = FunctionType::get(
        compiler->builder.getInt64Ty(), Ints, false
    );
    Function *func = Function::Create(
        type,
        GlobalValue::ExternalLinkage,
        name->identifier,
        &compiler->module
    );
    unsigned Idx = 0;
    for (Function::arg_iterator AI = func->arg_begin(); Idx != args->list.size();++AI, ++Idx) {
        AI->setName(((NodeIdent*)args->list[Idx])->identifier);
    }
    BasicBlock *func_entry_bb = BasicBlock::Create(
        *compiler->context,
        name->identifier+"_entry",
        func
    );
    compiler->builder.SetInsertPoint(func_entry_bb);
    compiler->curr_fun = name->identifier;
    stmts->llvm_codegen(compiler);
    if(compiler->isret==0){
        compiler->builder.CreateRet(compiler->builder.getInt64(0));
    }
    compiler->isret=1;
    compiler->builder.SetInsertPoint(compiler->main);
    compiler->curr_fun = "main";
    return NULL;
}

Value *NodeFunCall::llvm_codegen(LLVMCompiler *compiler) {
    std::vector<Value*> Args;
    for(unsigned long i=0;i<args->list.size();i++){
        Value* v= args->list[i]->llvm_codegen(compiler);
        Args.push_back(v);
    }
    return compiler->builder.CreateCall(compiler->module.getFunction(name->identifier), Args);
}

Value *NodeArgs::llvm_codegen(LLVMCompiler*compiler){
    return NULL;
}

Value *NodeRet::llvm_codegen(LLVMCompiler *compiler) {
    auto x = compiler->builder.CreateRet(expression->llvm_codegen(compiler));
    compiler->builder.SetInsertPoint(compiler->if_ret);
    compiler->emptyret=0;
    return x;
}

Value *NodeBinOp::llvm_codegen(LLVMCompiler *compiler) {
    Value *left_expr = left->llvm_codegen(compiler);
    Value *right_expr = right->llvm_codegen(compiler);

    switch(op) {
        case PLUS:
        return compiler->builder.CreateAdd(left_expr, right_expr, "addtmp");
        case MINUS:
        return compiler->builder.CreateSub(left_expr, right_expr, "minustmp");
        case MULT:
        return compiler->builder.CreateMul(left_expr, right_expr, "multtmp");
        case DIV:
        return compiler->builder.CreateSDiv(left_expr, right_expr, "divtmp");
    }
}

Value* NodeTernOp::llvm_codegen(LLVMCompiler* compiler) {
    Value* left_expr = left->llvm_codegen(compiler); 
    Value* mid_expr = mid->llvm_codegen(compiler);
    Value* right_expr = right->llvm_codegen(compiler);

    left_expr = compiler->builder.CreateICmpNE(left_expr, compiler->builder.getInt64(0), "condition");
    Function *funct = compiler->builder.GetInsertBlock()->getParent(); //
    BasicBlock *block1 = BasicBlock::Create(*compiler->context, "block1", funct);
    BasicBlock *block2 = BasicBlock::Create(*compiler->context, "block2", funct);
    BasicBlock *blockc = BasicBlock::Create(*compiler->context, "common", funct);
    compiler->builder.CreateCondBr(left_expr, block1, block2);
    compiler->builder.SetInsertPoint(block1);
    compiler->builder.CreateBr(blockc);
    block1 = compiler->builder.GetInsertBlock();
    compiler->builder.SetInsertPoint(block2);
    compiler->builder.CreateBr(blockc);
    block2 = compiler->builder.GetInsertBlock();
    compiler->builder.SetInsertPoint(blockc);
    PHINode *phi = compiler->builder.CreatePHI(Type::getInt64Ty(*compiler->context), 2, "iftmp");
    phi->addIncoming(mid_expr, block1);
    phi->addIncoming(right_expr, block2);
    return phi;
}


Value *NodeAssign::llvm_codegen(LLVMCompiler *compiler) {
    Value *expr = expression->llvm_codegen(compiler);

    AllocaInst *alloc = compiler->locals[identifier] ;

    return compiler->builder.CreateStore(expr, alloc);
}

Value *NodeDecl::llvm_codegen(LLVMCompiler *compiler) {
    Value *expr = expression->llvm_codegen(compiler);

    IRBuilder<> temp_builder(
        &compiler->module.getFunction(compiler->curr_fun)->getEntryBlock(),
        compiler->module.getFunction(compiler->curr_fun)->getEntryBlock().begin()
    );

    AllocaInst *alloc = temp_builder.CreateAlloca(compiler->builder.getInt64Ty(), 0, identifier);

    compiler->locals[identifier] = alloc;

    return compiler->builder.CreateStore(expr, alloc);
}

Value *NodeIdent::llvm_codegen(LLVMCompiler *compiler) {
    unsigned Idx = 0;
    for (Function::arg_iterator AI = compiler->module.getFunction(compiler->curr_fun)->arg_begin(); AI!=compiler->module.getFunction(compiler->curr_fun)->arg_end();++AI, ++Idx) {
        if(identifier==AI->getName()) return AI;
    }
    AllocaInst *alloc = compiler->locals[identifier];

    // if your LLVM_MAJOR_VERSION >= 14
    return compiler->builder.CreateLoad(compiler->builder.getInt64Ty(), alloc, identifier);
}

#undef MAIN_FUNC