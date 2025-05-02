#include "front/semantic.h"

#include <cassert>

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

vector<ir::Operand> SZ;

void frontend::SymbolTable::add_scope()
{
    int cnt = scope_stack.size();
    scope_stack.push_back({cnt, "Scope" + STR(cnt)});
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
    symbol_table.add_scope();
    GET_CHILD_PTR(ftype, FuncType, 0);
    auto type = analysisFuncType(ftype);
    GET_CHILD_PTR(fname, Term, 1);
    auto name = fname->token.value;

    std::vector<Operand> params;
    if (SIZE > 5)
        TODO;
    // 确保main函数被正确创建并添加到program
    func = new Function(name, params, type); // 使用正确的构造函数
    symbol_table.functions[name] = func;     // 添加到函数表
    namespa = name;
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
    if (p) // 不是从函数进入时，而是从while，if等进入block
    {
        symbol_table.add_scope();
        TODO;
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
    if (root->children[0]->type == NodeType::TERMINAL) // 首字符为终结符
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
                    assert(0 && "to be continue");
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
        default:
            break;
        }
    }
    else
    {
        GET_CHILD_PTR(lval, LVal, 0);
        GET_CHILD_PTR(exp, Exp, 2);
        switch (root->children[0]->type)
        {
        case NodeType::LVAL:
        {
            analysisLVal(lval, buffer);
            analysisExp(exp, buffer);
            Operand tx("t" + std::to_string(tmp_cnt++), lval->t);
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
            tmp_cnt--;
            break;
        }
        default:
            break;
        }
    }
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

    std::string tmp_name = "t" + std::to_string(tmp_cnt++);
    if (root->t == Type::IntLiteral)
    {
        buffer.push_back(new Instruction({p0->v, p0->t}, {}, {tmp_name, Type::Int}, Operator::mov));
        root->t = Type::Int;
        root->v = tmp_name;
    }
    else if (root->t == Type::FloatLiteral)
    {
    }
    for (int i = 2; i < SIZE; i += 2)
    {
        GET_CHILD_PTR(p1, MulExp, i);
        GET_CHILD_PTR(fh, Term, i - 1);
        analysisMulExp(p1, buffer);
        string tmp_name2 = "t" + std::to_string(tmp_cnt++);
        switch (root->t)
        {
        case Type::Int:
        {
            if (p1->t == Type::Float || p1->t == Type::FloatLiteral)
                buffer.push_back(new Instruction({p1->v, p1->t}, {}, {tmp_name2, Type::Int}, Operator::cvt_f2i));
            else
                buffer.push_back(new Instruction({p1->v, p1->t}, {}, {tmp_name2, Type::Int}, Operator::mov));
            string tmp_name3 = "t" + std::to_string(tmp_cnt);
            if (fh->token.type == TokenType::PLUS)
            {
                buffer.push_back(new Instruction({tmp_name2, Type::Int}, {root->v, Type::Int}, {tmp_name3, Type::Int}, Operator::add));
            }
            else
            {
                buffer.push_back(new Instruction({root->v, Type::Int}, {tmp_name2, Type::Int}, {tmp_name3, Type::Int}, Operator::sub));
            }
            root->v = tmp_name3;
            break;
        }
        case Type::Float:
        {
            break;
        }
        default:
            break;
        }
        tmp_cnt--;
    }
}

void frontend::Analyzer::analysisMulExp(MulExp *root, vector<ir::Instruction *> &buffer)
{
    GET_CHILD_PTR(p0, UnaryExp, 0);
    analysisUnaryExp(p0, buffer);

    if (SIZE == 1)
        COPY_EXP_NODE(p0, root);
    return;
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
    }
    else if (Term *node = dynamic_cast<Term *>(root->children[0]))
    {
    }
    else
        assert(0 && "Unknown UnaryExp type");
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
        analysisLVal(node, buffer);
        COPY_EXP_NODE(node, root);
    }
    else
        assert(0 && "Unknown PrimaryExp type");
}

void frontend::Analyzer::analysisLVal(LVal *root, vector<ir::Instruction *> &buffer)
{
    GET_CHILD_PTR(ident, Term, 0);
    auto ste = symbol_table.get_ste(ident->token.value); // 找出所对应的一条记录

    root->v = ste.operand.name;
    root->t = ste.operand.type;

    if (SIZE == 1)
        return;
}

void frontend::Analyzer::analysisNumber(Number *root) // solve
{
    GET_CHILD_PTR(term, Term, 0);
    string num_str = term->token.value;

    if (term->token.type == TokenType::INTLTR)
    {
        root->t = Type::IntLiteral;
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
        else
        {
            root->v = num_str;
        }
    }
    else
    {
        root->t = Type::FloatLiteral;
        root->v = num_str;
    }
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

    // 添加到当前作用域（类型信息已包含在operand中）
    symbol_table.scope_stack.back().table[ident] = ste;

    if (SIZE == 3)
    {
        GET_CHILD_PTR(val, ConstInitVal, 2);
        analysisConstInitVal(val, type, buffer);

        if (type == Type::Float)
            buffer.push_back(new Instruction(
                {val->v, val->t},
                {},
                ste.operand,
                Operator::fdef));
        else
            buffer.push_back(new Instruction(
                {val->v, val->t},
                {},
                ste.operand,
                Operator::def));
        return;
    }
}

void frontend::Analyzer::analysisConstInitVal(ConstInitVal *root, Type type, vector<ir::Instruction *> &buffer)
{
    if (ConstExp *node = dynamic_cast<ConstExp *>(root->children[0]))
    {
        analysisConstExp(node, buffer);
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
        COPY_EXP_NODE(node, root);
        tmp_cnt--;
    }
    else if (Term *node = dynamic_cast<Term *>(root->children[0]))
    {
        // 处理数组初始化的情况（暂不实现）
        TODO;
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
    return (p0->token.type == TokenType::INTTK) ? Type::Int : Type::Float;
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
    GET_CHILD_PTR(p1, Term, i);
    for (; p1->token.type == TokenType::LBRACK;)
    {
        i++;
        GET_CHILD_PTR(pk, ConstExp, i)
        analysisConstExp(pk, buffer);
        Len *= stoi(pk->v);
        ste.dimension.push_back(stoi(pk->v));
        i += 2;
        GET_CHILD_PTR(p1, Term, i);
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
        if (SIZE - 1 != i)
            return;
        GET_CHILD_PTR(p2, InitVal, i);
        analysisInitVal(p2, type, buffer);
        SZ.clear();
    }
    else
    {
        symbol_table.scope_stack.back().table[ident] = ste;
        if (p1->token.type == TokenType::ASSIGN)
        {
            GET_CHILD_PTR(p2, InitVal, i);
            analysisInitVal(p2, type, buffer);
            // 添加变量声明指令
            if (type == Type::Int)
                buffer.push_back(new Instruction(
                    {p2->v, p2->t},
                    {},
                    ste.operand,
                    Operator::def));
            else
                buffer.push_back(new Instruction(
                    {p2->v, p2->t},
                    {},
                    ste.operand,
                    Operator::fdef));
        }
    }
}

// InitVal->Exp | '{' [ InitVal { ',' InitVal } ] '}'
void frontend::Analyzer::analysisInitVal(InitVal *root, ir::Type type, vector<ir::Instruction *> &buffer)
{
    if (Exp *node = dynamic_cast<Exp *>(root->children[0]))
    {
        analysisExp(node, buffer);
        std::string temp_name = "t" + std::to_string(tmp_cnt++);
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
                        {node->v, ir::Type::Float}, {}, {node->v, ir::Type::Int},
                        Operator::cvt_f2i));
                    node->t = ir::Type::Int;
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
                        {node->v, ir::Type::Int}, {}, {node->v, ir::Type::Float},
                        Operator::cvt_i2f));
                    node->t = ir::Type::Float;
                }
                // buffer.push_back(new Instruction(
                //     {node->v, node->t}, {}, dest, Operator::fmov));
                break;
            default:
                assert(0 && "Unsupported type in initval");
            }
        }
        COPY_EXP_NODE(node, root);
        tmp_cnt--;
    }
    else if (Term *node = dynamic_cast<Term *>(root->children[0]))
    {
        for (int i = 1; i < SIZE; i += 2)
        {
            GET_CHILD_PTR(p0, InitVal, i);
            analysisInitVal(p0,type,buffer);
            
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
            program.globalVal.push_back(ir::GlobalVal(i.second.operand));
        else
            program.globalVal.push_back(ir::GlobalVal(i.second.operand, size));
    }

    return program;
}
