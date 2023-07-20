#include "ast.hh"

#include <string>
#include <vector>

extern int yyerror(std::string msg);

NodeBinOp::NodeBinOp(NodeBinOp::Op ope, Node *leftptr, Node *rightptr) {
    type = BIN_OP;
    op = ope;
    left = leftptr;
    right = rightptr;
}

std::string NodeBinOp::to_string() {
    std::string out = "(";
    switch(op) {
        case PLUS: out += '+'; break;
        case MINUS: out += '-'; break;
        case MULT: out += '*'; break;
        case DIV: out += '/'; break;
    }

    out += ' ' + left->to_string() + ' ' + right->to_string() + ')';

    return out;
}

NodeTernOp::NodeTernOp(NodeTernOp::Op ope, Node* leftptr, Node* midptr, Node* rightptr) {
    type = TERN_OP;
    op = ope;
    left = leftptr;
    mid = midptr;
    right = rightptr;
}

std::string NodeTernOp::to_string() {
    std::string out = "(?:";
    out += ' ' + left->to_string() + ' ' + mid->to_string() + ' ' + right->to_string() + ')';

    return out;
}

NodeIf::NodeIf(Node* leftptr,Node* midptr,Node* rightptr) {
    type = IF_STMT;
    left = leftptr;
    right = rightptr;
    middle = midptr;
}

std::string NodeIf::to_string() {
    std::string out = "(IF";
    out += " " + left->to_string();
    out += " " + middle->to_string();
    out += " " + right->to_string();
    out += ')';
    return out;
}

NodeFun::NodeFun(NodeIdent* nam, NodeArgs* arg, NodeStmts* stmt){
    name = nam;
    type =  FUNC_DEFN;
    args = arg;
    stmts = stmt;
}

std::string NodeFun::to_string() {
    std::string out = "(FUN";
    out += " " + name->to_string();
    out += " " + args->to_string();
    out += " " + stmts->to_string();
    out += ')';
    return out;
}

NodeFunCall::NodeFunCall(NodeIdent* nam, NodeArgs* arg){
    name = nam;
    args = arg;
    type = FUNC_CALL;
}

std::string NodeFunCall::to_string() {
    std::string out = "(CALL";
    out += " " + name->to_string();
    out += " " + args->to_string();
    out += ')';
    return out;
}

NodeInt::NodeInt(int val) {
    type = INT_LIT;
    value = val;
    if(val <= INT16_MAX)
        data_type = 1;
    else if(val <=  INT32_MAX)
        data_type = 2;
    else
        data_type = 3;
}

std::string NodeInt::to_string() {
    return std::to_string(value);
}

NodeStmts::NodeStmts() {
    type = STMTS;
    list = std::vector<Node*>();
}

void NodeStmts::push_back(Node *node) {
    list.push_back(node);
}

std::string NodeStmts::to_string() {
    std::string out = "(begin";
    for(auto i : list) {
        out += " " + i->to_string();
    }

    out += ')';

    return out;
}

NodeArgs::NodeArgs() {
    type = ARGS;
    list = std::vector<Node*>();
}

void NodeArgs::push_back(Node *node) {
    list.push_back(node);
}

std::string NodeArgs::to_string() {
    std::string out = "(args";
    for(auto i : list) {
        out += " " + i->to_string();
    }

    out += ')';

    return out;
}

NodeAssign::NodeAssign(std::string id, std::string dtype, Node *expr) {
    type = ASSN;
    identifier = id;
    expression = expr;
    if(dtype == "short" && expr->data_type == 1)
        data_type = 1;
    else if(dtype == "int" && expr->data_type <= 2)
        data_type = 2;
    else if(dtype == "long" && expr->data_type <= 3)
        data_type = 3;
    else
        yyerror("Type Mismatch");
    
}

std::string NodeAssign::to_string() {
    return "(" + identifier + " " + expression->to_string() + ")";
}

NodeDecl::NodeDecl(std::string id, std::string dtype, Node *expr) {
    type = DECL;
    identifier = id;
    expression = expr;
    if(dtype == "short" && expr->data_type == 1)
        data_type = 1;
    else if(dtype == "int" && expr->data_type <= 2)
        data_type = 2;
    else if(dtype == "long" && expr->data_type <= 3)
        data_type = 3;
    else
        yyerror("Type Mismatch");
    
}

std::string NodeDecl::to_string() {
    if(data_type == 1)
        return "(let short " + identifier + " " + expression->to_string() + ")";
    else if(data_type == 2)
        return "(let int " + identifier + " " + expression->to_string() + ")";
    else
        return "(let long " + identifier + " " + expression->to_string() + ")";
}

NodeDebug::NodeDebug(Node *expr) {
    type = DBG;
    expression = expr;
}

std::string NodeDebug::to_string() {
    return "(dbg " + expression->to_string() + ")";
}

NodeRet::NodeRet(Node *expr) {
    type = RET;
    expression = expr;
}

std::string NodeRet::to_string() {
    return "(ret " + expression->to_string() + ")";
}

NodeIdent::NodeIdent(std::string ident, int dtype) {
    type = IDENT;
    identifier = ident;
    data_type = dtype;
}
std::string NodeIdent::to_string() {
    return identifier;
}