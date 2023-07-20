#ifndef SYMBOL_HH
#define SYMBOL_HH

#include <set>
#include <string>
#include <unordered_map>
#include "ast.hh"


// Basic symbol table, just keeping track of prior existence and nothing else
struct SymbolTable {
    std::unordered_map<std::string, std::string> table;

    bool contains(std::string key);
    void insert(std::string key, std::string data_type);
    std::string get_value(std::string key);
};

#endif