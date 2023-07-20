#ifndef AST_HH
#define AST_HH

#include <llvm/IR/Value.h>
#include <string>
#include <vector>

struct LLVMCompiler;

/**
Base node class. Defined as `abstract`.
*/
struct Node {
    enum NodeType {
        BIN_OP, INT_LIT, STMTS, ASSN, DBG, IDENT, TERN_OP, DECL, ARGS, RET, IF_STMT, FUNC_DEFN, FUNC_CALL
    } type;
    int data_type;
    virtual std::string to_string() = 0;
    virtual llvm::Value *llvm_codegen(LLVMCompiler *compiler) = 0;
};


/**
    Node for idnetifiers
*/
struct NodeIdent : public Node {
    std::string identifier;
    int dtype;

    NodeIdent(std::string ident, int data_type);
    std::string to_string();
    llvm::Value *llvm_codegen(LLVMCompiler *compiler);
};

/**
    Node for list of statements
*/
struct NodeStmts : public Node {
    std::vector<Node*> list;

    NodeStmts();
    void push_back(Node *node);
    std::string to_string();
    llvm::Value *llvm_codegen(LLVMCompiler *compiler);
};

struct NodeArgs : public Node {
    std::vector<Node*> list;

    NodeArgs();
    void push_back(Node *node);
    std::string to_string();
    llvm::Value *llvm_codegen(LLVMCompiler *compiler);
};

/**
    Node for binary operations
*/
struct NodeBinOp : public Node {
    enum Op {
        PLUS, MINUS, MULT, DIV
    } op;

    Node *left, *right;

    NodeBinOp(Op op, Node *leftptr, Node *rightptr);
    std::string to_string();
    llvm::Value *llvm_codegen(LLVMCompiler *compiler);
};

struct NodeTernOp : public Node {
    enum Op {
        TERNOP
    } op;

    Node* left;
    Node* mid;
    Node* right;

    NodeTernOp(Op op, Node* leftptr, Node* midptr, Node* rightptr);
    std::string to_string();
    llvm::Value* llvm_codegen(LLVMCompiler *compiler);
};

struct NodeIf : public Node {
    Node* left;
    Node* middle;
    Node* right;

    NodeIf(Node* leftptr, Node* midptr, Node* rightptr);
    std::string to_string();
    llvm::Value* llvm_codegen(LLVMCompiler *compiler);
};

struct NodeFun: public Node {
    NodeArgs* args;
    NodeIdent* name;
    NodeStmts* stmts;

    NodeFun(NodeIdent* name, NodeArgs* args, NodeStmts* stmts);
    std::string to_string();
    llvm::Value* llvm_codegen(LLVMCompiler *compiler);
};

struct NodeFunCall: public Node {
    NodeArgs* args;
    NodeIdent* name;

    NodeFunCall(NodeIdent* name, NodeArgs* args);
    std::string to_string();
    llvm::Value* llvm_codegen(LLVMCompiler *compiler);
};

/**
    Node for integer literals
*/
struct NodeInt : public Node {
    int value;

    NodeInt(int val);
    std::string to_string();
    llvm::Value *llvm_codegen(LLVMCompiler *compiler);
};

/**
    Node for variable assignments
*/
struct NodeAssign : public Node {
    std::string identifier;
    Node *expression;

    NodeAssign(std::string id, std::string d_type, Node *expr);
    std::string to_string();
    llvm::Value *llvm_codegen(LLVMCompiler *compiler);
};

struct NodeDecl : public Node {
    std::string identifier;
    Node *expression;

    NodeDecl(std::string id, std::string data_type, Node *expr);
    std::string to_string();
    llvm::Value *llvm_codegen(LLVMCompiler *compiler);
};

/**
    Node for `dbg` statements
*/
struct NodeDebug : public Node {
    Node *expression;

    NodeDebug(Node *expr);
    std::string to_string();
    llvm::Value *llvm_codegen(LLVMCompiler *compiler);
};

struct NodeRet : public Node {
    Node *expression;

    NodeRet(Node *expr);
    std::string to_string();
    llvm::Value *llvm_codegen(LLVMCompiler *compiler);
};



#endif