#include"front/syntax.h"

#include<iostream>
#include<cassert>

using frontend::Parser;

// #define DEBUG_PARSER
#define TODO assert(0 && "todo")
#define CUR_TOKEN_IS(tk_type) (token_stream[index].type == TokenType::tk_type)
#define PARSE_TOKEN(tk_type) root->children.push_back(parseTerm(root, TokenType::tk_type))
#define PARSE(name, type) auto name = new type(root); assert(parse##type(name)); root->children.push_back(name); 
#define WRONG_SOLVE(idx_var, idx_val) \
    { \
        idx_var = idx_val; \
        return false; \
    }

Parser::Parser(const std::vector<frontend::Token>& tokens): index(0), token_stream(tokens) {}

Parser::~Parser() {}

void Parser::parseTerm(AstNode* root)
{
    frontend::Term* term = new frontend::Term(token_stream[index]);
    root->children.push_back(term);
    index++;
}

frontend::CompUnit* Parser::get_abstract_syntax_tree()
{
    frontend::CompUnit*  root = new frontend::CompUnit;
    
    while(index < (int)token_stream.size())
    {
        int idx = index;
        frontend::FuncDef* a = new frontend::FuncDef;
        if(parseFuncDef(a))
        {
            root->children.push_back(a);
            continue;
        }
        index = idx;
        delete a;
        frontend::Decl* a = new frontend::Decl;
        if(parseDecl(a))
        {
            root->children.push_back(a);
            continue;
        }
        assert(0 && "invalid Comp_Unit");
    }
    return root;
}

bool Parser::parseFuncDef(AstNode* root)//函数声明
{
    uint32_t idx = index;
    frontend::FuncType* type = new frontend::FuncType;

    if(!parseFuncType(type))
        WRONG_SOLVE(index, idx); 
    root->children.push_back(type);

    if(token_stream[index].type == frontend::TokenType::IDENFR)
        parseTerm(root);
    else
        WRONG_SOLVE(index, idx); 

    if(token_stream[index].type == frontend::TokenType::LPARENT)
        parseTerm(root);
    else
        WRONG_SOLVE(index, idx); 

    if(token_stream[index].type != frontend::TokenType::RPARENT)
    {
        frontend::FuncFParams* funcfparams = new frontend::FuncFParams;
        if(!parseFuncFParams(funcfparams))
            WRONG_SOLVE(index, idx); 
        root->children.push_back(funcfparams);
    }

    if(token_stream[index].type == frontend::TokenType::RPARENT)
        parseTerm(root);
    else
        WRONG_SOLVE(index, idx); 
    frontend::Block* block = new frontend::Block;

    if(!parseBlock(block))
        WRONG_SOLVE(index, idx); 
    root->children.push_back(block);

    return true;
}

bool Parser::parseFuncFParams(AstNode* root)//参数调用
{
    return false;
}

bool Parser::parseFuncType(AstNode* root)
{
    std::string s = token_stream[index].value;
    if(s=="int" || s=="void" || s=="float")
    {
        parseTerm(root);
        return true;
    }
    return false;
}

bool Parser::parseBlock(AstNode* root)
{
    int idx = index;
    if(token_stream[index].type == frontend::TokenType::LBRACE)
        parseTerm(root);
    else
        WRONG_SOLVE(index, idx); 
    while(token_stream[index].type != frontend::TokenType::RBRACE)
    {
        frontend::BlockItem* blockitem = new frontend::BlockItem;
        if(!parseBlockItem(blockitem))
            WRONG_SOLVE(index, idx); 
        root->children.push_back(blockitem);
    }
    if(token_stream[index].type == frontend::TokenType::RBRACE)
        parseTerm(root);
    else
        WRONG_SOLVE(index, idx); 
}

bool Parser::parseBlockItem(AstNode* root)
{
    int idx = index;
    frontend::Decl* decl = new frontend::Decl;
    if(parseDecl(decl))
    {
        root->children.push_back(decl);
        return true;
    }
    index = idx;
    delete decl;
    frontend::Stmt* stmt = new frontend::Stmt;
    if(parseStmt(stmt))
    {
        root->children.push_back(stmt);
        return true;
    }
    assert(0 && "invalid BlockItem");
}

bool Parser::parseStmt(AstNode* root)
{
    int idx = index;
    if(token_stream[index].type == TokenType::RETURNTK)
    {
        parseTerm(root);
        if(token_stream[index].type == TokenType::SEMICN)
        {
            parseTerm(root);
            return true;
        }
        frontend::Exp* exp = new frontend::Exp;
        if(parseExp(exp))
            root->children.push_back(exp);
        else
            assert(0 && "invalid Exp");
        parseTerm(root);//处理分号保证为分号
        return true;
    }
}

bool Parser::parseExp(AstNode* root)
{
    frontend::AddExp* addexp = new frontend::AddExp;
    parseAddExp(addexp);
    root->children.push_back(addexp);
    return true;
}

bool Parser::parseAddExp(AstNode* root)
{
    frontend::MulExp* left = new frontend::MulExp;
    parseMulExp(left);
    
}

bool Parser::parseDecl(AstNode* root)
{
    return false;
}

void Parser::log(AstNode* node){
#ifdef DEBUG_PARSER
        std::cout << "in parse" << toString(node->type) << ", cur_token_type::" << toString(token_stream[index].type) << ", token_val::" << token_stream[index].value << '\n';
#endif
}
