#include"front/lexical.h"

#include<map>
#include<cassert>
#include<string>

#include<sstream>
#define TODO assert(0 && "todo")

// #define DEBUG_DFA
// #define DEBUG_SCANNER

std::string frontend::toString(State s) {
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

std::set<std::string> frontend::keywords= {
    "const", "int", "float", "if", "else", "while", "continue", "break", "return", "void"
};

frontend::DFA::DFA(): cur_state(frontend::State::Empty), cur_str() {}

frontend::DFA::~DFA() {}

bool frontend::DFA::next(char input, Token& buf) {
#ifdef DEBUG_DFA
#include<iostream>
    std::cout << "in state [" << toString(cur_state) << "], input = \'" << input << "\', str = " << cur_str << "\t";
#endif

    
    return false;

#ifdef DEBUG_DFA
    std::cout << "next state is [" << toString(cur_state) << "], next str = " << cur_str << "\t, ret = " << ret << std::endl;
#endif
}

void frontend::DFA::reset() {
    cur_state = State::Empty;
    cur_str = "";
}

frontend::Scanner::Scanner(std::string filename): fin(filename) {
    if(!fin.is_open()) {
        assert(0 && "in Scanner constructor, input file cannot open");
    }
}

frontend::Scanner::~Scanner() {
    fin.close();
}

std::string frontend::Scanner::preprocess(std::ifstream& fin){
    std::stringstream ss;
    std::string line;
    bool inBlockComment = false;

    while (getline(fin, line)) {
        size_t pos;
        
        // 处理块注释 /* ... */
        if (!inBlockComment) {
            pos = line.find("/*");
            if (pos != std::string::npos) {
                inBlockComment = true;
                line = line.substr(0, pos);
            }
        } else {
            pos = line.find("*/");
            if (pos != std::string::npos) {
                inBlockComment = false;
                line = line.substr(pos + 2);
            } else {
                line.clear(); // 仍在块注释中，忽略整行
            }
        }

        // 如果不是在块注释中，处理行注释 //
        if (!inBlockComment) {
            pos = line.find("//");
            if (pos != std::string::npos) {
                line = line.substr(0, pos);
            }
        }

        // 添加非空行到结果
        if (!line.empty()) {
            ss << line << ' ';
        }
    }

    return ss.str();
}

std::vector<frontend::Token> frontend::Scanner::run() {
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
            std::cout << "token: " << toString(tk.type) << "\t" << tk.value << std::endl;
#endif
}