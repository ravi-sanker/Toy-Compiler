#include <iostream>
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include <fstream>

#include "parser.hh"
#include "ast.hh"
#include "llvmcodegen.hh"

extern FILE *yyin;
extern int yylex();
extern char *yytext;
// extern void set_pass1();
// extern void set_pass2();

NodeStmts *final_values;

#define ARG_OPTION_L 0
#define ARG_OPTION_P 1
#define ARG_OPTION_S 2
#define ARG_OPTION_O 3
#define ARG_FAIL -1

int parse_arguments(int argc, char *argv[]) {
	if (argc == 3 || argc == 4) {
		if (strlen(argv[2]) == 2 && argv[2][0] == '-') {
			if (argc == 3) {
				switch (argv[2][1]) {
					case 'l':
					return ARG_OPTION_L;

					case 'p':
					return ARG_OPTION_P;

					case 's':
					return ARG_OPTION_S;
				}
			} else if (argv[2][1] == 'o') {
				return ARG_OPTION_O;
			}
		}
	} 
	
	std::cerr << "Usage:\nEach of the following options halts the compilation process at the corresponding stage and prints the intermediate output:\n\n";
	std::cerr << "\t`./bin/base <file_name> -l`, to tokenize the input and print the token stream to stdout\n";
	std::cerr << "\t`./bin/base <file_name> -p`, to parse the input and print the abstract syntax tree (AST) to stdout\n";
	std::cerr << "\t`./bin/base <file_name> -s`, to compile the file to LLVM assembly and print it to stdout\n";
	std::cerr << "\t`./bin/base <file_name> -o <output>`, to compile the file to LLVM bitcode and write to <output>\n";
	return ARG_FAIL;
}

// call this only when you know there are only literals involved
int evaluate(Node* root){
	if(root -> type == Node::INT_LIT){
		return ((NodeInt*)root) -> value;
	}

	int ans = 0;
	if(root -> type == Node::BIN_OP){
		int l = evaluate(((NodeBinOp*)root) -> left);
		int r = evaluate(((NodeBinOp*)root) -> right);
		switch(((NodeBinOp*)root) -> op) {
			case NodeBinOp::PLUS: ans = l + r; break;
			case NodeBinOp::MINUS: ans = l - r; break;
			case NodeBinOp::MULT: ans = l * r; break;
			case NodeBinOp::DIV: ans = l/r; break;
    	}
	}
	return ans;	
}


bool allLiterals(Node* par, Node* root, bool right = false){
	if(root -> type == Node::INT_LIT){
		return true;
	}
	if(root -> type == Node::BIN_OP){
		bool l = allLiterals(root, ((NodeBinOp*)root) -> left, false);
		bool r = allLiterals(root, ((NodeBinOp*)root) -> right, true);

		if(l && r){
			int evalValue = evaluate(root);
			//std::cout << evalValue << std::endl;
			Node* repl = new NodeInt(evalValue);

			if(par -> type == Node::BIN_OP){
				if(right)
					((NodeBinOp*)par)->right = repl;
				else
					((NodeBinOp*)par)->left = repl;
			}
			else if(par -> type == Node::ASSN){
				
				((NodeAssign*)par)->expression = repl;
			}
			else if(par -> type == Node::DECL)
				((NodeDecl*)par)->expression = repl;
			else if(par -> type == Node::IF_STMT)
				((NodeIf*)par)->left = repl;
		}		

		return l && r;
	}
	return false;	
}

void constant_folding(NodeStmts* cur_values){
	for(Node* root: cur_values->list){
		if(root->type == Node::DECL) {
			Node* cur = ((NodeDecl*)root)->expression;
			allLiterals(root, cur);
		}
		else if(root->type == Node::ASSN) {
			Node* cur = ((NodeAssign*)root)->expression;
			allLiterals(root, cur);
		}
		else if(root->type == Node::FUNC_DEFN){
			constant_folding(((NodeFun*)root)->stmts);
		}
		else if(root->type == Node::IF_STMT){
			
			if(((NodeIf*)root)->left->type == Node::BIN_OP){
				allLiterals(root, ((NodeIf*)root)->left);
			}
			constant_folding(((NodeStmts*)((NodeIf*)root) -> middle));
			constant_folding(((NodeStmts*)((NodeIf*)root) -> right));
		}
		
	}
}

void branch_elimination(NodeStmts* cur_values){
	int ind = 0;
	for(Node* root: cur_values->list){
		if(root->type == Node::IF_STMT) {
			
			if((((NodeIf*)root) -> left) -> type == Node::INT_LIT){
				
				Node* repl;
				if(((NodeInt*)(((NodeIf*)root) -> left)) -> value == 0)
					repl = ((NodeIf*)root) -> right;
					
				else
					repl = ((NodeIf*)root) -> middle;
				branch_elimination((NodeStmts*)repl);
				cur_values->list[ind] = repl;
			}
				
			
		}
		else if(root->type == Node::FUNC_DEFN){
			branch_elimination(((NodeFun*)root)->stmts);
		}
		ind++;
	}
}
int main(int argc, char *argv[]) {
	int arg_option = parse_arguments(argc, argv);
	if (arg_option == ARG_FAIL) {
		exit(1);
	}

	std::string file_name(argv[1]);
	FILE *source = fopen(argv[1], "r");

    if(!source) {
        std::cerr << "File does not exists.\n";
        exit(1);
    }

	yyin = source;
	// set_pass1();
	// yyparse();
	// fseek(yyin, 0, SEEK_SET);
	// set_pass2();
	if (arg_option == ARG_OPTION_L) {
		extern std::string token_to_string(int token, const char *lexeme);

		while (true) {
			int token = yylex();
			if (token == 0) {
				break;
			}

			std::cout << token_to_string(token, yytext) << "\n";
		}
		fclose(yyin);
		return 0;
	}

    final_values = nullptr;
	yyparse();

	fclose(yyin);
	if(final_values) {
		
		constant_folding(final_values);
		branch_elimination(final_values);

		if (arg_option == ARG_OPTION_P) {
			std::cout << final_values->to_string() << std::endl;
			return 0;
		}
		
        llvm::LLVMContext context;
		LLVMCompiler compiler(&context, "base");
		compiler.compile(final_values);
        if (arg_option == ARG_OPTION_S) {
			compiler.dump();
        } else {
            compiler.write(std::string(argv[3]));
		}
	} else {
	 	std::cerr << "empty program";
	}

    return 0;
}