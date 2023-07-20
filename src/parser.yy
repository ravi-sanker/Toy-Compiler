%define api.value.type { ParserValue }

%code requires {
#include <iostream>
#include <vector>
#include <string>
#include <map>

#include "parser_util.hh"
#include "symbol.hh"

}

%code {

#include <cstdlib>
extern int yylex();
extern int yyparse();

extern NodeStmts* final_values;

std::vector<SymbolTable> table_stack;
int scope_count=0;
int prev_scope_count=0;
int max_scope_count=0;
int in_func_def=0;
int cur_ret_type=-1;
std::unordered_map<std::string,int> fn_ret;
std::unordered_map<std::string,std::vector<int>> fn_arg;
std::vector<int> arg_list;
std::string cur_func;
// SymbolTable global;
// table_stack.push_back(global);

int yyerror(std::string msg);

}
 
%define parse.error verbose

%token TPLUS TDASH TSTAR TSLASH
%token <lexeme> TINT_LIT TIDENT  TTYPE
%token TLET TDBG TQUEST TCOLON TSCOL TLPAREN TRPAREN TEQUAL TLPAREN2 TRPAREN2 TIF TELSE TFUN TRET TCOM

%type <node> Expr Stmt
%type <stmts> Program StmtList
%type <args> ArgList ArgListActual

%left TCOLON
%left TQUEST
%left TPLUS TDASH
%left TSTAR TSLASH

%%

Program :                
        { final_values = nullptr; }
        | StmtList 
        { final_values = $1; }
	    ;

StmtList : Stmt              
         { $$ = new NodeStmts(); $$->push_back($1); }
	     | StmtList Stmt 
         { $$->push_back($2); }
	     ;

Stmt : TLET TIDENT TCOLON TTYPE TEQUAL Expr TSCOL
     {
        std::string name_temp=$2;
        name_temp+="_"+std::to_string(scope_count);
        if(table_stack[table_stack.size()-1].contains(name_temp)) {
            // tried to redeclare variable, so error
            yyerror("tried to redeclare variable.\n");
        } else {
            table_stack[table_stack.size()-1].insert(name_temp, $4);
            if($4 == "short" && $6->data_type > 1)
                yyerror("Type Mismatch");
            else if($4 == "int" && $6->data_type > 2)
                yyerror("Type Mismatch");
            else if($4 == "long" && $6->data_type > 3)
                yyerror("Type Mismatch");
            $$ = new NodeDecl(name_temp, $4, $6);
        }
     }
     |  TIDENT TEQUAL Expr TSCOL
     {
        std::string name_temp=$1;
        name_temp+="_"+std::to_string(scope_count);
        int flag;
        for(int i =table_stack.size()-1; i>=0;i--){
            std::vector<std::string> keys;
            for (auto& it: table_stack[i].table) {
                keys.push_back(it.first);
            }
            flag=0;
            for(int j= keys.size()-1;j>=0;j--){
                if(keys[j].substr(0,$1.size())==$1){
                    std::string temp = table_stack[i].get_value(keys[j]);
                    if(temp == "short" && $3->data_type > 1)
                        yyerror("Type Mismatch");
                    else if(temp == "int" && $3->data_type > 2)
                        yyerror("Type Mismatch");
                    else if(temp == "long" && $3->data_type > 3)
                        yyerror("Type Mismatch");
                    
                    $$ = new NodeAssign(keys[j], temp, $3);
                    flag=1;
                    break;
                }
            }
            if(flag) break; 
        }
        if(!flag)
            yyerror("using undeclared variable.\n");
        
     }
     | TDBG Expr TSCOL
     { 
        $$ = new NodeDebug($2);
     }
     | TRET Expr TSCOL
     {
        cur_ret_type=$2->data_type;
        $$ = new NodeRet($2);
     }
     | TIF Expr TLPAREN2 Program TRPAREN2 TELSE TLPAREN2 Program TRPAREN2
     {
        $$ = new NodeIf($2,$4,$8);
     }
     | TFUN TIDENT TLPAREN ArgList TRPAREN TCOLON TTYPE TLPAREN2 Program TRPAREN2
     {
        $$ = new NodeFun(new NodeIdent($2,1),$4,$9);
        if($7=="short"){
            fn_ret[$2]=1;
        }                
        if($7=="int"){
            fn_ret[$2]=2;
        }                
        if($7=="long"){
            fn_ret[$2]=3;
        }
        if(fn_ret[$2] < cur_ret_type){
            yyerror("Return type mismatch");
        }
        cur_ret_type=-1;
        std::vector<int> vectemp=arg_list;
        fn_arg[$2] = vectemp;
     }
     ;

ArgList :{$$ = new NodeArgs(); arg_list.clear();}
        | TIDENT TCOLON TTYPE
        {
            arg_list.clear();
            std::string name_temp=$1;
            name_temp+="_"+std::to_string(scope_count);
            table_stack[table_stack.size()-1].insert(name_temp,$3);
            if($3=="short"){
                $$ = new NodeArgs(); $$->push_back(new NodeIdent(name_temp,1));
                arg_list.push_back(1);
            }                
            if($3=="int"){
                $$ = new NodeArgs(); $$->push_back(new NodeIdent(name_temp,2));
                arg_list.push_back(2);
            }                
            if($3=="long"){
                $$ = new NodeArgs(); $$->push_back(new NodeIdent(name_temp,3));
                arg_list.push_back(3);
            }   
        }
        | ArgList TCOM TIDENT TCOLON TTYPE
        {
            std::string name_temp=$3;
            name_temp+="_"+std::to_string(scope_count);
            table_stack[table_stack.size()-1].insert(name_temp,$5);
            if($5=="short"){
                $$->push_back(new NodeIdent(name_temp,1));
                arg_list.push_back(1);
            }
            if($5=="int"){
                $$->push_back(new NodeIdent(name_temp,2));
                arg_list.push_back(2);
            }
            if($5=="long"){
                $$->push_back(new NodeIdent(name_temp,3));
                arg_list.push_back(3);
            }
        }

ArgListActual :{$$ = new NodeArgs(); arg_list.clear();}
        | Expr
        {
            $$ = new NodeArgs(); $$->push_back($1);
            arg_list.clear();
            arg_list.push_back($1->data_type);
        }
        | ArgListActual TCOM Expr
        {
            $$->push_back($3);
            arg_list.push_back($3->data_type);
        }

Expr : TINT_LIT               
     { $$ = new NodeInt(stoi($1)); 
     }
     | TIDENT
     { 
        std::string name_temp=$1;
        name_temp+="_"+std::to_string(scope_count);
        int flag;
        for(int i =table_stack.size()-1; i>=0;i--){
            std::vector<std::string> keys;
            for (auto& it: table_stack[i].table) {
                keys.push_back(it.first);
            }
            flag=0;
            for(int j= keys.size()-1;j>=0;j--){
                if(keys[j].substr(0,$1.size())==$1){
                    std::string x = table_stack[i].get_value(keys[j]);
                    if(x == "short")
                        $$ = new NodeIdent(keys[j], 1);
                    else if(x == "int")
                        $$ = new NodeIdent(keys[j], 2);
                    else 
                        $$ = new NodeIdent(keys[j], 3); 
                    flag=1;
                    break;
                }
            }
            if(flag) break; 
        }
        if(!flag)
            yyerror("using undeclared variable.\n");
     }
     | Expr TPLUS Expr
     { $$ = new NodeBinOp(NodeBinOp::PLUS, $1, $3); $$->data_type=std::max($1->data_type,$3->data_type);}
     | Expr TDASH Expr
     { $$ = new NodeBinOp(NodeBinOp::MINUS, $1, $3); $$->data_type=std::max($1->data_type,$3->data_type);}
     | Expr TSTAR Expr
     { $$ = new NodeBinOp(NodeBinOp::MULT, $1, $3); $$->data_type=std::max($1->data_type,$3->data_type);}
     | Expr TSLASH Expr
     { $$ = new NodeBinOp(NodeBinOp::DIV, $1, $3); $$->data_type=std::max($1->data_type,$3->data_type);}
     |Expr TQUEST Expr TCOLON Expr
     { $$ = new NodeTernOp(NodeTernOp::TERNOP, $1, $3, $5);}
     | TLPAREN Expr TRPAREN { $$ = $2; }
     | TIDENT TLPAREN ArgListActual TRPAREN
     {
        $$ = new NodeFunCall(new NodeIdent($1,1),$3);
        $$->data_type=fn_ret[$1];
        std::vector<int> vectemp=fn_arg[$1];
        for(unsigned long i=0;i<vectemp.size();i++){
            if(vectemp[i]<arg_list[i]){
                yyerror("Argument type mismatch");
            }
        }
     }
     ;

%%

int yyerror(std::string msg) {
    std::cerr << "Error! " << msg << std::endl;
    exit(1);
}