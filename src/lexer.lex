%option noyywrap
%x DEF
%x DEF2
%x UNDEF
%x IFDEF
%x IFDUMMY
%x ELSEIFDEF
%x SCOPE
%{
#include "parser.hh"
#include "symbol.hh"
#include <string>
#include <unordered_map>
#include <vector>
#include <stack>

extern int yyerror(std::string msg);
extern std::vector<SymbolTable> table_stack;
extern int scope_count;
extern int prev_scope_count;
extern int max_scope_count;
extern int in_func_def;

std::unordered_map <std::string,std::vector<std::string>> adj;
std::unordered_map <std::string,std::string> mp;
std::string key;
// int ifCount = 0;

std::stack<std::pair<bool, bool>> ifState;
// first is if any of the preceding if/elif statements in the current if block were executed
// second if true if an else has been encountered in the current if block
bool isCyclicUtil(std::string v, std::unordered_map<std::string, bool> &visited, std::unordered_map<std::string, bool> &recStack) {
    if(visited[v] == false) {
        visited[v] = true;
        recStack[v] = true;
        for(auto i = adj[v].begin(); i != adj[v].end(); ++i) {
            if (!visited[*i] && isCyclicUtil(*i, visited, recStack) )
                return true;
            else if (recStack[*i])
                return true;
        }
 
    }
    recStack[v] = false;
    return false;
}

bool isCyclic() {
    std::unordered_map<std::string,bool> visited;
    std::unordered_map<std::string,bool> recStack;

    for(auto i = adj.begin(); i != adj.end(); i++) {
        visited[i -> first] = false;
        recStack[i -> first] = false;
    }
    for(auto i = adj.begin(); i != adj.end(); i++)
        if (!visited[i->first] && isCyclicUtil(i -> first, visited, recStack))
            return true;
 
    return false;
}

void addLink(std::string from, std::string to) {
    adj[from].push_back(to);
    if(isCyclic()) {
        yyerror("Cycle");
    }
}

void reset(std::string s) {
    mp.erase(s); 
    adj[s].clear();
}

void insertDef(std::string s) {
    std::vector<std::string> v;
    adj[s]=v;
}


%}

%%
"#def " { BEGIN DEF; }
<DEF>[a-zA-Z]+ { key = yytext; mp[key]=""; insertDef(yytext); }
<DEF>[ ]+ { BEGIN DEF2; }
<DEF>"\n" { mp[key]="1"; BEGIN INITIAL; }
<DEF>"//".*    { /* skip */ }
<DEF>"/*"([^*]|\*+[^*/])*\*+"/" { /* skip */ }

<DEF2>[;0-9+-/*=() ]+ { mp[key].append(yytext);}
<DEF2>[A-Za-z]+ { 
                    if(adj.find(key) != adj.end()) {
                        addLink(key, yytext);
                    }  
                    mp[key].append(yytext);
                }
<DEF2>"\\\n" { }
<DEF2>"\n" { if(mp[key]==""){mp[key]="1";} BEGIN INITIAL; }
<DEF2>"//".*    { /* skip */ }
<DEF2>"/*"([^*]|\*+[^*/])*\*+"/" { /* skip */ }

"#undef " { BEGIN UNDEF; }
<UNDEF>[a-zA-Z]+ { reset(yytext);}
<UNDEF>[ \t\n]+ { BEGIN INITIAL; }

"#ifdef "   {
                
                //ifCount++;
                BEGIN IFDEF;
               
            }
<IFDEF>[a-zA-Z0-9]+ {
                        if(mp.find(yytext) == mp.end()){
                            ifState.push({false, false});
                            //std::cout <<"here3\n";
                            BEGIN IFDUMMY;
                        }
                        else {
                            //std::cout <<"here\n";
                            ifState.push({true, false});
                            BEGIN INITIAL;
                        }  
                    }

<ELSEIFDEF>[a-zA-Z0-9]+ {
                            if(mp.find(yytext) == mp.end()) BEGIN IFDUMMY;
                            else {
                                ifState.top().first = true;
                                BEGIN INITIAL;
                            }
                        }

"#endif"[ \t\n]*    {
                        if(!ifState.size())
                            yyerror("Illegal endif.\n");
                        else{
                            //std::cout << "here";
                            ifState.pop();
                        }
                    }

"#else"[ \t\n]+ {
                    if(!ifState.size() || ifState.top().second)
                        yyerror("Illegal else.\n");
                    else{
                                    ifState.top().second = true;
                                    if(ifState.top().first) BEGIN IFDUMMY;
                                }
                }

"#elseif"[ \t\n]+   {
                        if(!ifState.size() || ifState.top().second)
                            yyerror("Illegal elseif.\n");
                        else if (ifState.top().first) 
                            BEGIN IFDUMMY;
                        else 
                            BEGIN ELSEIFDEF;
                        
                    }
<IFDUMMY>"#endif"[ \t\n]*   {
                                if(!ifState.size())
                                    yyerror("Illegal endif.\n");
                                else {
                                    ifState.pop();
                                    if(!ifState.size()) 
                                        BEGIN INITIAL;
                                }
                            }
<IFDUMMY>"#elseif"[ \t\n]+   {
                        if(!ifState.size() || ifState.top().second)
                            yyerror("Illegal elseif.\n");
                        else if (ifState.top().first) 
                            BEGIN IFDUMMY;
                        else 
                            BEGIN ELSEIFDEF;
                        
                    }
<IFDUMMY>"#else"[ \t\n]+    {
                                if(!ifState.size() || ifState.top().second)
                                    yyerror("Illegal else.\n");
                                else{
                                    ifState.top().second = true;
                                    if(!ifState.top().first) BEGIN INITIAL;
                                }
                                    
                            }
<IFDUMMY>\n   { /* skip */ }
<IFDUMMY>.   { /* skip */ }

<IFDEF><<EOF>> { yyerror("Enclosing endif missing.\n"); }
<IFDUMMY><<EOF>> { yyerror("Enclosing endif missing.\n"); }
<<EOF>> {
            //std::cout << ifState.size() << std::endl;
            if(ifState.size() != 0)
                yyerror("Enclosing endif missing.\n");
            else 
                return 0;
        }

<SCOPE>. {if(!in_func_def){SymbolTable x; prev_scope_count = scope_count; scope_count+=max_scope_count+1; max_scope_count++; table_stack.push_back(x);} in_func_def=0; unput(yytext[0]); BEGIN INITIAL;}

"//".*    { /* skip */ }
"/*"([^*]|\*+[^*/])*\*+"/" { /* skip */ }
"if"      {return TIF; }
"else"      {return TELSE; }
"fun"      { in_func_def=1; SymbolTable x; prev_scope_count = scope_count; scope_count+=max_scope_count+1; max_scope_count++; table_stack.push_back(x); return TFUN; }
"ret"       {return TRET;}
","      {return TCOM; }
"+"       { return TPLUS; }
"-"       { return TDASH; }
"*"       { return TSTAR; }
"/"       { return TSLASH; }
";"       { return TSCOL; }
"("       { return TLPAREN; }
")"       { return TRPAREN; }
"{"       { BEGIN SCOPE; return TLPAREN2; }
"}"       { table_stack.pop_back(); scope_count = prev_scope_count; return TRPAREN2; }
"="       { return TEQUAL; }
":"       { return TCOLON; }
"?"       { return TQUEST; }
"dbg"     { return TDBG; }
"let"     { return TLET; }
"short"|"int"|"long" { yylval.lexeme = std::string(yytext); return TTYPE; }
[0-9]+    { yylval.lexeme = std::string(yytext); return TINT_LIT; }
[a-zA-Z_]+ { 
            if(mp.find(yytext) != mp.end()) {
                std::string val = mp[yytext];
                int n = val.size();
                for(int i = n - 1; i >= 0; i--)
                    unput(val[i]);
            } 
            else {
                yylval.lexeme = std::string(yytext); 
                return TIDENT;
            }
          }
[ \t\n]   { /* skip */ if(table_stack.size()==0){SymbolTable x;table_stack.push_back(x);}}
.         { yyerror(yytext); }

%%

std::string token_to_string(int token, const char *lexeme) {
    std::string s;
    switch (token) {
        case TQUEST: s = "TQUEST"; break;
        case TCOLON: s = "TCOLON"; break;
        case TPLUS: s = "TPLUS"; break;
        case TDASH: s = "TDASH"; break;
        case TSTAR: s = "TSTAR"; break;
        case TSLASH: s = "TSLASH"; break;
        case TSCOL: s = "TSCOL"; break;
        case TLPAREN: s = "TLPAREN"; break;
        case TRPAREN: s = "TRPAREN"; break;
        case TLPAREN2: s = "TLPAREN2"; break;
        case TRPAREN2: s = "TRPAREN2"; break;
        case TEQUAL: s = "TEQUAL"; break;
        case TIF: s = "TIF"; break;
        case TELSE: s = "TELSE"; break;
        case TFUN: s = "TFUN"; break;
        case TCOM: s = "TCOM"; break;
        case TRET: s = "TRET"; break;
        case TTYPE: s = "TTYPE"; break;
        
        case TDBG: s = "TDBG"; break;
        case TLET: s = "TLET"; break;
        
        case TINT_LIT: s = "TINT_LIT"; s.append("  ").append(lexeme); break;
        case TIDENT: s = "TIDENT"; s.append("  ").append(lexeme); break;
    }

    return s;
}
