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
        frontend::Decl* b = new frontend::Decl;
        if(parseDecl(a))
        {
            root->children.push_back(a);
            continue;
        }
        assert(0 && "invalid Comp_Unit");
    }
    return root;
}

bool Parser::parseFuncDef(AstNode* root) // SOLVE
{
    uint32_t idx = index;
    frontend::FuncType* type = new frontend::FuncType;

    if(!parseFuncType(type))
        WRONG_SOLVE(index, idx); 
    root->children.push_back(type);

    if(CUR_TOKEN_IS(IDENFR))
        parseTerm(root);
    else
        WRONG_SOLVE(index, idx); 

    if(CUR_TOKEN_IS(LPARENT))
        parseTerm(root);
    else
        WRONG_SOLVE(index, idx); 

    if(!CUR_TOKEN_IS(RPARENT))
    {
        frontend::FuncFParams* funcfparams = new frontend::FuncFParams;
        if(!parseFuncFParams(funcfparams))
            WRONG_SOLVE(index, idx); 
        root->children.push_back(funcfparams);
    }

    if(CUR_TOKEN_IS(RPARENT))
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
    if(CUR_TOKEN_IS(LBRACE))
        parseTerm(root);
    else
        WRONG_SOLVE(index, idx); 
    while(!CUR_TOKEN_IS(RBRACE))
    {
        frontend::BlockItem* blockitem = new frontend::BlockItem;
        if(!parseBlockItem(blockitem))
            WRONG_SOLVE(index, idx); 
        root->children.push_back(blockitem);
    }
    if(CUR_TOKEN_IS(RBRACE))
        parseTerm(root);
    else
        WRONG_SOLVE(index, idx); 
    return true;
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
    if(CUR_TOKEN_IS(RETURNTK))
    {
        parseTerm(root);
        if(CUR_TOKEN_IS(SEMICN))
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
    if(CUR_TOKEN_IS(BREAKTK) || CUR_TOKEN_IS(CONTINUETK))
    {
        parseTerm(root);
        parseTerm(root);
        return true;
    }
    if(CUR_TOKEN_IS(IDENFR))
    {
        frontend::LVal* lval = new frontend::LVal;
        if(parseLVal(lval))
            root->children.push_back(lval);
        else
            assert(0 && "invalid Stmt lval");
        parseTerm(root);
        frontend::Exp* exp = new frontend::Exp;
        if(parseExp(exp))
            root->children.push_back(exp);
        else
            assert(0 && "invalid Stmt exp");
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

bool Parser::parseAddExp(AstNode* root) // SOLVE
{
    frontend::MulExp* left = new frontend::MulExp;
    if (!parseMulExp(left))
        return false;
    root->children.push_back(left);

    while (index < token_stream.size() && 
          (CUR_TOKEN_IS(PLUS) || CUR_TOKEN_IS(MINU))) {
        parseTerm(root);  // Parse the operator

        frontend::MulExp* right = new frontend::MulExp;
        if (!parseMulExp(right))
            return false;
        root->children.push_back(right);
    }
    return true;
}

bool Parser::parseMulExp(AstNode* root) // SOLVE
{
    frontend::UnaryExp* left = new frontend::UnaryExp;
    if(!parseUnaryExp(left))
        return false;
    root->children.push_back(left);

    while(index < token_stream.size() && (CUR_TOKEN_IS(MULT)||CUR_TOKEN_IS(DIV)||CUR_TOKEN_IS(MOD)))
    {
        parseTerm(root);

        frontend::UnaryExp* right = new frontend::UnaryExp;
        if(!parseUnaryExp(right))
            return false;
        root->children.push_back(right);
    }
    return true;
}

bool Parser::parseUnaryExp(AstNode* root)// SOLVE
{
    int idx = index;
    frontend::PrimaryExp* a = new frontend::PrimaryExp;
    if(parsePrimaryExp(a))
    {
        root->children.push_back(a);
        return true;
    }
    delete a;
    index = idx;
    if(CUR_TOKEN_IS(IDENFR))//如果进入一定为函数调用
    {
        parseTerm(root);
        if(!CUR_TOKEN_IS(LPARENT))
            assert(0 && "invalid Ident '(' [FuncRParams]");
        parseTerm(root);
        if(!CUR_TOKEN_IS(RPARENT))
        {
            frontend::FuncRParams* b = new frontend::FuncRParams;
            if(parseFuncRParams(b))
            {
                root->children.push_back(b);
                parseTerm(root);//右括号
                return true;
            }
            assert(0 && "invalid [FuncRParams]");
        }
        parseTerm(root);//右括号
        return true;
    }
    //无需重复赋值，进入上面的判断一定不会出来
    frontend::UnaryOp* op = new frontend::UnaryOp;
    if(parseUnaryOp(op))
        root->children.push_back(op);
    else
        assert(0 && "invalid UnaryOp");
    frontend::UnaryExp* exp = new frontend::UnaryExp;
    if(parseUnaryExp(exp))
        root->children.push_back(exp);
    else
    assert(0 && "invalid UnaryExp,UnaryExp");
}

bool Parser::parseFuncRParams(AstNode* node)
{
    return false;
}

bool Parser::parseUnaryOp(AstNode* root) // SOLVE
{
    if(CUR_TOKEN_IS(PLUS) || CUR_TOKEN_IS(MINU) || CUR_TOKEN_IS(NOT))
    {
        parseTerm(root);
        return true;
    }
    return false;
}

bool Parser::parsePrimaryExp(AstNode* root)
{
    if(CUR_TOKEN_IS(LPARENT))
    {
        parseTerm(root);
        frontend::Exp* exp = new frontend::Exp;
        if(parseExp(exp))
            root->children.push_back(exp);
        else
            assert(0 && "invalid Ident '(' [FuncRParams]");
        parseTerm(root);//直接插入右括号未判断
        return true;
    }
    if(CUR_TOKEN_IS(IDENFR))
    {
        frontend::LVal* lval = new frontend::LVal;
        if(parseLVal(lval))
            root->children.push_back(lval);
        else
            assert(0 && "invalid lval");
        return true;
    }
    frontend::Number* n = new frontend::Number;
    if(parseNumber(n))
    {
        root->children.push_back(n);
        return true;
    }
    else
        assert(0 && "invalid Number");
}

bool Parser::parseNumber(AstNode* root)
{
    parseTerm(root);
    return true;
}

bool Parser::parseLVal(AstNode* root)
{
    parseTerm(root);
    return true;
}

bool Parser::parseDecl(AstNode* root)
{
    if(CUR_TOKEN_IS(CONSTTK))
    {
        frontend::ConstDecl* condel = new frontend::ConstDecl;
        if(parseConstDecl(condel))
            root->children.push_back(condel);
        else
            assert(0 && "invalid Decl parseDecl");
        return true;
    }

    frontend::VarDecl* vardecl = new frontend::VarDecl;
    if(parseVarDecl(vardecl))
        root->children.push_back(vardecl);
    else
        return false;

    return true;
}

bool Parser::parseVarDecl(AstNode* root)
{
    frontend::BType* btype = new frontend::BType;
    if(parseBType(btype))
        root->children.push_back(root);
    else
        return false;
    do{
        if(CUR_TOKEN_IS(COMMA))
            parseTerm(root);
        frontend::VarDef* vardef = new frontend::VarDef;
        if(parseVarDef(vardef))
            root->children.push_back(vardef);
        else
            assert(0 && "invalid VarDef");
    }while(CUR_TOKEN_IS(COMMA));
    parseTerm(root);
    return true;
}

bool Parser::parseConstDecl(AstNode* root)
{
    frontend::BType* btype = new frontend::BType;
    parseTerm(root);//const 只有这个位置使用const，错误直接assert
    if(parseBType(btype))
        root->children.push_back(btype);
    else
        assert(0 && "invalid CONSTTK Decl");
    return true;    
    do{
        if(CUR_TOKEN_IS(COMMA))
            parseTerm(root);
        frontend::ConstDef* condef = new frontend::ConstDef;

    }while(CUR_TOKEN_IS(COMMA));
    parseTerm(root);
    return true;
}

bool Parser::parseConstDef(AstNode* root) //solve
{
    if(!CUR_TOKEN_IS(IDENFR)) 
        assert(0 && "invalid ConstDef IDENFR");
    parseTerm(root);

    while(CUR_TOKEN_IS(LBRACK)) {
        parseTerm(root); // '['
        
        frontend::ConstExp* const_exp = new frontend::ConstExp;
        if(!parseConstExp(const_exp)) 
            assert(0 && "invalid ConstDef ConstExp");
        root->children.push_back(const_exp);

        parseTerm(root); // ']'
    }

    if(!CUR_TOKEN_IS(ASSIGN)) 
        assert(0 && "invalid ConstDef ASSIGN");
    parseTerm(root); // '='
    
    frontend::ConstInitVal* const_init_val = new frontend::ConstInitVal;
    if(!parseConstInitVal(const_init_val)) 
        assert(0 && "invalid ConstDef const_init_val");
    root->children.push_back(const_init_val);

    return true;
}

bool Parser::parseConstInitVal(AstNode* root)
{
    uint32_t idx = index;
    
    // Case 1: Simple ConstExp
    if(!CUR_TOKEN_IS(LBRACE)) {
        frontend::ConstExp* const_exp = new frontend::ConstExp;
        if(parseConstExp(const_exp)) {
            root->children.push_back(const_exp);
            return true;
        }
        assert(0 && "invalid ConstInitVal exp");
    }
    
    // Case 2: Initialization list
    parseTerm(root); // '{'
    
    // Optional ConstInitVal list
    if(!CUR_TOKEN_IS(RBRACE)) {
        do {
            frontend::ConstInitVal* const_init_val = new frontend::ConstInitVal;
            assert(0 && "invalid ConstInitVal ConstInitVal");
            root->children.push_back(const_init_val);
            
            if(CUR_TOKEN_IS(COMMA)) {
                parseTerm(root); // ','
            } else {
                break;
            }
        } while(true);
    }
    
    parseTerm(root); // '}'
    
    return true;
}

bool Parser::parseVarDef(AstNode* root) //solve
{
    
    if(!CUR_TOKEN_IS(IDENFR))
        assert(0 && "invalid VarDef IDENFR");
    parseTerm(root);

    while(CUR_TOKEN_IS(LBRACK)) {
        parseTerm(root); // '['
        
        frontend::ConstExp* const_exp = new frontend::ConstExp;
        if(!parseConstExp(const_exp)) 
            assert(0 && "invalid VarDef const_exp");
        root->children.push_back(const_exp);
        
        if(!CUR_TOKEN_IS(RBRACK)) 
            assert(0 && "invalid VarDef RBRACK");
        parseTerm(root); // ']'
    }

    if(CUR_TOKEN_IS(ASSIGN)) {
        parseTerm(root); // '='
        
        frontend::InitVal* init_val = new frontend::InitVal;
        if(!parseInitVal(init_val)) 
            assert(0 && "invalid VarDef ASSIGN");
        root->children.push_back(init_val);
    }

    return true;
}

bool Parser::parseConstExp(AstNode* root) //solve
{
    frontend::AddExp* addexp = new frontend::AddExp;
    if(!parseAddExp(addexp))
        assert(0 && "invalid ConstExp");
    return true;
}

bool Parser::parseInitVal(AstNode* root) // solve
{
    
    if(!CUR_TOKEN_IS(LBRACE)) {
        frontend::Exp* exp = new frontend::Exp;
        if(parseExp(exp)) {
            root->children.push_back(exp);
            return true;
        }
        delete exp;
            assert(0 && "invalid InitVal");
    }
    
    parseTerm(root); // '{'
    
    if(!CUR_TOKEN_IS(RBRACE)) {
        do {
            frontend::InitVal* init_val = new frontend::InitVal;
            if(!parseInitVal(init_val)) 
                assert(0 && "invalid InitVal InitVal");
            root->children.push_back(init_val);
            
            if(CUR_TOKEN_IS(COMMA)) {
                parseTerm(root); // ','
            } else {
                break;
            }
        } while(true);
    }
    
    parseTerm(root); // '}'
    
    return true;
}

bool Parser::parseBType(AstNode* root) //solve
{
    if(CUR_TOKEN_IS(FLOATTK) || CUR_TOKEN_IS(INTTK))
    {
        parseTerm(root);
        return true;
    }
    return false;
}

void Parser::log(AstNode* node){
#ifdef DEBUG_PARSER
        std::cout << "in parse" << toString(node->type) << ", cur_token_type::" << toString(token_stream[index].type) << ", token_val::" << token_stream[index].value << '\n';
#endif
}
