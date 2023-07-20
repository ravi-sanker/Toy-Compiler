#include "symbol.hh"

bool SymbolTable::contains(std::string key) {
    return table.find(key) != table.end();
}

void SymbolTable::insert(std::string key, std::string data_type) {
    table[key] = data_type;
}

std::string SymbolTable::get_value(std::string key){
    return table[key];
}