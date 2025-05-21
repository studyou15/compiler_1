#include "front/semantic.h"

#include <cassert>
#include <sstream>
#include <iomanip>
using ir::Function;
using ir::Instruction;
using ir::Operand;
using ir::Operator;

#define TODO assert(0 && "TODO");

#define SIZE int(root->children.size())
#define GET_CHILD_PTR(node, type, index)                     \
    auto node = dynamic_cast<type *>(root->children[index]); \
    assert(node);
#define ANALYSIS(node, type, index)                          \
    auto node = dynamic_cast<type *>(root->children[index]); \
    assert(node);                                            \
    analysis##type(node, buffer);
#define COPY_EXP_NODE(from, to)              \
    to->is_computable = from->is_computable; \
    to->v = from->v;                         \
    to->t = from->t;
#define STR(x) std::to_string(x)

std::string namespa = "global";

map<std::string, ir::Function *> *frontend::get_lib_funcs()
{
    static map<std::string, ir::Function *> lib_funcs = {
        {"getint", new Function("getint", Type::Int)},
        {"getch", new Function("getch", Type::Int)},
        {"getfloat", new Function("getfloat", Type::Float)},
        {"getarray", new Function("getarray", {Operand("arr", Type::IntPtr)}, Type::Int)},
        {"getfarray", new Function("getfarray", {Operand("arr", Type::FloatPtr)}, Type::Int)},
        {"putint", new Function("putint", {Operand("i", Type::Int)}, Type::null)},
        {"putch", new Function("putch", {Operand("i", Type::Int)}, Type::null)},
        {"putfloat", new Function("putfloat", {Operand("f", Type::Float)}, Type::null)},
        {"putarray", new Function("putarray", {Operand("n", Type::Int), Operand("arr", Type::IntPtr)}, Type::null)},
        {"putfarray", new Function("putfarray", {Operand("n", Type::Int), Operand("arr", Type::FloatPtr)}, Type::null)},
    };
    return &lib_funcs;
}

vector<int> while_s;
vector<int> while_e;

string const_n = "X";
bool SZ = 0;
Type ExpT = Type::IntLiteral;

void frontend::SymbolTable::add_scope()
{
    int cnt = scope_stack.size();
    scope_stack.push_back({cnt, namespa + "_S" + STR(cnt)});
}
void frontend::SymbolTable::exit_scope()
{
    scope_stack.pop_back();
}

string frontend::SymbolTable::get_scoped_name(string id) const
/*输入一个变量名, 返回其在当前作用域下重命名后的名字 (相当于加后缀)*/
{
    if (scope_stack.empty() || scope_stack.back().cnt == 0)
    {
        return id; // Global scope or first block - no suffix
    }
    return id + "_" + scope_stack.back().name; // Add scope suffix
}

Operand frontend::SymbolTable::get_operand(string id) const
/*输入一个变量名, 在符号表中寻找最近的同名变量, 返回对应的 Operand(注意，此 Operand 的 name 是重命名后的)*/
{
    STE ste = get_ste(id);
    return ste.operand;
}

frontend::STE frontend::SymbolTable::get_ste(string id) const
/*输入一个变量名, 在符号表中寻找最近的同名变量, 返回 STE*/
{
    // Search from innermost to outermost scope
    for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); ++it)
    {

        if (it->table.find(id) != it->table.end())
        {
            return it->table.at(id);
        }
    }
    throw std::runtime_error("Symbol not found: " + id);
}

frontend::Analyzer::Analyzer() : tmp_cnt(0), symbol_table() {}

void frontend::Analyzer::analysisCompUnit(CompUnit *root, ir::Program &program)
{
    if (Decl *node = dynamic_cast<Decl *>(root->children[0]))
    { // Decl
        analysisDecl(node, program.functions[0].InstVec);
    }
    else if (FuncDef *node = dynamic_cast<FuncDef *>(root->children[0]))
    {
        Function *new_func = new Function();
        tmp_cnt = 0;
        analysisFuncDef(node, new_func);
        program.addFunction(*new_func);
    }
    else
        assert(0 && "Unknown node type");

    if (root->children.size() == 2)
    {
        GET_CHILD_PTR(child_comp, CompUnit, 1);
        analysisCompUnit(child_comp, program);
    }
}

void frontend::Analyzer::analysisFuncDef(FuncDef *root, ir::Function *&func)
{
    GET_CHILD_PTR(ftype, FuncType, 0);
    auto type = analysisFuncType(ftype);
    GET_CHILD_PTR(fname, Term, 1);
    auto name = fname->token.value;
    namespa = name;
    symbol_table.add_scope();
    std::vector<Operand> params;
    if (SIZE > 5) // 添加参数
    {
        GET_CHILD_PTR(p00, FuncFParams, 3);
        analysisFuncFParams(p00, func, params);
    }
    // 确保main函数被正确创建并添加到program
    func = new Function(name, params, type); // 使用正确的构造函数
    symbol_table.functions[name] = func;     // 添加到函数表

    GET_CHILD_PTR(block, Block, SIZE - 1);
    analysisBlock(block, func->InstVec, 0);

    symbol_table.exit_scope();
    if (func->InstVec.back()->op != Operator::_return)
    {
        // 只在函数确实没有return语句时添加默认返回
        if (type == ir::Type::null)
        {
            func->addInst(new Instruction({}, {}, {}, Operator::_return));
        }
        // 仅当函数名为main且没有return时添加return 0
        else if (name == "main")
        {
            func->addInst(new Instruction({"0", ir::Type::IntLiteral}, {}, {}, {Operator::_return}));
        }
    }
    namespa = "global";
}

void frontend::Analyzer::analysisFuncFParams(FuncFParams *root, Function *&func, std::vector<Operand> &parameter)
{
    for (int i = 0; i < SIZE; i += 2)
    {
        GET_CHILD_PTR(p0, FuncFParam, i);
        analysisFuncFParam(p0, func, parameter);
    }
}

void frontend::Analyzer::analysisFuncFParam(FuncFParam *root, ir::Function *&func, std::vector<Operand> &parameter)
{
    GET_CHILD_PTR(p0, BType, 0);
    analysisBType(p0);
    GET_CHILD_PTR(p1, Term, 1);

    if (SIZE != 2)
        p0->t = (p0->t == Type::Int) ? Type::IntPtr : Type::FloatPtr;

    symbol_table.scope_stack.back().table[p1->token.value];

    for (int i = 5; i < SIZE; i = i + 3) //???
    {
        GET_CHILD_PTR(exp, Exp, i);
        analysisExp(exp, func->InstVec);
    }
    parameter.push_back(Operand(p1->token.value, p0->t));
    symbol_table.scope_stack.back().table[p1->token.value] = {Operand(p1->token.value, p0->t)};
    return;
}

Type frontend::Analyzer::analysisFuncType(FuncType *root) // solve
{
    GET_CHILD_PTR(type, Term, 0);
    if (type->token.type == TokenType::INTTK)
        return Type::Int;
    if (type->token.type == TokenType::FLOATTK)
        return Type::Float;
    return Type::null;
}

void frontend::Analyzer::analysisBlock(Block *root, vector<ir::Instruction *> &buffer, bool p)
{
    // tmp_cnt = 0;
    if (p) // 不是从函数进入时，而是从while，if等进入block
    {
        symbol_table.add_scope();
        for (int i = 1; i < SIZE - 1; i++)
        {
            GET_CHILD_PTR(item, BlockItem, i);
            analysisBlockItem(item, buffer);
        }
        symbol_table.exit_scope();
    }
    else
    {
        for (int i = 1; i < SIZE - 1; i++)
        {
            GET_CHILD_PTR(item, BlockItem, i);
            analysisBlockItem(item, buffer);
        }
    }
}

void frontend::Analyzer::analysisBlockItem(BlockItem *root, vector<ir::Instruction *> &buffer) // solve
{
    if (Decl *node = dynamic_cast<Decl *>(root->children[0]))
        analysisDecl(node, buffer);
    else if (Stmt *node = dynamic_cast<Stmt *>(root->children[0]))
        analysisStmt(node, buffer);
    else
        assert(0 && "Unknown BlockItem type");
}

void frontend::Analyzer::analysisStmt(Stmt *root, vector<ir::Instruction *> &buffer)
{
    if (Block *node = dynamic_cast<Block *>(root->children[0]))
    {
        analysisBlock(node, buffer, true);
    }
    else if (root->children[0]->type == NodeType::TERMINAL) // 首字符为终结符
    {
        Term *term = dynamic_cast<Term *>(root->children[0]);
        switch (term->token.type)
        {
        case TokenType::RETURNTK:
        {
            if (SIZE == 2)
            {
                buffer.push_back(new Instruction({}, {}, {}, {Operator::_return}));
                return;
            }
            GET_CHILD_PTR(exp, Exp, 1);
            analysisExp(exp, buffer);
            //* 由于返回值的类型可能不匹配，所以还需要增加一个指向当前函数的全局指针来读取当前的返回值类型
            if (symbol_table.functions[namespa]->returnType == Type::Int)
            {
                if (exp->t == Type::Int || exp->t == Type::IntLiteral)
                {
                    buffer.push_back(new Instruction({exp->v, exp->t}, {}, {}, {Operator::_return}));
                }
                else if (exp->t == Type::FloatLiteral)
                {
                    exp->v = STR(int(std::stof(exp->v)));
                    exp->t = Type::IntLiteral;
                    buffer.push_back(new Instruction({exp->v, exp->t}, {}, {}, {Operator::_return}));
                }
                else
                {
                    string name = "f2i_t" + std::to_string(tmp_cnt); // 几个名称从创建开始就是一个类型不会变
                    buffer.push_back(new Instruction({exp->v, exp->t}, {}, {name, Type::Int}, {Operator::cvt_f2i}));
                    buffer.push_back(new Instruction({name, Type::Int}, {}, {}, {Operator::_return}));
                }
            }
            else if (symbol_table.functions[namespa]->returnType == Type::Float)
            {
                if (exp->t == Type::Float || exp->t == Type::FloatLiteral)
                {
                    buffer.push_back(new Instruction({exp->v, exp->t}, {}, {}, {Operator::_return}));
                }
                else
                {
                    assert(0 && "to be continue");
                }
            }
            else
            {
                assert(0 && "to be continue");
            }
            break;
        }
        case TokenType::IFTK:
        {
            GET_CHILD_PTR(cond, Cond, 2);
            analysisCond(cond, buffer);
            buffer.push_back(new Instruction({cond->v, cond->t},
                                             {},
                                             {"2", ir::Type::IntLiteral},
                                             Operator::_goto));
            int s = buffer.size();
            Instruction *branch = new Instruction({}, {}, {"0", ir::Type::IntLiteral}, Operator::_goto);
            Instruction *branch1 = new Instruction({}, {}, {"0", ir::Type::IntLiteral}, Operator::_goto);
            buffer.push_back(branch);

            GET_CHILD_PTR(p0, Stmt, 4);
            analysisStmt(p0, buffer);
            int e = buffer.size();
            buffer.push_back(branch1);
            branch->des.name = std::to_string(buffer.size() - s);
            if (SIZE == 7)
            {
                GET_CHILD_PTR(p1, Stmt, 6);
                analysisStmt(p1, buffer);
            }
            branch1->des.name = std::to_string(buffer.size() - e);
            break;
        }
        case TokenType::WHILETK:
        {
            while_s.push_back((int)buffer.size());
            GET_CHILD_PTR(cond, Cond, 2);
            analysisCond(cond, buffer);
            buffer.push_back(new Instruction({cond->v, cond->t},
                                             {},
                                             {"2", ir::Type::IntLiteral},
                                             Operator::_goto));
            while_e.push_back((int)buffer.size());
            Instruction *branch = new Instruction({}, {}, {"0", ir::Type::IntLiteral}, Operator::_goto);
            buffer.push_back(branch);
            GET_CHILD_PTR(stmt, Stmt, 4);
            analysisStmt(stmt, buffer);
            buffer.push_back(new Instruction(
                {},
                {},
                {"-" + std::to_string(buffer.size() - while_s.back()), Type::IntLiteral},
                Operator::_goto));
            branch->des.name = std::to_string((int)buffer.size() - while_e.back());
            while_e.pop_back();
            while_s.pop_back();
            break;
        }
        case TokenType::BREAKTK:
        {
            buffer.push_back(new ir::Instruction( //
                ir::Operand(),                    //
                ir::Operand(),                    //
                ir::Operand("-" + std::to_string(buffer.size() - while_e.back()),
                            Type::IntLiteral), //
                ir::Operator::_goto));
            return;
        }
        case TokenType::CONTINUETK:
        {
            buffer.push_back(new ir::Instruction( //
                ir::Operand(),                    //
                ir::Operand(),                    //
                ir::Operand("-" + std::to_string(buffer.size() - while_s.back()),
                            Type::IntLiteral), //
                ir::Operator::_goto));
            return;
        }
        default:
            break;
        }
    }
    else if (Exp *node = dynamic_cast<Exp *>(root->children[0]))
    {
        analysisExp(node, buffer);
    }
    else
    {
        GET_CHILD_PTR(lval, LVal, 0);
        GET_CHILD_PTR(exp, Exp, 2);

        analysisLVal(lval, buffer, true);
        if (lval->t == Type::Float || lval->t == Type::FloatLiteral)
            ExpT = Type::FloatLiteral;
        analysisExp(exp, buffer);
        ExpT = Type::IntLiteral;
        Operand tx("t" + std::to_string(tmp_cnt++), lval->t);
        if (lval->children.size() > 1) // 处理数组赋值
        {
            if (lval->t == Type::IntPtr && (exp->t == Type::Float || exp->t == Type::FloatLiteral))
            {
                buffer.push_back(new Instruction({exp->v, exp->t}, {}, tx, Operator::cvt_f2i));
                buffer.push_back(new Instruction({lval->v, lval->t}, {lval->i, Type::Int}, tx, Operator::store));
            }
            else if (lval->t == Type::FloatPtr && (exp->t == Type::Int || exp->t == Type::IntLiteral))
            {
                buffer.push_back(new Instruction({exp->v, exp->t}, {}, tx, Operator::cvt_i2f));
                buffer.push_back(new Instruction({lval->v, lval->t}, {lval->i, Type::Int}, tx, Operator::store));
            }
            else
                buffer.push_back(new Instruction({lval->v, lval->t}, {lval->i, Type::Int}, {exp->v, exp->t}, Operator::store));
        }
        else
        {
            if (lval->t == Type::Int && (exp->t == Type::Float || exp->t == Type::FloatLiteral))
            {
                buffer.push_back(new Instruction({exp->v, exp->t}, {}, tx, Operator::cvt_f2i));
                buffer.push_back(new Instruction(tx, {}, {lval->v, lval->t}, Operator::mov));
            }
            else if (lval->t == Type::Float && (exp->t == Type::Int || exp->t == Type::IntLiteral))
            {
                buffer.push_back(new Instruction({exp->v, exp->t}, {}, tx, Operator::cvt_i2f));
                buffer.push_back(new Instruction(tx, {}, {lval->v, lval->t}, Operator::mov));
            }
            else
                buffer.push_back(new Instruction({exp->v, exp->t}, {}, {lval->v, lval->t}, Operator::mov));
        }

        // tmp_cnt--;
    }
}

void frontend::Analyzer::analysisCond(Cond *root, vector<ir::Instruction *> &buffer)
{
    GET_CHILD_PTR(p0, LOrExp, 0);
    analysisLOrExp(p0, buffer);
    root->v = p0->v;
    root->t = p0->t;
}

void frontend::Analyzer::analysisLOrExp(LOrExp *root, vector<ir::Instruction *> &buffer)
{
    GET_CHILD_PTR(p0, LAndExp, 0);
    analysisLAndExp(p0, buffer);
    root->v = p0->v;
    root->t = p0->t;
    if (SIZE == 1)
        return;
    if (root->v[0] != 't')
    {
        string name = "t" + std::to_string(tmp_cnt++);
        buffer.push_back(new Instruction({root->v, root->t}, {}, {name, Type::Int}, Operator::mov));
        root->v = name; // 不管是什么类型的数，都将其放入一个寄存器中，防止了如果是变量，导致变量名出问题
        root->t = Type::Int;
        tmp_cnt++;
    }

    Instruction *branch = new Instruction({root->v, root->t}, {}, {"0", ir::Type::IntLiteral}, Operator::_goto);
    buffer.push_back(branch);
    int rback = buffer.size();
    GET_CHILD_PTR(p1, LOrExp, 2);
    analysisLOrExp(p1, buffer);

    if (p1->v[0] >= '0' && p1->v[0] <= '9')
    {
        if (p1->v.find(".") != -1)
        {
            if (stof(p1->v) > 0)
            {
                p1->v = "1";
                p1->t = Type::IntLiteral;
            }
            else
            {
                p1->v = "0";
                p1->t = Type::IntLiteral;
            }
        }
    }
    buffer.push_back(new Instruction({root->v, root->t}, {p1->v, p1->t}, {root->v, root->t}, Operator::_or));
    branch->des.name = std::to_string(buffer.size() - rback + 1);
    // for (int i = 2; i < SIZE; i += 2)
    // {
    //     GET_CHILD_PTR(p1, LOrExp, i);
    //     analysisLOrExp(p1, buffer);
    //     if (p1->v[0] >= '0' && p1->v[0] <= '9')
    //     {
    //         p1->t = Type::IntLiteral;
    //     }
    //     buffer.push_back(new Instruction({p1->v, p1->t}, {root->v, root->t}, {root->v, root->t}, Operator::_or));
    // }
    // tmp_cnt--;
}

void frontend::Analyzer::analysisLAndExp(LAndExp *root, vector<ir::Instruction *> &buffer)
{
    GET_CHILD_PTR(p0, EqExp, 0);
    analysisEqExp(p0, buffer);
    root->v = p0->v;
    root->t = p0->t;
    if (SIZE == 1)
        return;
    if (root->v[0] != 't')
    {
        string name = "t" + std::to_string(tmp_cnt++);
        if (p0->t == Type::Float || p0->t == Type::FloatLiteral)
            buffer.push_back(new Instruction({root->v, root->t}, {}, {name, Type::Int}, Operator::cvt_f2i));
        else
            buffer.push_back(new Instruction({root->v, root->t}, {}, {name, Type::Int}, Operator::mov));
        root->v = name; // 不管是什么类型的数，都将其放入一个寄存器中，防止了如果是变量，导致变量名出问题
        root->t = Type::Int;
        tmp_cnt++;
    }

    buffer.push_back(new Instruction({root->v, root->t}, {}, {"2", ir::Type::IntLiteral}, Operator::_goto));
    Instruction *branch = new Instruction({}, {}, {"0", ir::Type::IntLiteral}, Operator::_goto);
    buffer.push_back(branch);
    int rback = buffer.size();
    GET_CHILD_PTR(p1, LAndExp, 2);
    analysisLAndExp(p1, buffer);

    if (p1->v[0] >= '0' && p1->v[0] <= '9')
    {
        if (p1->v.find(".") != -1)
        {
            if (stof(p1->v) > 0)
            {
                p1->v = "1";
                p1->t = Type::IntLiteral;
            }
            else
            {
                p1->v = "0";
                p1->t = Type::IntLiteral;
            }
        }
        else
            p1->t = Type::IntLiteral;
    }
    buffer.push_back(new Instruction({root->v, root->t}, {p1->v, p1->t}, {root->v, root->t}, Operator::_and));
    branch->des.name = std::to_string(buffer.size() - rback + 1);
    // for (int i = 2; i < SIZE; i += 2)
    // {
    //     GET_CHILD_PTR(p1, LAndExp, i);
    //     analysisLAndExp(p1, buffer);
    //     if (p1->v[0] >= '0' && p1->v[0] <= '9')
    //     {
    //         p1->t = Type::IntLiteral;
    //     }
    //     buffer.push_back(new Instruction({p1->v, p1->t}, {root->v, root->t}, {root->v, root->t}, Operator::_and));
    // }
    // tmp_cnt--;
}

void frontend::Analyzer::analysisEqExp(EqExp *root, vector<ir::Instruction *> &buffer) // 没有考虑float情况
{
    GET_CHILD_PTR(p0, RelExp, 0);
    analysisRelExp(p0, buffer);
    root->v = p0->v;
    root->t = p0->t;
    if (SIZE == 1)
        return;
    if (root->v[0] != 't')
    {
        string name = "t" + std::to_string(tmp_cnt);
        buffer.push_back(new Instruction({root->v, root->t}, {}, {name, Type::Int}, Operator::mov));
        root->v = name; // 不管是什么类型的数，都将其放入一个寄存器中，防止了如果是变量，导致变量名出问题
        root->t = Type::Int;
        tmp_cnt++;
    }

    for (int i = 2; i < SIZE; i += 2)
    {
        GET_CHILD_PTR(p1, RelExp, i);
        analysisRelExp(p1, buffer);
        GET_CHILD_PTR(pt, Term, i - 1);
        if (p1->v[0] >= '0' && p1->v[0] <= '9')
        {
            if(p1->v.size() >=2 && p1->v[1] == '.')
                p1->t = Type::FloatLiteral;
            else
                p1->t = Type::IntLiteral;
        }
        if (pt->token.type == TokenType::EQL)
            buffer.push_back(new Instruction({p1->v, p1->t}, {root->v, root->t}, {root->v, root->t}, Operator::eq));
        else
            buffer.push_back(new Instruction({p1->v, p1->t}, {root->v, root->t}, {root->v, root->t}, Operator::neq));
    }
    // tmp_cnt--;
}

void frontend::Analyzer::analysisRelExp(RelExp *root, vector<ir::Instruction *> &buffer)
{
    GET_CHILD_PTR(p0, AddExp, 0);
    analysisAddExp(p0, buffer);
    root->v = p0->v;
    root->t = p0->t;
    if (SIZE == 1)
        return;
    if (root->v[0] != 't')
    {
        string name = "t" + std::to_string(tmp_cnt);
        if (root->t == Type::Int || root->t == Type::IntLiteral)
        {
            buffer.push_back(new Instruction({root->v, root->t}, {}, {name, Type::Int}, Operator::mov));
            root->v = name; // 不管是什么类型的数，都将其放入一个寄存器中，防止了如果是变量，导致变量名出问题
            root->t = Type::Int;
        }
        else
        {
            buffer.push_back(new Instruction({root->v, root->t}, {}, {name, Type::Float}, Operator::fmov));
            root->v = name; // 不管是什么类型的数，都将其放入一个寄存器中，防止了如果是变量，导致变量名出问题
            root->t = Type::Float;
        }
        tmp_cnt++;
    }
    if (root->t == Type::Int || root->t == Type::IntLiteral)
    {
        for (int i = 2; i < SIZE; i += 2)
        {
            GET_CHILD_PTR(p1, AddExp, i);
            analysisAddExp(p1, buffer);
            GET_CHILD_PTR(pt, Term, i - 1);
            if (p1->v[0] >= '0' && p1->v[0] <= '9')
            {
                p1->t = Type::IntLiteral;
            }
            if (pt->token.type == TokenType::LSS)
                buffer.push_back(new Instruction({root->v, root->t}, {p1->v, p1->t}, {root->v, root->t}, Operator::lss));
            else if (pt->token.type == TokenType::LEQ)
                buffer.push_back(new Instruction({root->v, root->t}, {p1->v, p1->t}, {root->v, root->t}, Operator::leq));
            else if (pt->token.type == TokenType::GTR)
                buffer.push_back(new Instruction({root->v, root->t}, {p1->v, p1->t}, {root->v, root->t}, Operator::gtr));
            else if (pt->token.type == TokenType::GEQ)
                buffer.push_back(new Instruction({root->v, root->t}, {p1->v, p1->t}, {root->v, root->t}, Operator::geq));
        }
    }
    else
    {
        for (int i = 2; i < SIZE; i += 2)
        {
            GET_CHILD_PTR(p1, AddExp, i);
            ExpT = Type::FloatLiteral;
            analysisAddExp(p1, buffer);
            ExpT = Type::IntLiteral;
            GET_CHILD_PTR(pt, Term, i - 1);

            if (pt->token.type == TokenType::LSS)
                buffer.push_back(new Instruction({root->v, root->t}, {p1->v, p1->t}, {root->v, root->t}, Operator::flss));
            else if (pt->token.type == TokenType::LEQ)
                buffer.push_back(new Instruction({root->v, root->t}, {p1->v, p1->t}, {root->v, root->t}, Operator::fleq));
            else if (pt->token.type == TokenType::GTR)
                buffer.push_back(new Instruction({root->v, root->t}, {p1->v, p1->t}, {root->v, root->t}, Operator::fgtr));
            else if (pt->token.type == TokenType::GEQ)
                buffer.push_back(new Instruction({root->v, root->t}, {p1->v, p1->t}, {root->v, root->t}, Operator::fgeq));
        }
        string name = "t" + std::to_string(tmp_cnt++);
        buffer.push_back(new Instruction({root->v, root->t}, {}, {name, Type::Int}, {Operator::cvt_f2i}));
    }
    // tmp_cnt--;
}

void frontend::Analyzer::analysisExp(Exp *root, vector<ir::Instruction *> &buffer)
{
    GET_CHILD_PTR(add_exp, AddExp, 0);
    analysisAddExp(add_exp, buffer);
    COPY_EXP_NODE(add_exp, root);
}

void frontend::Analyzer::analysisAddExp(AddExp *root, vector<ir::Instruction *> &buffer)
{
    GET_CHILD_PTR(p0, MulExp, 0);
    analysisMulExp(p0, buffer);

    COPY_EXP_NODE(p0, root);
    if (SIZE == 1)
        return;
    if (SZ) // 声明数组情况下
    {
        root->t = Type::IntLiteral;
        root->v = p0->v;
        for (int i = 2; i < SIZE; i += 2)
        {
            GET_CHILD_PTR(p1, MulExp, i);
            GET_CHILD_PTR(fh, Term, i - 1);
            analysisMulExp(p1, buffer);
            if (fh->token.type == TokenType::PLUS)
                root->v = std::to_string(stoi(root->v) + stoi(p1->v));
            else
                root->v = std::to_string(stoi(root->v) - stoi(p1->v));
        }
        return;
    }

    if(p0->v[0] != 't')
    {
        std::string tmp_name = "t" + std::to_string(tmp_cnt++);

        if (root->t == Type::IntLiteral || root->t == Type::Int) // 多加了一条指令，将从上层传来的名字覆盖掉
        {
            buffer.push_back(new Instruction({p0->v, p0->t}, {}, {tmp_name, Type::Int}, Operator::mov));
            root->t = Type::Int;
            root->v = tmp_name;
        }
        else if (root->t == Type::FloatLiteral || root->t == Type::Float)
        {
            buffer.push_back(new Instruction({p0->v, p0->t}, {}, {tmp_name, Type::Float}, Operator::fmov));
            root->t = Type::Float;
            root->v = tmp_name;
        }
    }

    string ans = const_n;
    for (int i = 2; i < SIZE; i += 2)
    {
        GET_CHILD_PTR(p1, MulExp, i);
        GET_CHILD_PTR(fh, Term, i - 1);
        analysisMulExp(p1, buffer);
        string tmp_name2 = "t" + std::to_string(tmp_cnt++);
        
        if(const_n != "X")
        {
            float temp = stof(const_n) + stof(ans);
            std::ostringstream oss;
            oss<<std::fixed<<std::setprecision(7)<<temp;
            ans = oss.str();
        }

        switch (root->t)
        {
        case Type::Int:
        {
            if (p1->t == Type::Float || p1->t == Type::FloatLiteral)
            {
                buffer.push_back(new Instruction({root->v, root->t}, {}, {tmp_name2, Type::Float}, Operator::cvt_i2f));
                root->t = Type::Float;
                root->v = tmp_name2;
                if (fh->token.type == TokenType::PLUS)
                {
                    buffer.push_back(new Instruction({root->v, Type::Float}, {p1->v, p1->t}, {root->v, Type::Float}, Operator::fadd));
                }
                else
                {
                    buffer.push_back(new Instruction({root->v, Type::Float}, {p1->v, p1->t}, {root->v, Type::Float}, Operator::fsub));
                }
                break;
            }
            buffer.push_back(new Instruction({p1->v, p1->t}, {}, {tmp_name2, Type::Int}, Operator::mov));
            if (fh->token.type == TokenType::PLUS)
            {
                buffer.push_back(new Instruction({tmp_name2, Type::Int}, {root->v, Type::Int}, {root->v, Type::Int}, Operator::add));
            }
            else
            {
                buffer.push_back(new Instruction({root->v, Type::Int}, {tmp_name2, Type::Int}, {root->v, Type::Int}, Operator::sub));
            }
            // tmp_cnt--;
            break;
        }
        case Type::Float:
        {
            if (p1->t == Type::Int || p1->t == Type::IntLiteral)
                buffer.push_back(new Instruction({p1->v, p1->t}, {}, {tmp_name2, Type::Float}, Operator::cvt_i2f));
            else
                buffer.push_back(new Instruction({p1->v, p1->t}, {}, {tmp_name2, Type::Float}, Operator::fmov));
            if (fh->token.type == TokenType::PLUS)
            {
                buffer.push_back(new Instruction({tmp_name2, Type::Float}, {root->v, Type::Float}, {root->v, Type::Float}, Operator::fadd));
            }
            else
            {
                buffer.push_back(new Instruction({root->v, Type::Float}, {tmp_name2, Type::Float}, {root->v, Type::Float}, Operator::fsub));
            }
            // tmp_cnt--;
            break;
        }
        default:
            break;
        }
    }
    const_n = ans;
    // tmp_cnt--;
}

void frontend::Analyzer::analysisMulExp(MulExp *root, vector<ir::Instruction *> &buffer)
{
    GET_CHILD_PTR(p0, UnaryExp, 0);
    analysisUnaryExp(p0, buffer);
    COPY_EXP_NODE(p0, root);
    if (SIZE == 1)
        return;
    if (SZ) // 声明数组情况下
    {
        for (int i = 2; i < SIZE; i += 2)
        {
            root->t = Type::IntLiteral;
            GET_CHILD_PTR(p1, UnaryExp, i);
            GET_CHILD_PTR(fh, Term, i - 1);
            analysisUnaryExp(p1, buffer);
            if (fh->token.type == TokenType::MULT)
                root->v = std::to_string(stoi(p0->v) * stoi(p1->v));
            else if (fh->token.type == TokenType::DIV)
                root->v = std::to_string(stoi(p0->v) / stoi(p1->v));
            else
                root->v = std::to_string(stoi(p0->v) % stoi(p1->v));
        }
        return;
    }

    if(p0->v[0] != 't')
    {
        std::string tmp_name = "t" + std::to_string(tmp_cnt++);

        if (root->t == Type::IntLiteral || root->t == Type::Int) // 多加了一条指令，将从上层传来的名字覆盖掉
        {
            buffer.push_back(new Instruction({p0->v, p0->t}, {}, {tmp_name, Type::Int}, Operator::mov));
            root->t = Type::Int;
            root->v = tmp_name;
        }
        else if (root->t == Type::FloatLiteral || root->t == Type::Float)
        {
            buffer.push_back(new Instruction({p0->v, p0->t}, {}, {tmp_name, Type::Float}, Operator::fmov));
            root->t = Type::Float;
            root->v = tmp_name;
        }
    }

    string ans = const_n;
    for (int i = 2; i < SIZE; i += 2)
    {
        GET_CHILD_PTR(p1, UnaryExp, i);
        GET_CHILD_PTR(fh, Term, i - 1);
        analysisUnaryExp(p1, buffer);
        string tmp_name2 = "t" + std::to_string(tmp_cnt++);

        if(const_n != "X")
        {
            float temp = stof(const_n) * stof(ans);
            std::ostringstream oss;
            oss<<std::fixed<<std::setprecision(7)<<temp;
            ans = oss.str();
        }

        switch (root->t)
        {
        case Type::Int:
        {
            if (p1->t == Type::Float || p1->t == Type::FloatLiteral)
            {
                buffer.push_back(new Instruction({root->v, root->t}, {}, {tmp_name2, Type::Float}, Operator::cvt_i2f));
                root->v = tmp_name2;
                root->t = Type::Float;
                if (fh->token.type == TokenType::MULT)
                {
                    buffer.push_back(new Instruction({root->v, Type::Float}, {p1->v, p1->t}, {root->v, Type::Float}, Operator::fmul));
                }
                else if (fh->token.type == TokenType::DIV)
                {
                    buffer.push_back(new Instruction({root->v, Type::Float}, {p1->v, p1->t}, {root->v, Type::Float}, Operator::fdiv));
                }
                break;
            }
            buffer.push_back(new Instruction({p1->v, p1->t}, {}, {tmp_name2, Type::Int}, Operator::mov));
            if (fh->token.type == TokenType::MULT)
            {
                buffer.push_back(new Instruction({tmp_name2, Type::Int}, {root->v, Type::Int}, {root->v, Type::Int}, Operator::mul));
            }
            else if (fh->token.type == TokenType::DIV)
            {
                buffer.push_back(new Instruction({root->v, Type::Int}, {tmp_name2, Type::Int}, {root->v, Type::Int}, Operator::div));
            }
            else
            {
                buffer.push_back(new Instruction({root->v, Type::Int}, {tmp_name2, Type::Int}, {root->v, Type::Int}, Operator::mod));
            }
            // tmp_cnt--;
            break;
        }
        case Type::Float:
        {
            if (p1->t == Type::Int || p1->t == Type::IntLiteral)
                buffer.push_back(new Instruction({p1->v, p1->t}, {}, {tmp_name2, Type::Float}, Operator::cvt_i2f));
            else
                buffer.push_back(new Instruction({p1->v, p1->t}, {}, {tmp_name2, Type::Float}, Operator::fmov));
            if (fh->token.type == TokenType::MULT)
            {
                buffer.push_back(new Instruction({tmp_name2, Type::Float}, {root->v, Type::Float}, {root->v, Type::Float}, Operator::fmul));
            }
            else if (fh->token.type == TokenType::DIV)
            {
                buffer.push_back(new Instruction({root->v, Type::Float}, {tmp_name2, Type::Float}, {root->v, Type::Float}, Operator::fdiv));
            }
            // tmp_cnt--;
            break;
        }
        default:
            break;
        }
    }
    const_n = ans;
    // tmp_cnt--;
}
// UnaryExp->PrimaryExp | Ident '(' [FuncRParams] ')' | UnaryOp UnaryExp
void frontend::Analyzer::analysisUnaryExp(UnaryExp *root, vector<ir::Instruction *> &buffer)
{
    if (PrimaryExp *node = dynamic_cast<PrimaryExp *>(root->children[0]))
    {
        analysisPrimaryExp(node, buffer);
        COPY_EXP_NODE(node, root);
    }
    else if (UnaryOp *node = dynamic_cast<UnaryOp *>(root->children[0]))
    {
        analysisUnaryOp(node);
        GET_CHILD_PTR(p1, UnaryExp, 1);
        analysisUnaryExp(p1, buffer);
        COPY_EXP_NODE(p1, root);
        if (node->op == TokenType::PLUS)
            return;
        string name = "t" + std::to_string(tmp_cnt++);
        if (node->op == TokenType::MINU)
        {
            if (p1->t == Type::Float || p1->t == Type::FloatLiteral)
                buffer.push_back(new Instruction({"0.0", Type::FloatLiteral}, {p1->v, p1->t}, {name, Type::Float}, Operator::fsub));
            else
                buffer.push_back(new Instruction({"0", Type::IntLiteral}, {p1->v, p1->t}, {name, Type::Int}, Operator::sub));
            if(const_n != "X")
                const_n = "-" + const_n;
        }
        else if (node->op == TokenType::NOT)
        {
            if (p1->t == Type::Int || p1->t == Type::IntLiteral)
                buffer.push_back(new Instruction({p1->v, p1->t}, {}, {name, Type::Int}, Operator::_not));
            else
            {
                buffer.push_back(new Instruction({p1->v, p1->t}, {}, {name, Type::Int}, Operator::cvt_f2i));
                buffer.push_back(new Instruction({name, Type::Int}, {}, {name, Type::Int}, Operator::_not));
            }
            root->v = name;
            root->t = Type::Int;
            return;
        }
        // tmp_cnt--;
        root->v = name;
        root->t = (p1->t == Type::Float || p1->t == Type::FloatLiteral) ? Type::Float : Type::Int;
    }
    else if (Term *node = dynamic_cast<Term *>(root->children[0]))
    {
        GET_CHILD_PTR(ident, Term, 0);
        Function *func;
        if (symbol_table.functions.count(ident->token.value)) // 如果是用户自定义函数
        {
            func = symbol_table.functions[ident->token.value];
        }
        else if (get_lib_funcs()->count(ident->token.value)) // 如果是库函数
        {
            func = (*get_lib_funcs())[ident->token.value];
        }
        root->v = "t" + std::to_string(tmp_cnt++);
        root->t = func->returnType;

        if (root->t != ir::Type::null)
        {
            buffer.push_back(new ir::Instruction(                                                           // int a = 2;
                ir::Operand("0", (root->t == Type::Float) ? ir::Type::FloatLiteral : ir::Type::IntLiteral), //
                ir::Operand(),                                                                              //
                {root->v, root->t},                                                                         //
                (root->t == Type::Float) ? ir::Operator::fdef : ir::Operator::def));
        }

        if (SIZE == 3) // 无参
        {
            buffer.push_back(new ir::CallInst({ident->token.value, func->returnType}, {root->v, root->t}));
        }
        else
        {
            std::vector<ir::Operand> params;
            GET_CHILD_PTR(p00, FuncRParams, 2);
            for (auto it : func->ParameterList)
            {
                params.push_back(it);
            }
            analysisFuncRParams(p00, buffer, func, params);
            buffer.push_back(new ir::CallInst({ident->token.value, func->returnType}, params, {root->v, root->t}));
        }
    }
    else
        assert(0 && "Unknown UnaryExp type");
}

void frontend::Analyzer::analysisFuncRParams(FuncRParams *root, vector<ir::Instruction *> &buffer, ir::Function *func, vector<ir::Operand> &params)
{ // 未处理int调用float型，问题？如果exp是立即数怎么处理，名字没有更改
    for (int i = 0; i < SIZE; i += 2)
    {
        if (params[i / 2].type == Type::Float)
            ExpT = Type::FloatLiteral;
        GET_CHILD_PTR(exp, Exp, i);
        analysisExp(exp, buffer);
        ExpT = Type::IntLiteral;
        if ((params[i / 2].type == Type::Int || params[i / 2].type == Type::IntLiteral) && (exp->t == Type::Float || exp->t == Type::FloatLiteral))
        {
            string name = "t" + std::to_string(tmp_cnt++);
            buffer.push_back(new Instruction({exp->v, exp->t}, {}, {name, Type::Int}, Operator::cvt_f2i));
            params[i / 2] = Operand(name, Type::Int);
            continue;
        }
        params[i / 2] = Operand(exp->v, exp->t);
    }
}

void frontend::Analyzer::analysisUnaryOp(UnaryOp *root)
{
    GET_CHILD_PTR(p0, Term, 0);
    root->op = p0->token.type;
}

// PrimaryExp->'(' Exp ')' | LVal | Number
void frontend::Analyzer::analysisPrimaryExp(PrimaryExp *root, vector<ir::Instruction *> &buffer)
{
    if (Term *node = dynamic_cast<Term *>(root->children[0]))
    {
        GET_CHILD_PTR(exp, Exp, 1);
        analysisExp(exp, buffer);
        COPY_EXP_NODE(exp, root);
    }
    else if (Number *node = dynamic_cast<Number *>(root->children[0]))
    {
        analysisNumber(node);
        COPY_EXP_NODE(node, root);
    }
    else if (LVal *node = dynamic_cast<LVal *>(root->children[0]))
    {
        analysisLVal(node, buffer, false);
        COPY_EXP_NODE(node, root);
    }
    else
        assert(0 && "Unknown PrimaryExp type");
}

void frontend::Analyzer::analysisLVal(LVal *root, vector<ir::Instruction *> &buffer, bool p) // 一个是从数组取数，一个是将从数组存数
{
    GET_CHILD_PTR(ident, Term, 0);
    auto ste = symbol_table.get_ste(ident->token.value);

    root->v = ste.operand.name;
    root->t = ste.operand.type;

    if(const_n != "X")
        const_n = root->v;

    if (SIZE == 1)
        return;

    // 处理数组访问（优化临时变量使用）
    Operand base = ste.operand;
    std::string offset_tmp = "t" + std::to_string(tmp_cnt++); // 使用固定临时变量
    buffer.push_back(new Instruction({"0", Type::IntLiteral}, {}, {offset_tmp, Type::Int}, Operator::def));

    // 遍历数组维度索引
    for (int i = 2; i < SIZE; i += 3)
    {
        // 分析索引表达式
        GET_CHILD_PTR(exp, Exp, i);
        analysisExp(exp, buffer);

        // 复用t1作为当前维度偏移
        std::string dim_tmp = "t" + std::to_string(tmp_cnt);
        buffer.push_back(new Instruction({exp->v, exp->t}, {}, {dim_tmp, Type::Int}, Operator::mov));
        tmp_cnt++; // 占用t1

        // 乘以前面所有维度的乘积
        for (int j = (i - 2) / 3 + 1; j < ste.dimension.size(); j++)
        {
            buffer.push_back(new Instruction(
                {dim_tmp, Type::Int},
                {std::to_string(ste.dimension[j]), Type::IntLiteral},
                {dim_tmp, Type::Int}, // 复用dim_tmp存储结果
                Operator::mul));
        }

        // 累加到总偏移（复用offset_tmp）
        buffer.push_back(new Instruction(
            {offset_tmp, Type::Int},
            {dim_tmp, Type::Int},
            {offset_tmp, Type::Int}, // 结果存回offset_tmp
            Operator::add));

        // tmp_cnt--; // 释放dim_tmp（t1）
    }

    // tmp_cnt -= 1; // 释放offset_tmp（t0）

    if (p) // 取数组坐标
    {
        root->i = offset_tmp;
        return;
    }
    // 生成最终地址（使用数组相关名称）
    std::string addr_name = ste.operand.name + "_elem";
    buffer.push_back(new Instruction(
        base,
        {offset_tmp, Type::Int},
        {addr_name, (base.type == Type::IntPtr) ? Type::Int : Type::Float},
        Operator::load));

    // 释放所有临时变量并设置最终结果
    root->v = addr_name;
    root->t = (base.type == Type::IntPtr) ? Type::Int : Type::Float;
}

void frontend::Analyzer::analysisNumber(Number *root) // solve
{
    GET_CHILD_PTR(term, Term, 0);
    string num_str = term->token.value;

    if (term->token.type == TokenType::INTLTR)
    {
        root->t = ExpT;
        if (num_str.find("0x") == 0 || num_str.find("0X") == 0)
        {
            root->v = std::to_string(std::stoi(num_str.substr(2), 0, 16));
        }
        else if (num_str.find("0b") == 0 || num_str.find("0B") == 0)
        {
            root->v = std::to_string(std::stoi(num_str.substr(2), 0, 2));
        }
        else if (num_str[0] == '0' && num_str.size() > 1)
        {
            root->v = std::to_string(std::stoi(num_str.substr(1), 0, 8));
        }
        else if (num_str[0] == '.')
        {
            root->v = num_str.insert(0, "0");
            root->t = Type::FloatLiteral;
        }
        else
        {
            root->v = num_str;
        }
    }
    else
    {
        root->t = Type::FloatLiteral;
        if (num_str[0] == '.')
            num_str.insert(0, "0");
        root->v = num_str;
    }
    if(const_n != "X")
        const_n = root->v;
}

void frontend::Analyzer::analysisDecl(Decl *root, vector<ir::Instruction *> &buffer)
{
    if (ConstDecl *node = dynamic_cast<ConstDecl *>(root->children[0]))
        analysisConstDecl(node, buffer);
    else if (VarDecl *node = dynamic_cast<VarDecl *>(root->children[0]))
        analysisVarDecl(node, buffer);
    else
        assert(0 && "Unknown Decl type");
}

void frontend::Analyzer::analysisConstDecl(ConstDecl *root, vector<ir::Instruction *> &buffer)
{
    GET_CHILD_PTR(p0, BType, 1);
    Type type = analysisBType(p0);
    for (int i = 2; i < SIZE; i += 2)
    {
        GET_CHILD_PTR(p1, ConstDef, i);
        analysisConstDef(p1, type, buffer);
    }
}

void frontend::Analyzer::analysisConstDef(ConstDef *root, Type type, vector<ir::Instruction *> &buffer)
{
    GET_CHILD_PTR(p0, Term, 0);
    std::string ident = p0->token.value;

    // 生成带作用域后缀的变量名
    std::string scoped_name = symbol_table.get_scoped_name(ident);

    STE ste;
    ste.operand = Operand(scoped_name, type);

    if (SIZE == 3)
    {
        GET_CHILD_PTR(val, ConstInitVal, 2);
        const_n = "";
        analysisConstInitVal(val, type, buffer);
        if(type == Type::Int && const_n.find(".")!=-1)
        {
            const_n.erase(const_n.find("."));
        }
        ste.operand.name = const_n;
        const_n = "X";

        ste.operand.type = (type == Type::Int) ? Type::IntLiteral : Type::FloatLiteral;
        symbol_table.scope_stack.back().table[ident] = ste;

        if (type == Type::Float)
            buffer.push_back(new Instruction(
                {val->vecV[0].name, val->vecV[0].type},
                {},
                {ident, Type::Float},
                Operator::fdef));
        else
            buffer.push_back(new Instruction(
                {val->vecV[0].name, val->vecV[0].type},
                {},
                {ident, Type::Int},
                Operator::def));
        return;
    }
    // 数组
    int i = 1, Len = 1;
    Term *p1 = nullptr;

    while (i < SIZE)
    {
        // 获取当前节点并检查类型
        GET_CHILD_PTR(current, Term, i);
        if (current->token.type != TokenType::LBRACK)
            break;

        // 处理维度表达式
        i++; // 跳过LBRACK
        GET_CHILD_PTR(pk, ConstExp, i);
        SZ = true;
        if (type == Type::Float || type == Type::FloatLiteral)
            ExpT = Type::FloatLiteral;
        analysisConstExp(pk, buffer);
        ExpT = Type::IntLiteral;
        SZ = false;
        Len *= stoi(pk->v);
        ste.dimension.push_back(stoi(pk->v));

        i += 2;                                       // 跳过ConstExp和RBRACK
        p1 = dynamic_cast<Term *>(root->children[i]); // 更新p1指针
    }

    i++;
    ste.operand.type = (type == Type::Int) ? Type::IntPtr : Type::FloatPtr;
    symbol_table.scope_stack.back().table[ident] = ste;

    buffer.push_back(new Instruction(
        {std::to_string(Len), Type::IntLiteral},
        {},
        ste.operand,
        Operator::alloc));
    GET_CHILD_PTR(p2, ConstInitVal, i);
    analysisConstInitVal(p2, type, buffer);
    for (int j = 0; j < (int)p2->vecV.size(); j++)
    {
        buffer.push_back(new ir::Instruction(
            ste.operand,
            {std::to_string(j), Type::IntLiteral},
            p2->vecV[j],
            Operator::store));
    }
}

void frontend::Analyzer::analysisConstInitVal(ConstInitVal *root, Type type, vector<ir::Instruction *> &buffer)
{
    if (ConstExp *node = dynamic_cast<ConstExp *>(root->children[0]))
    {
        if (type == Type::Float || type == Type::FloatLiteral)
            ExpT = Type::FloatLiteral;
        analysisConstExp(node, buffer);
        ExpT = Type::IntLiteral;
        std::string temp_name = "t" + std::to_string(tmp_cnt++);
        Operand dest(temp_name, type);

        // 类型检查和转换逻辑
        switch (type)
        {
        case ir::Type::Int:
            if (node->t == ir::Type::FloatLiteral)
            {
                node->t = ir::Type::IntLiteral;
                node->v = std::to_string((int)std::stof(node->v));
            }
            else if (node->t == ir::Type::Float)
            {
                buffer.push_back(new Instruction(
                    {node->v, ir::Type::Float}, {}, {node->v, ir::Type::Int},
                    Operator::cvt_f2i));
                node->t = ir::Type::Int;
            }
            break;
        case ir::Type::Float:
            if (node->t == ir::Type::IntLiteral)
            {
                node->t = ir::Type::FloatLiteral;
                node->v = std::to_string((float)std::stoi(node->v));
            }
            else if (node->t == ir::Type::Int)
            {
                buffer.push_back(new Instruction(
                    {node->v, ir::Type::Int}, {}, {node->v, ir::Type::Float},
                    Operator::cvt_i2f));
                node->t = ir::Type::Float;
            }
            break;
        default:
            assert(0 && "Unsupported type in initval");
        }
        root->vecV.push_back({node->v, node->t});
        // tmp_cnt--;
    }
    else if (Term *node = dynamic_cast<Term *>(root->children[0]))
    {
        for (int i = 1; i < SIZE; i += 2)
        {
            GET_CHILD_PTR(p0, ConstInitVal, i);
            analysisConstInitVal(p0, type, buffer);
            for (auto j : p0->vecV)
                root->vecV.push_back(j);
        }
    }
    else
        assert(0 && "Unknown InitVal type");
}

void frontend::Analyzer::analysisConstExp(ConstExp *root, vector<ir::Instruction *> &buffer)
{
    GET_CHILD_PTR(add_exp, AddExp, 0);
    analysisAddExp(add_exp, buffer);
    COPY_EXP_NODE(add_exp, root);
}

// VarDecl->BType VarDef { ',' VarDef } ';
void frontend::Analyzer::analysisVarDecl(VarDecl *root, vector<ir::Instruction *> &buffer)
{
    GET_CHILD_PTR(p0, BType, 0);
    Type type = analysisBType(p0);
    for (int i = 1; i < SIZE; i += 2)
    {
        GET_CHILD_PTR(p1, VarDef, i);
        analysisVarDef(p1, type, buffer);
    }
}

ir::Type frontend::Analyzer::analysisBType(BType *root)
{
    GET_CHILD_PTR(p0, Term, 0);
    root->t = (p0->token.type == TokenType::INTTK) ? Type::Int : Type::Float;
    return root->t;
}

void frontend::Analyzer::analysisVarDef(VarDef *root, ir::Type type, vector<ir::Instruction *> &buffer)
{
    GET_CHILD_PTR(p0, Term, 0);
    std::string ident = p0->token.value;

    // 生成带作用域后缀的变量名
    std::string scoped_name = symbol_table.get_scoped_name(ident);

    STE ste;
    ste.operand = Operand(scoped_name, type);

    if (SIZE == 1) // 只声明不赋值
    {
        symbol_table.scope_stack.back().table[ident] = ste;
        switch (type)
        {
        case Type::Int:
            buffer.push_back(new Instruction({"0", Type::IntLiteral}, {}, ste.operand, Operator::def));
            break;
        case Type::Float:
            buffer.push_back(new Instruction({"0.0", Type::FloatLiteral}, {}, ste.operand, Operator::fdef));
            break;
        default:
            break;
        }
        return;
    }

    int i = 1, Len = 1;
    Term *p1 = nullptr; // 声明在循环外部

    // 循环处理每个数组维度
    while (i < SIZE)
    {
        // 获取当前节点并检查类型
        GET_CHILD_PTR(current, Term, i);
        if (current->token.type != TokenType::LBRACK)
            break;

        // 处理维度表达式
        i++; // 跳过LBRACK
        GET_CHILD_PTR(pk, ConstExp, i);
        SZ = 1;
        analysisConstExp(pk, buffer);
        SZ = 0;
        Len *= stoi(pk->v);
        ste.dimension.push_back(stoi(pk->v));

        i += 2; // 跳过ConstExp和RBRACK
        // p1 = dynamic_cast<Term *>(root->children[i]); // 更新p1指针
    }

    i++;
    if (!ste.dimension.empty())
    {
        ste.operand.type = (type == Type::Int) ? Type::IntPtr : Type::FloatPtr;
        symbol_table.scope_stack.back().table[ident] = ste;

        buffer.push_back(new Instruction(
            {std::to_string(Len), Type::IntLiteral},
            {},
            ste.operand,
            Operator::alloc));
        int j = 0;
        if (SIZE - 1 != i)
        {
            // for (; j < Len; j++)
            // {
            //     buffer.push_back(new ir::Instruction(
            //         ste.operand,
            //         {std::to_string(j), Type::IntLiteral},
            //         {"0", Type::IntLiteral},
            //         Operator::store));
            // }
            return;
        }
        GET_CHILD_PTR(p2, InitVal, i);
        analysisInitVal(p2, type, buffer);
        for (; j < (int)p2->vecV.size(); j++)
        {
            buffer.push_back(new ir::Instruction(
                ste.operand,
                {std::to_string(j), Type::IntLiteral},
                p2->vecV[j],
                Operator::store));
        }
        for (; j < Len && namespa != "global"; j++)
        {
            if (type == Type::Int)
                buffer.push_back(new ir::Instruction(
                    ste.operand,
                    {std::to_string(j), Type::IntLiteral},
                    {"0", Type::IntLiteral},
                    Operator::store));
            else
                buffer.push_back(new ir::Instruction(
                    ste.operand,
                    {std::to_string(j), Type::IntLiteral},
                    {"0", Type::FloatLiteral},
                    Operator::store));
        }
    }
    else
    {
        symbol_table.scope_stack.back().table[ident] = ste;
        GET_CHILD_PTR(p2, InitVal, i);
        analysisInitVal(p2, type, buffer);
        // 添加变量声明指令
        if (type == Type::Int)
            buffer.push_back(new Instruction(
                {p2->vecV[0].name, p2->vecV[0].type},
                {},
                ste.operand,
                Operator::def));
        else
            buffer.push_back(new Instruction(
                {p2->vecV[0].name, p2->vecV[0].type},
                {},
                ste.operand,
                Operator::fdef));
    }
}

// InitVal->Exp | '{' [ InitVal { ',' InitVal } ] '}'
void frontend::Analyzer::analysisInitVal(InitVal *root, ir::Type type, vector<ir::Instruction *> &buffer)
{
    if (Exp *node = dynamic_cast<Exp *>(root->children[0]))
    {
        if (type == Type::Float)
            ExpT = Type::FloatLiteral;
        analysisExp(node, buffer);
        ExpT = Type::IntLiteral;
        std::string temp_name = "t" + std::to_string(tmp_cnt);
        Operand dest(temp_name, type);

        // 类型检查和转换逻辑
        if (!(node->v == "0" || node->v == "0.0"))
        {
            switch (type)
            {
            case ir::Type::Int:
                if (node->t == ir::Type::FloatLiteral)
                {
                    node->t = ir::Type::IntLiteral;
                    node->v = std::to_string((int)std::stof(node->v));
                }
                else if (node->t == ir::Type::Float)
                {
                    buffer.push_back(new Instruction(
                        {node->v, ir::Type::Float}, {}, {temp_name, ir::Type::Int},
                        Operator::cvt_f2i));
                    node->t = ir::Type::Int;
                    node->v = temp_name;
                    tmp_cnt++;
                }
                // buffer.push_back(new Instruction(
                //     {node->v, node->t}, {}, dest, Operator::mov));
                break;
            case ir::Type::Float:
                if (node->t == ir::Type::IntLiteral)
                {
                    node->t = ir::Type::FloatLiteral;
                    node->v = std::to_string((float)std::stoi(node->v));
                }
                else if (node->t == ir::Type::Int)
                {
                    buffer.push_back(new Instruction(
                        {node->v, ir::Type::Int}, {}, {temp_name, ir::Type::Float},
                        Operator::cvt_i2f));
                    node->t = ir::Type::Float;
                    node->v = temp_name;
                    tmp_cnt++;
                }
                // buffer.push_back(new Instruction(
                //     {node->v, node->t}, {}, dest, Operator::fmov));
                break;
            default:
                assert(0 && "Unsupported type in initval");
            }
        }
        root->vecV.push_back({node->v, node->t});
        // tmp_cnt--;
    }
    else if (SIZE == 2)
        return;
    else if (Term *node = dynamic_cast<Term *>(root->children[0]))
    {
        for (int i = 1; i < SIZE; i += 2)
        {
            GET_CHILD_PTR(p0, InitVal, i);
            analysisInitVal(p0, type, buffer);
            for (auto j : p0->vecV)
                root->vecV.push_back(j);
        }
    }
    else
        assert(0 && "Unknown InitVal type");
}

ir::Program frontend::Analyzer::get_ir_program(CompUnit *root)
{ // 最后结果输出
    map<std::string, ir::Function *> libFuncs = *get_lib_funcs();
    for (auto libFunc = libFuncs.begin(); libFunc != libFuncs.end(); libFunc++)
    {
        symbol_table.functions[libFunc->first] = libFunc->second;
    }

    ir::Program program;

    // 显式创建全局初始化函数（使用正确的构造函数）
    Function global_init;
    global_init.name = "_global";
    program.addFunction(global_init);
    symbol_table.functions["_global"] = &global_init;

    symbol_table.add_scope();        // 添加全局作用域
    analysisCompUnit(root, program); // 从根节点开始分析AST

    // 添加全局初始化指令到_global函数
    for (auto i : program.functions[0].InstVec)
        global_init.InstVec.push_back(i);
    program.functions[0].addInst(new Instruction({}, {}, {}, Operator::_return));

    // 在main函数开头插入调用_global的指令
    auto call_global = new ir::CallInst(Operand("_global", ir::Type::null), Operand());

    for (auto &i : program.functions)
    {
        if (i.name == "main")
        {
            i.InstVec.insert(i.InstVec.begin(), call_global);
            break;
        }
    }

    for (auto i : symbol_table.scope_stack[0].table)
    {
        int size = 1;
        for (auto j : i.second.dimension)
            size = size * j;
        if (size == 1)
        {
            if(isalpha(i.second.operand.name[0]) || i.second.operand.name[0] == '_')
                program.globalVal.push_back(ir::GlobalVal(i.second.operand));
            else
            {
                Operand temp = i.second.operand;
                temp.name = i.first;
                program.globalVal.push_back(ir::GlobalVal(temp));
            }
        }
        else
            program.globalVal.push_back(ir::GlobalVal(i.second.operand, size));
    }

    return program;
}
