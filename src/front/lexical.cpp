#include"front/lexical.h"

#include<map>
#include<cassert>
#include<string>

#include<sstream>
#define TODO assert(0 && "todo")

// #define DEBUG_DFA
// #define DEBUG_SCANNER

std::string frontend::toString(State s) 
{
    switch (s) {
    case State::Empty: return "Empty";
    case State::Ident: return "Ident";
    case State::IntLiteral: return "IntLiteral";
    case State::FloatLiteral: return "FloatLiteral";
    case State::op: return "op";
    default:
        assert(0 && "invalid State");
    }
    return "";
}

std::set<std::string> frontend::keywords= 
{
    "const", "int", "float", "if", "else", "while", "continue", "break", "return", "void"
};

std::set<char> frontend::opwords1 = 
{
    ')', '+', '-', '*', '/', '%', 
    '<', '>', ':', '=', ';', ',', 
    '(', '[', ']', '{', '}', '!', '|', '&'
};

std::set<std::string> frontend::opwords2 =
{
    "<=", ">=", "==", "!=", "&&", "||"
};

frontend::DFA::DFA(): cur_state(frontend::State::Empty), cur_str() {}

frontend::DFA::~DFA() {}

bool frontend::DFA::next(char input, Token& buf) 
{
#ifdef DEBUG_DFA
#include<iostream>
    std::cout << "in state [" << toString(cur_state) << "], input = \'" << input << "\', str = " << cur_str << "\t";
#endif

    if(isspace(input))//空格情况
    {
        buf.value = cur_str;
        if(cur_state == frontend::State::Empty)
            return false;
        else if(cur_state == frontend::State::FloatLiteral)
            buf.type = frontend::TokenType::FLOATLTR;
        else if(cur_state == frontend::State::IntLiteral)
            buf.type = frontend::TokenType::INTLTR;
        else if(cur_state == frontend::State::Ident)
            buf.type = get_Ident_type(cur_str);
        else if(cur_state == frontend::State::op)
            buf.type = get_op_type(cur_str);
        reset();
        return true;
    }

    bool res = 0;

    switch (cur_state){
        case frontend::State::Empty:
            cur_str = input;
            if(input=='_' || isalpha(input))
                cur_state = frontend::State::Ident;
            else if(opwords1.find(input)!=opwords1.end())
                cur_state = frontend::State::op;
            else if(input=='.')
                cur_state = frontend::State::FloatLiteral;
            else if(isdigit(input))
                cur_state = frontend::State::IntLiteral;
            else
                assert(0 && "invalid Empty char");
            break;
        case frontend::State::FloatLiteral:
            if(isdigit(input))
                cur_str += input;
            else if(opwords1.find(input)!=opwords1.end())
            {
                buf.value = cur_str;
                buf.type = frontend::TokenType::FLOATLTR;
                cur_state = frontend::State::op;
                cur_str = input;
                res = 1;
            }
            else
                assert(0 && "invalid FloatLiteral char");
            break;
        case frontend::State::Ident:
            if(isdigit(input) || isalpha(input) || input=='_')
                cur_str += input;
            else if(opwords1.find(input)!=opwords1.end())
            {
                buf.value = cur_str;
                buf.type = get_Ident_type(cur_str);
                cur_state = frontend::State::op;
                cur_str = input;
                res = 1;
            }
            else
                assert(0 && "invalid Ident char");
            break;
        case frontend::State::IntLiteral:
            if(input == '.')
            {
                cur_state = frontend::State::FloatLiteral;
                cur_str += input;
            }
            else if(isalnum(input))
                cur_str += input;
            else if(opwords1.find(input)!=opwords1.end())
            {
                buf.value = cur_str;
                buf.type = frontend::TokenType::INTLTR;
                cur_state = frontend::State::op;
                cur_str = input;
                res = 1;
            }
            else
                assert(0 && "invalid Ident char");
            break;
        case frontend::State::op:
            if(isdigit(input))
            {
                buf.value = cur_str;
                buf.type = get_op_type(cur_str);
                cur_state = frontend::State::IntLiteral;
                cur_str = input;
                res = 1;
            }
            else if(isalpha(input) || input == '_')
            {
                buf.value = cur_str;
                buf.type = get_op_type(cur_str);
                cur_state = frontend::State::Ident;
                cur_str = input;
                res = 1;
            }
            else if(opwords1.find(input)!=opwords1.end())
            {
                if(opwords2.find(cur_str + input)!=opwords2.end())
                    cur_str += input;
                else
                {
                    buf.value = cur_str;
                    buf.type = get_op_type(cur_str);
                    cur_state = frontend::State::op;
                    cur_str = input;
                    res = 1;
                }
            }
            else
                assert(0 && "invalid op char");
            break;
    }
    return res;

#ifdef DEBUG_DFA
    std::cout << "next state is [" << toString(cur_state) << "], next str = " << cur_str << "\t, ret = " << ret << std::endl;
#endif
}

void frontend::DFA::reset() 
{
    cur_state = State::Empty;
    cur_str = "";
}

frontend::Scanner::Scanner(std::string filename): fin(filename) 
{
    if(!fin.is_open()) 
    {
        assert(0 && "in Scanner constructor, input file cannot open");
    }
}

frontend::Scanner::~Scanner() 
{
    fin.close();
}

std::string frontend::Scanner::preprocess(std::ifstream& fin)
{
    std::stringstream ss;
    std::string line;
    bool inBlockComment = false;

    while (getline(fin, line)) 
    {
        size_t pos;
        
        // 处理块注释 /* ... */
        if (!inBlockComment) {
            pos = line.find("/*");
            if (pos != std::string::npos) 
            {
                inBlockComment = true;
                line = line.substr(0, pos);
            }
        } else 
        {
            pos = line.find("*/");
            if (pos != std::string::npos) 
            {
                inBlockComment = false;
                line = line.substr(pos + 2);
            } else 
            {
                line.clear(); // 仍在块注释中，忽略整行
            }
        }

        // 如果不是在块注释中，处理行注释 //
        if (!inBlockComment) 
        {
            pos = line.find("//");
            if (pos != std::string::npos) 
            {
                line = line.substr(0, pos);
            }
        }

        // 添加非空行到结果
        if (!line.empty()) 
        {
            ss << line << ' ';
        }
    }

    return ss.str();
}

std::vector<frontend::Token> frontend::Scanner::run() 
{
    std::vector<frontend::Token>ans;
    Token tk;
    DFA dfa;
    std::string s = frontend::Scanner::preprocess(fin);
    for(auto c: s){
        if((dfa.next(c,tk)))
            ans.push_back(tk);
    }
    return ans;
#ifdef DEBUG_SCANNER
#include<iostream>
#include "lexical.h"
            std::cout << "token: " << toString(tk.type) << "\t" << tk.value << std::endl;
#endif
}

frontend::TokenType frontend::get_Ident_type(std::string s)
{
    if (s == "const")
        return frontend::TokenType::CONSTTK;
    else if (s == "int")
        return frontend::TokenType::INTTK;
    else if (s == "float")
        return frontend::TokenType::FLOATLTR;
    else if (s == "if")
        return frontend::TokenType::IFTK;
    else if (s == "else")
        return frontend::TokenType::ELSETK;
    else if (s == "while")
        return frontend::TokenType::WHILETK;
    else if (s == "continue")
        return frontend::TokenType::CONTINUETK;
    else if (s == "break")
        return frontend::TokenType::BREAKTK;
    else if (s == "return")
        return frontend::TokenType::RETURNTK;
    else if (s == "void")
        return frontend::TokenType::VOIDTK;

    return frontend::TokenType::IDENFR;
}

frontend::TokenType frontend::get_op_type(std::string s)
{
    if (s == "+")
        return frontend::TokenType::PLUS;
    else if (s == "-")
        return frontend::TokenType::MINU;
    else if (s == "*")
        return frontend::TokenType::MULT;
    else if (s == "/")
        return frontend::TokenType::DIV;
    else if (s == "%")
        return frontend::TokenType::MOD;
    else if (s == "<")
        return frontend::TokenType::LSS;
    else if (s == ">")
        return frontend::TokenType::GTR;
    else if (s == ":")
        return frontend::TokenType::COLON;
    else if (s == "=")
        return frontend::TokenType::ASSIGN;
    else if (s == ";")
        return frontend::TokenType::SEMICN;
    else if (s == ",")
        return frontend::TokenType::COMMA;
    else if (s == "(")
        return frontend::TokenType::LPARENT;
    else if (s == ")")
        return frontend::TokenType::RPARENT;
    else if (s == "[")
        return frontend::TokenType::LBRACK;
    else if (s == "]")
        return frontend::TokenType::RBRACK;
    else if (s == "{")
        return frontend::TokenType::LBRACE;
    else if (s == "}")
        return frontend::TokenType::RBRACE;
    else if (s == "!")
        return frontend::TokenType::NOT;
    else if (s == "<=")
        return frontend::TokenType::LEQ;
    else if (s == ">=")
        return frontend::TokenType::GEQ;
    else if (s == "==")
        return frontend::TokenType::EQL;
    else if (s == "!=")
        return frontend::TokenType::NEQ;
    else if (s == "&&")
        return frontend::TokenType::AND;
    else if (s == "||")
        return frontend::TokenType::OR;
    else
        assert(0 && "invalid operator!");
}
