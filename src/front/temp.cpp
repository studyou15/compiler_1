#include <map>
#include <cassert>
#include <string>
#include <iostream>

#define TODO assert(0 && "TODO")
// #define DEBUG_DFA

// enumerate for Status
enum class State {
	Empty,              // space, \n, \r ...
	IntLiteral,         // int literal, like '1' '01900', '0xAB', '0b11001'
	op                  // operators and '(', ')'
};

std::string toString(State s) {
	switch (s) {
		case State::Empty:
			return "Empty";
		case State::IntLiteral:
			return "IntLiteral";
		case State::op:
			return "op";
		default:
			assert(0 && "invalid State");
	}
	return "";
}

// enumerate for Token type
enum class TokenType {
	INTLTR,        // int literal
	PLUS,        // +
	MINU,        // -
	MULT,        // *
	DIV,        // /
	LPARENT,        // (
	RPARENT,        // )
};

std::string toString(TokenType type) {
	switch (type) {
		case TokenType::INTLTR:
			return "INTLTR";
		case TokenType::PLUS:
			return "PLUS";
		case TokenType::MINU:
			return "MINU";
		case TokenType::MULT:
			return "MULT";
		case TokenType::DIV:
			return "DIV";
		case TokenType::LPARENT:
			return "LPARENT";
		case TokenType::RPARENT:
			return "RPARENT";
		default:
			assert(0 && "invalid token type");
			break;
	}
	return "";
}

// definition of Token
struct Token {
	TokenType type;
	std::string value;
};

// definition of DFA
struct DFA {
		/**
		 * @brief constructor, set the init state to State::Empty
		 */
		DFA();

		/**
		 * @brief destructor
		 */
		~DFA();

		// the meaning of copy and assignment for a DFA is not clear, so we do not allow them
		DFA(const DFA &) = delete;  // copy constructor
		DFA &operator=(const DFA &) = delete;   // assignment

		/**
		 * @brief take a char as input, change state to next state, and output a Token if necessary
		 * @param[in] input: the input character
		 * @param[out] buf: the output Token buffer
		 * @return  return true if a Token is produced, the buf is valid then
		 */
		bool next(char input, Token &buf);

		/**
		 * @brief reset the DFA state to begin
		 */
		void reset();

	private:
		State cur_state;    // record current state of the DFA
		std::string cur_str;    // record input characters
};


DFA::DFA(): cur_state(State::Empty), cur_str() {}

DFA::~DFA() {}

// helper function, you are not require to implement these, but they may be helpful
bool isoperator(char c) {
	TODO;



}

TokenType get_op_type(std::string s) {
	if(s=="+")return TokenType::PLUS;
    if(s=="-")return TokenType::MINU;
    if(s=="*")return TokenType::MULT;
    if(s=="/")return TokenType::DIV;
    if(s=="(")return TokenType::LPARENT;
    if(s==")")return TokenType::RPARENT;
    return TokenType::INTLTR;
}

bool DFA::next(char input, Token &buf) {
    if(isspace(input))
    {   
		if(cur_state == State::Empty)
			return false;
        buf.value = cur_str;
		buf.type = get_op_type(cur_str);
        reset();
        return true;
    }
	if(cur_state == State::Empty)
	{
		cur_str += input;
		cur_state = get_op_type(cur_str) == TokenType::INTLTR? State::IntLiteral:State::op;
		return false;
	}
	std::string ss = "";
	ss += input;
	TokenType temp = get_op_type(ss);
	if(temp != TokenType::INTLTR)
	{
		buf.value = cur_str;
		buf.type = get_op_type(cur_str);
		reset();
		cur_str = ss;
		cur_state = State::op;
		return true;
	}
	else if(cur_state == State::op)
	{
		buf.value = cur_str;
		buf.type = get_op_type(cur_str);
		reset();
		cur_str = ss;
		cur_state = State::IntLiteral;
		return true;
	}
	else
	{
		cur_str +=ss;
		return false;
	}
}

void DFA::reset() {
	cur_state = State::Empty;
	cur_str = "";
}

int main() {
	std::string stdin_str;
	std::getline(std::cin, stdin_str);
	stdin_str += "\n";
	DFA dfa;
	Token tk;
	for (size_t i = 0; i < stdin_str.size(); i++) {
		if (dfa.next(stdin_str[i], tk)) {
			std::cout << toString(tk.type) << " " << tk.value << std::endl;
		}
	}
	return 0;
}

