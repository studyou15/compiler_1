#include "backend/generator.h"

#include <assert.h>

#define TODO assert(0 && "todo")

int frame_size = 520;
int Instr_ID = 0;

backend::Generator::Generator(ir::Program &p, std::ofstream &f) : program(p), fout(f), global_vars() {}

int backend::stackVarMap::find_operand(ir::Operand root)
{
    if (_table.find(root.name) == _table.end())
        return -1; // 全局
    else
        return _table[root.name]; // 局部地址
}

void backend::stackVarMap::add_operand(ir::Operand root, unsigned int size)
{
    auto it = _table.find(root.name);
    if (it != _table.end())
        return;
    _table[root.name] = offset;
    offset = offset - size;
}

void backend::Generator::gen()
{
    fout << "\t.option nopic\n";
    fout << "\t.data\n";

    GlobalInit(program.globalVal);
    fout << "\t.text\n";

    for (auto it : program.functions)
    {
        funcSolve(it);
    }
}

void backend::Generator::GlobalInit(const std::vector<ir::GlobalVal> &global)
{
    for (auto it : global)
    {
        // 将全局变量添加到全局变量集合中
        global_vars.insert(it.val.name);

        if (it.maxlen == 0)
        {
            fout << it.val.name + ":\n";
            fout << "\t.word\t0\n";
        }
        else
        {
            fout << it.val.name + ":\n";
            fout << "\t.space\t" + std::to_string(it.maxlen * 4) + "\n";
        }
    }
}

void backend::Generator::funcSolve(const ir::Function &func)
{
    // 重置变量映射表，为新函数准备
    varMap = stackVarMap();

    // 保存当前处理的函数名
    current_func_name = func.name;

    // 是否可以不保存s1->s3的寄存器
    fout << "\t.align\t2\n";
    fout << "\t.globl\t" + func.name + "\n";
    fout << "\t.type\t" + func.name + ",\t" + "@function" + "\n";
    fout << func.name + ":\n";

    fout << "\taddi\tsp,sp,-" + std::to_string(frame_size) + "\n";
    fout << "\tsw\tra," + std::to_string(frame_size - 4) + "(sp)\n";
    fout << "\tsw\tt0," + std::to_string(frame_size - 8) + "(sp)\n";
    fout <<"\tsw\tt1," + std::to_string(frame_size - 12) + "(sp)\n";
    fout <<"\tsw\tt2," + std::to_string(frame_size - 16) + "(sp)\n";
    fout <<"\tsw\tt3," + std::to_string(frame_size - 20) + "(sp)\n";
    fout << "\taddi\ts0,sp," + std::to_string(frame_size) + "\n";
    // this->sentences.push_back("\tfmv.w.x\tft0,zero");

    paramSolve(func.ParameterList);

    int current_ID = 0;
    for (auto it : func.InstVec)
    {
        // 输出标签，用于goto跳转
        fout << "." << func.name << "_" << Instr_ID << ":\n";
        
        fout << "#" + std::to_string(Instr_ID++) + ": " + it->draw() + "\n";
        InstrSolve(it);
        current_ID++;
    }
    fout << "." << func.name << "_" << Instr_ID << ":\n";

    fout << func.name + "_return:\n";
    fout << "\tlw\tra," + std::to_string(frame_size - 4) + "(sp)\n";
    fout << "\tlw\tt0," + std::to_string(frame_size - 8) + "(sp)\n";
    fout <<"\tlw\tt1," + std::to_string(frame_size - 12) + "(sp)\n";
    fout <<"\tlw\tt2," + std::to_string(frame_size - 16) + "(sp)\n";
    fout <<"\tlw\tt3," + std::to_string(frame_size - 20) + "(sp)\n";
    fout << "\taddi\tsp,sp," + std::to_string(frame_size) + "\n";
    fout << "\tjr\tra\n";
    fout << "\t.size\t" + func.name + ",\t.-" + func.name + "\n";
}

rv::rvREG backend::Generator::getRd(ir::Operand rd, bool p)
{
    if (p)
    {
        if (rd.type == ir::Type::IntLiteral)
            fout << "\tli\t" << rv::toString(rv::rvREG::X7) << "," << rd.name << "\n";
        else
        {
            int des_offset = varMap.find_operand(rd);
            if (des_offset != -1)
            {
                // 本地变量，从栈中加载
                fout << "\tlw\t" << rv::toString(rv::rvREG::X7) << "," << des_offset << "(sp)\n";
            }
            else
            {
                fout << "\tlw\t" << rv::toString(rv::rvREG::X7) << "," << rd.name << "\n";
            }
        }
    }
    return rv::rvREG::X7; // t2
}

rv::rvREG backend::Generator::getRs1(ir::Operand rs1)
{
    // 对于字面量，直接加载到寄存器
    if (rs1.type == ir::Type::IntLiteral)
    {
        fout << "\tli\t" << rv::toString(rv::rvREG::X5) << "," << rs1.name << "\n";
        return rv::rvREG::X5; // t0
    }

    int op1_offset = varMap.find_operand(rs1);
    if (op1_offset != -1)
    {
        // 本地变量，从栈中加载
        fout << "\tlw\t" << rv::toString(rv::rvREG::X5) << "," << op1_offset << "(sp)\n";
    }
    else
    {
        fout << "\tlw\t" << rv::toString(rv::rvREG::X5) << "," << rs1.name << "\n";
    }
    return rv::rvREG::X5; // t0
}

rv::rvREG backend::Generator::getRs2(ir::Operand rs2)
{
    // 对于字面量，直接加载到寄存器
    if (rs2.type == ir::Type::IntLiteral)
    {
        fout << "\tli\t" << rv::toString(rv::rvREG::X6) << "," << rs2.name << "\n";
        return rv::rvREG::X6; // t1
    }

    int op2_offset = varMap.find_operand(rs2);
    if (op2_offset != -1)
    {
        // 本地变量，从栈中加载
        fout << "\tlw\t" << rv::toString(rv::rvREG::X6) << "," << op2_offset << "(sp)\n";
    }
    else
    {
        fout << "\tlw\t" << rv::toString(rv::rvREG::X6) << "," << rs2.name << "\n";
    }
    return rv::rvREG::X6; // t1
}

void backend::Generator::paramSolve(const std::vector<ir::Operand> &params)
{
    // 处理函数参数
    for (size_t i = 0; i < params.size(); i++)
    {
        // 将参数添加到变量映射表，假设每个参数占用4字节
        varMap.add_operand(params[i], 4);

        // 如果参数少于8个，则从a0-a7寄存器加载到内存
        // RISC-V调用约定中，前8个参数通过a0-a7寄存器传递
        if (i < 8)
        {
            int offset = varMap.find_operand(params[i]);
            // a0是X10，逐个对应后续的参数寄存器
            fout << "\tsw\tx" << (10 + i) << "," << offset << "(sp)\n";
        }
        // 超过8个的参数需要从调用者的栈帧中读取，这里简化处理
    }
}

void backend::Generator::InstrSolve(ir::Instruction *&instr)
{
    switch (instr->op)
    {
    case ::ir::Operator::alloc:
        AllocSolve(instr);
        break;
    case ir::Operator::_return:
        ReturnSolve(instr);
        break;
    case ir::Operator::call:
        CallSolve(instr);
        break;
    case ir::Operator::add:
        AddSolve(instr);
        break;
    case ir::Operator::sub:
        SubSolve(instr);
        break;
    case ir::Operator::mul:
        MulSolve(instr);
        break;
    case ir::Operator::div:
        DivSolve(instr);
        break;
    case ir::Operator::def:
        // 检查是否是全局变量定义
        if (current_func_name == "_global")
        {
            GlobalDefSolve(instr);
        }
        else
        {
            DefSolve(instr);
        }
        break;
    case ir::Operator::mov:
        MovSolve(instr);
        break;
    case ir::Operator::load:
        LoadSolve(instr);
        break;
    case ir::Operator::store:
        StoreSolve(instr);
        break;
    case ir::Operator::mod:
        RemSolve(instr);
        break;
    case ir::Operator::eq:
        EqSolve(instr);
        break;
    case ir::Operator::neq:
        NeqSolve(instr);
        break;
    case ir::Operator::lss:
        SltSolve(instr);
        break;
    case ir::Operator::gtr:
    {
        ir::Operand temp = instr->op1;
        instr->op2 = instr->op1;
        instr->op1 = temp;
        SltSolve(instr);
        break;
    }
    case ir::Operator::leq:
    {
        ir::Operand temp = instr->op1;
        instr->op2 = instr->op1;
        instr->op1 = temp;
        SltXSolve(instr);
        break;
    }
    case ir::Operator::geq:
        SltXSolve(instr);
        break;
    case ir::Operator::_or:
        OrSolve(instr);
        break;
    case ir::Operator::_and:
        AndSolve(instr);
        break;
    case ir::Operator::_goto:
        GotoSolve(instr);
        break;
    case ir::Operator::_not:
        SeqzSolve(instr);
    break;
    default:
        // 未实现的指令，输出警告
        fout << "\t# 未实现的指令: " << toString(instr->op) << "\n";
        break;
    }
}

void backend::Generator::SeqzSolve(ir::Instruction *&instr)
{
    rv::rvREG rd, rs1;
    rd = getRd(instr->des);
    rs1 = getRs1(instr->op1);
    int des_offset = varMap.find_operand(instr->des);

    if (des_offset == -1)
    {
        // 检查是否为全局变量
        if (global_vars.find(instr->des.name) != global_vars.end())
        {
            // 是全局变量，生成存储到全局变量的代码
            fout << "\tseqz\t" << rv::toString(rd) << "," << rv::toString(rs1) << "\n";
            fout << "\tla\ts3," << instr->des.name << "\n";
            fout << "\tsw\t" << rv::toString(rd) << ",0(s3)\n";
            return;
        }
        else
        {
            // 是局部变量，添加到变量表
            varMap.add_operand(instr->des);
            des_offset = varMap.find_operand(instr->des);
        }
    }

    fout << "\tseqz\t" << rv::toString(rd) << "," << rv::toString(rs1) << "\n";
    fout << "\tsw\t" << rv::toString(rd) << "," << des_offset << "(sp)\n";
}

void backend::Generator::CallSolve(ir::Instruction *&instr)
{
    const auto &callInst = dynamic_cast<const ir::CallInst &>(*instr);
    const auto &arguments = callInst.argumentList;

    // 处理参数传递：RISC-V调用约定前8个参数通过a0-a7寄存器传递
    for (size_t i = 0; i < arguments.size() && i < 8; ++i)
    {
        const auto &arg = arguments[i];

        if (arg.type == ir::Type::IntLiteral)
        {
            // 整数字面量直接加载到参数寄存器
            fout << "\tli\ta" << i << "," << arg.name << "\n";
        }
        else
        {
            // 查找参数在当前栈帧中的偏移量
            int arg_offset = varMap.find_operand(arg);
            if (arg_offset != -1)
            {
                // 对于数组参数，要计算并传递实际地址，而不只是偏移量
                if (arg.type == ir::Type::IntPtr || arg.type == ir::Type::FloatPtr)
                {
                    // 先加载数组基址偏移量
                    fout << "\tlw\tt3," << arg_offset << "(sp)\n";
                    // 计算实际地址：sp + 数组基址偏移量
                    fout << "\taddi\ta" << i << ",t3,520\n";
                }
                else
                {
                    // 普通变量，直接加载值
                    fout << "\tlw\ta" << i << "," << arg_offset << "(sp)\n";
                }
            }
            else
            {
                // 全局变量
                if (arg.type == ir::Type::IntPtr || arg.type == ir::Type::FloatPtr)
                {
                    // 对于全局数组，直接加载地址
                    fout << "\tla\ta" << i << "," << arg.name << "\n";
                }
                else
                {
                    // 对于全局普通变量，加载其值
                    fout << "\tlw\ta" << i << "," << arg.name << "\n";
                }
            }
        }
    }

    // 调用函数
    fout << "\tcall\t" + instr->op1.name + "\n";

    // 处理返回值
    if (instr->des.type != ir::Type::null)
    {
        int des_offset = varMap.find_operand(instr->des);

        if (des_offset == -1)
        {
            varMap.add_operand(instr->des);
            des_offset = varMap.find_operand(instr->des);
        }

        fout << "\tsw\ta0," << des_offset << "(sp)\n";
    }
}

void backend::Generator::ReturnSolve(ir::Instruction *&instr)
{
    if (instr->op1.type != ir::Type::null)
    {
        if (instr->op1.type == ir::Type::Int)
        {
            int op1_offset = varMap.find_operand(instr->op1);

            if (op1_offset != -1)
            {
                fout << "\tlw\ta0," << op1_offset << "(sp)\n";
            }
            else
            {
                fout << "\tlw\ta0," << instr->op1.name << "\n";
            }
        }
        else if (instr->op1.type == ir::Type::IntLiteral)
        {
            fout << "\tli\ta0," << instr->op1.name << "\n";
        }
    }

    // 跳转到函数返回处理
    fout << "\tj\t" << current_func_name << "_return\n";
}

void backend::Generator::AddSolve(ir::Instruction *&instr)
{
    rv::rvREG rd, rs1, rs2;
    rd = getRd(instr->des);
    rs1 = getRs1(instr->op1);
    rs2 = getRs2(instr->op2);

    int des_offset = varMap.find_operand(instr->des);

    if (des_offset == -1)
    {
        // 检查是否为全局变量
        if (global_vars.find(instr->des.name) != global_vars.end())
        {
            // 是全局变量，生成存储到全局变量的代码
            fout << "\tadd\t" << rv::toString(rd) << "," << rv::toString(rs1) << "," << rv::toString(rs2) << "\n";
            fout << "\tla\ts3," << instr->des.name << "\n";
            fout << "\tsw\t" << rv::toString(rd) << ",0(s3)\n";
            return;
        }
        else
        {
            // 是局部变量，添加到变量表
            varMap.add_operand(instr->des);
            des_offset = varMap.find_operand(instr->des);
        }
    }

    fout << "\tadd\t" << rv::toString(rd) << "," << rv::toString(rs1) << "," << rv::toString(rs2) << "\n";
    fout << "\tsw\t" << rv::toString(rd) << "," << des_offset << "(sp)\n";
}

void backend::Generator::SubSolve(ir::Instruction *&instr)
{
    rv::rvREG rd, rs1, rs2;
    rd = getRd(instr->des);
    rs1 = getRs1(instr->op1);
    rs2 = getRs2(instr->op2);

    int des_offset = varMap.find_operand(instr->des);

    if (des_offset == -1)
    {
        // 检查是否为全局变量
        if (global_vars.find(instr->des.name) != global_vars.end())
        {
            // 是全局变量，生成存储到全局变量的代码
            fout << "\tsub\t" << rv::toString(rd) << "," << rv::toString(rs1) << "," << rv::toString(rs2) << "\n";
            fout << "\tla\ts3," << instr->des.name << "\n";
            fout << "\tsw\t" << rv::toString(rd) << ",0(s3)\n";
            return;
        }
        else
        {
            // 是局部变量，添加到变量表
            varMap.add_operand(instr->des);
            des_offset = varMap.find_operand(instr->des);
        }
    }

    fout << "\tsub\t" << rv::toString(rd) << "," << rv::toString(rs1) << "," << rv::toString(rs2) << "\n";
    fout << "\tsw\t" << rv::toString(rd) << "," << des_offset << "(sp)\n";
}

void backend::Generator::MulSolve(ir::Instruction *&instr)
{
    rv::rvREG rd, rs1, rs2;
    rd = getRd(instr->des);
    rs1 = getRs1(instr->op1);
    rs2 = getRs2(instr->op2);

    int des_offset = varMap.find_operand(instr->des);

    if (des_offset == -1)
    {
        // 检查是否为全局变量
        if (global_vars.find(instr->des.name) != global_vars.end())
        {
            // 是全局变量，生成存储到全局变量的代码
            fout << "\tmul\t" << rv::toString(rd) << "," << rv::toString(rs1) << "," << rv::toString(rs2) << "\n";
            fout << "\tla\ts3," << instr->des.name << "\n";
            fout << "\tsw\t" << rv::toString(rd) << ",0(s3)\n";
            return;
        }
        else
        {
            // 是局部变量，添加到变量表
            varMap.add_operand(instr->des);
            des_offset = varMap.find_operand(instr->des);
        }
    }

    fout << "\tmul\t" << rv::toString(rd) << "," << rv::toString(rs1) << "," << rv::toString(rs2) << "\n";
    fout << "\tsw\t" << rv::toString(rd) << "," << des_offset << "(sp)\n";
}

void backend::Generator::DivSolve(ir::Instruction *&instr)
{
    rv::rvREG rd, rs1, rs2;
    rd = getRd(instr->des);
    rs1 = getRs1(instr->op1);
    rs2 = getRs2(instr->op2);

    int des_offset = varMap.find_operand(instr->des);

    if (des_offset == -1)
    {
        // 检查是否为全局变量
        if (global_vars.find(instr->des.name) != global_vars.end())
        {
            // 是全局变量，生成存储到全局变量的代码
            fout << "\tdiv\t" << rv::toString(rd) << "," << rv::toString(rs1) << "," << rv::toString(rs2) << "\n";
            fout << "\tla\ts3," << instr->des.name << "\n";
            fout << "\tsw\t" << rv::toString(rd) << ",0(s3)\n";
            return;
        }
        else
        {
            // 是局部变量，添加到变量表
            varMap.add_operand(instr->des);
            des_offset = varMap.find_operand(instr->des);
        }
    }

    fout << "\tdiv\t" << rv::toString(rd) << "," << rv::toString(rs1) << "," << rv::toString(rs2) << "\n";
    fout << "\tsw\t" << rv::toString(rd) << "," << des_offset << "(sp)\n";
}

void backend::Generator::RemSolve(ir::Instruction *&instr)
{
    rv::rvREG rd, rs1, rs2;
    rd = getRd(instr->des);
    rs1 = getRs1(instr->op1);
    rs2 = getRs2(instr->op2);
    int des_offset = varMap.find_operand(instr->des);

    if (des_offset == -1)
    {
        // 检查是否为全局变量
        if (global_vars.find(instr->des.name) != global_vars.end())
        {
            // 是全局变量，生成存储到全局变量的代码
            fout << "\trem\t" << rv::toString(rd) << "," << rv::toString(rs1) << "," << rv::toString(rs2) << "\n";
            fout << "\tla\ts3," << instr->des.name << "\n";
            fout << "\tsw\t" << rv::toString(rd) << ",0(s3)\n";
            return;
        }
        else
        {
            // 是局部变量，添加到变量表
            varMap.add_operand(instr->des);
            des_offset = varMap.find_operand(instr->des);
        }
    }

    fout << "\trem\t" << rv::toString(rd) << "," << rv::toString(rs1) << "," << rv::toString(rs2) << "\n";
    fout << "\tsw\t" << rv::toString(rd) << "," << des_offset << "(sp)\n";
}

void backend::Generator::EqSolve(ir::Instruction *&instr)
{
    rv::rvREG rd, rs1, rs2;
    rd = getRd(instr->des);
    rs1 = getRs1(instr->op1);
    rs2 = getRs2(instr->op2);

    int des_offset = varMap.find_operand(instr->des);

    if (des_offset == -1)
    {
        // 检查是否为全局变量
        if (global_vars.find(instr->des.name) != global_vars.end())
        {
            // 是全局变量，生成存储到全局变量的代码
            fout << "\tsub\t" << rv::toString(rd) << "," << rv::toString(rs1) << "," << rv::toString(rs2) << "\n";
            fout << "\tsltiu\t" << rv::toString(rd) << "," << rv::toString(rd) << "," << "1\n";
            fout << "\tla\ts3," << instr->des.name << "\n";
            fout << "\tsw\t" << rv::toString(rd) << ",0(s3)\n";
            return;
        }
        else
        {
            // 是局部变量，添加到变量表
            varMap.add_operand(instr->des);
            des_offset = varMap.find_operand(instr->des);
        }
    }
    fout << "\tsub\t" << rv::toString(rd) << "," << rv::toString(rs1) << "," << rv::toString(rs2) << "\n";
    fout << "\tsltiu\t" << rv::toString(rd) << "," << rv::toString(rd) << "," << "1\n";
    fout << "\tsw\t" << rv::toString(rd) << "," << des_offset << "(sp)\n";
}

void backend::Generator::NeqSolve(ir::Instruction *&instr)
{
    rv::rvREG rd, rs1, rs2;
    rd = getRd(instr->des);
    rs1 = getRs1(instr->op1);
    rs2 = getRs2(instr->op2);

    int des_offset = varMap.find_operand(instr->des);

    if (des_offset == -1)
    {
        // 检查是否为全局变量
        if (global_vars.find(instr->des.name) != global_vars.end())
        {
            // 是全局变量，生成存储到全局变量的代码
            fout << "\tsub\t" << rv::toString(rd) << "," << rv::toString(rs1) << "," << rv::toString(rs2) << "\n";
            fout << "\tsltu\t" << rv::toString(rd) << "," << "zero," << rv::toString(rd) << "\n";
            fout << "\tla\ts3," << instr->des.name << "\n";
            fout << "\tsw\t" << rv::toString(rd) << ",0(s3)\n";
            return;
        }
        else
        {
            // 是局部变量，添加到变量表
            varMap.add_operand(instr->des);
            des_offset = varMap.find_operand(instr->des);
        }
    }
    fout << "\tsub\t" << rv::toString(rd) << "," << rv::toString(rs1) << "," << rv::toString(rs2) << "\n";
    fout << "\tsltu\t" << rv::toString(rd) << "," << "zero," << rv::toString(rd) << "\n";
    fout << "\tsw\t" << rv::toString(rd) << "," << des_offset << "(sp)\n";
}

void backend::Generator::SltSolve(ir::Instruction *&instr)
{
    rv::rvREG rd, rs1, rs2;
    rd = getRd(instr->des);
    rs1 = getRs1(instr->op1);
    rs2 = getRs2(instr->op2);

    int des_offset = varMap.find_operand(instr->des);

    if (des_offset == -1)
    {
        // 检查是否为全局变量
        if (global_vars.find(instr->des.name) != global_vars.end())
        {
            // 是全局变量，生成存储到全局变量的代码
            fout << "\tslt\t" << rv::toString(rd) << "," << rv::toString(rs1) << "," << rv::toString(rs2) << "\n";
            fout << "\tla\ts3," << instr->des.name << "\n";
            fout << "\tsw\t" << rv::toString(rd) << ",0(s3)\n";
            return;
        }
        else
        {
            // 是局部变量，添加到变量表
            varMap.add_operand(instr->des);
            des_offset = varMap.find_operand(instr->des);
        }
    }

    fout << "\tslt\t" << rv::toString(rd) << "," << rv::toString(rs1) << "," << rv::toString(rs2) << "\n";
    fout << "\tsw\t" << rv::toString(rd) << "," << des_offset << "(sp)\n";
}

void backend::Generator::SltXSolve(ir::Instruction *&instr)
{
    rv::rvREG rd, rs1, rs2;
    rd = getRd(instr->des);
    rs1 = getRs1(instr->op1);
    rs2 = getRs2(instr->op2);

    int des_offset = varMap.find_operand(instr->des);

    if (des_offset == -1)
    {
        // 检查是否为全局变量
        if (global_vars.find(instr->des.name) != global_vars.end())
        {
            // 是全局变量，生成存储到全局变量的代码
            fout << "\tslt\t" << rv::toString(rd) << "," << rv::toString(rs1) << "," << rv::toString(rs2) << "\n";
            fout << "\txori\t" << rv::toString(rd) << "," << rv::toString(rd) << "," << "1\n";
            fout << "\tla\ts3," << instr->des.name << "\n";
            fout << "\tsw\t" << rv::toString(rd) << ",0(s3)\n";
            return;
        }
        else
        {
            // 是局部变量，添加到变量表
            varMap.add_operand(instr->des);
            des_offset = varMap.find_operand(instr->des);
        }
    }

    fout << "\tslt\t" << rv::toString(rd) << "," << rv::toString(rs1) << "," << rv::toString(rs2) << "\n";
    fout << "\txori\t" << rv::toString(rd) << "," << rv::toString(rd) << "," << "1\n";
    fout << "\tsw\t" << rv::toString(rd) << "," << des_offset << "(sp)\n";
}

void backend::Generator::AndSolve(ir::Instruction *&instr)
{
    rv::rvREG rd, rs1, rs2;
    rd = getRd(instr->des);
    rs1 = getRs1(instr->op1);
    rs2 = getRs2(instr->op2);

    int des_offset = varMap.find_operand(instr->des);

    if (des_offset == -1)
    {
        // 检查是否为全局变量
        if (global_vars.find(instr->des.name) != global_vars.end())
        {
            // 是全局变量，生成存储到全局变量的代码
            fout << "\tand\t" << rv::toString(rd) << "," << rv::toString(rs1) << "," << rv::toString(rs2) << "\n";
            fout << "\tla\ts3," << instr->des.name << "\n";
            fout << "\tsw\t" << rv::toString(rd) << ",0(s3)\n";
            return;
        }
        else
        {
            // 是局部变量，添加到变量表
            varMap.add_operand(instr->des);
            des_offset = varMap.find_operand(instr->des);
        }
    }

    fout << "\tand\t" << rv::toString(rd) << "," << rv::toString(rs1) << "," << rv::toString(rs2) << "\n";
    fout << "\tsw\t" << rv::toString(rd) << "," << des_offset << "(sp)\n";
}

void backend::Generator::OrSolve(ir::Instruction *&instr)
{
    rv::rvREG rd, rs1, rs2;
    rd = getRd(instr->des);
    rs1 = getRs1(instr->op1);
    rs2 = getRs2(instr->op2);

    int des_offset = varMap.find_operand(instr->des);

    if (des_offset == -1)
    {
        // 检查是否为全局变量
        if (global_vars.find(instr->des.name) != global_vars.end())
        {
            // 是全局变量，生成存储到全局变量的代码
            fout << "\tor\t" << rv::toString(rd) << "," << rv::toString(rs1) << "," << rv::toString(rs2) << "\n";
            fout << "\tla\ts3," << instr->des.name << "\n";
            fout << "\tsw\t" << rv::toString(rd) << ",0(s3)\n";
            return;
        }
        else
        {
            // 是局部变量，添加到变量表
            varMap.add_operand(instr->des);
            des_offset = varMap.find_operand(instr->des);
        }
    }

    fout << "\tor\t" << rv::toString(rd) << "," << rv::toString(rs1) << "," << rv::toString(rs2) << "\n";
    fout << "\tsw\t" << rv::toString(rd) << "," << des_offset << "(sp)\n";
}

void backend::Generator::MovSolve(ir::Instruction *&instr)
{
    rv::rvREG rs1;
    rs1 = getRs1(instr->op1);

    int des_offset = varMap.find_operand(instr->des);

    // 如果目标变量不在变量表中，将其添加到变量表
    if (des_offset == -1)
    {
        // 检查是否为全局变量
        if (global_vars.find(instr->des.name) != global_vars.end())
        {
            // 是全局变量，生成存储到全局变量的代码
            fout << "\tla\ts3," << instr->des.name << "\n";
            fout << "\tsw\t" << rv::toString(rs1) << ",0(s3)\n";
            return;
        }
        else
        {
            // 是局部变量，添加到变量表
            varMap.add_operand(instr->des);
            des_offset = varMap.find_operand(instr->des);
        }
    }

    // 存储结果到内存
    fout << "\tsw\t" << rv::toString(rs1) << "," << des_offset << "(sp)\n";
}

void backend::Generator::LoadSolve(ir::Instruction *&instr)
{
    rv::rvREG rd, rs2;
    rd = getRd(instr->des);
    
    // 获取索引偏移量
    rs2 = getRs2(instr->op2);
    
    int des_offset = varMap.find_operand(instr->des);
    
    // 如果目标变量不在变量表中，将其添加到变量表
    if (des_offset == -1)
    {
        varMap.add_operand(instr->des);
        des_offset = varMap.find_operand(instr->des);
    }
    
    int array_var_offset = varMap.find_operand(instr->op1);
    if (array_var_offset != -1)
    {
        // 先读取数组基地址偏移量
        fout << "\tlw\tt4," << array_var_offset << "(sp)\n";
        // 计算元素地址：基地址偏移量 + 索引*4
        fout << "\tslli\tt3," << rv::toString(rs2) << ",2\n";
        fout << "\tadd\tt3,t4,t3\n";
        // 加上sp得到最终地址
        fout << "\tadd\tt3,sp,t3\n";
    }
    else
    {
        // 全局数组
        rv::rvREG rs1 = rv::rvREG::X5;
        fout << "\tla\t" << rv::toString(rv::rvREG::X5) << "," << instr->op1.name << "\n";
        fout << "\tslli\tt3," << rv::toString(rs2) << ",2\n";
        fout << "\tadd\tt3," << rv::toString(rs1) << ",t3\n";
    }
    
    // 从计算出的地址加载值
    fout << "\tlw\t" << rv::toString(rd) << ",0(t3)\n";
    // 存储到目标变量
    fout << "\tsw\t" << rv::toString(rd) << "," << des_offset << "(sp)\n";
}

void backend::Generator::StoreSolve(ir::Instruction *&instr)
{
    rv::rvREG rd, rs2;
    rd = getRd(instr->des, true); // 要存储的值
    rs2 = getRs2(instr->op2);     // 索引
    
    // 获取数组变量偏移量
    int array_var_offset = varMap.find_operand(instr->op1);
    if (array_var_offset != -1)
    {
        // 先读取数组基地址偏移量
        fout << "\tlw\tt4," << array_var_offset << "(sp)\n";
        // 计算元素偏移：索引*4
        fout << "\tslli\tt3," << rv::toString(rs2) << ",2\n";
        // 计算最终偏移量：基地址偏移量 + 索引*4
        fout << "\tadd\tt3,t4,t3\n";
        // 加上sp得到最终地址
        fout << "\tadd\tt3,sp,t3\n";
    }
    else
    {
        // 全局数组处理
        fout << "\tla\t" << rv::toString(rv::rvREG::X5) << "," << instr->op1.name << "\n";
        fout << "\tslli\tt3," << rv::toString(rs2) << ",2\n";
        fout << "\tadd\tt3," << rv::toString(rv::rvREG::X5) << ",t3\n";
    }
    
    // 存储值到计算出的地址
    fout << "\tsw\t" << rv::toString(rd) << ",0(t3)\n";
}

void backend::Generator::DefSolve(ir::Instruction *&instr)
{
    rv::rvREG rd;
    rd = getRd(instr->des);

    int des_offset = varMap.find_operand(instr->des);

    // 如果目标变量不在变量表中，将其添加到变量表
    if (des_offset == -1)
    {
        // 检查是否为全局变量
        if (global_vars.find(instr->des.name) != global_vars.end())
        {
            // 是全局变量，生成存储到全局变量的代码
            if (instr->op1.type == ir::Type::IntLiteral)
            {
                fout << "\tli\t" << rv::toString(rd) << "," << instr->op1.name << "\n";
            }
            else
            {
                rd = getRs1(instr->op1);
            }
            fout << "\tla\ts3," << instr->des.name << "\n";
            fout << "\tsw\t" << rv::toString(rd) << ",0(s3)\n";
            return;
        }
        else
        {
            // 是局部变量，添加到变量表
            varMap.add_operand(instr->des);
            des_offset = varMap.find_operand(instr->des);
        }
    }

    if (instr->op1.type == ir::Type::IntLiteral)
    {
        fout << "\tli\t" << rv::toString(rd) << "," << instr->op1.name << "\n";
    }
    else
    {
        rd = getRs1(instr->op1);
    }

    fout << "\tsw\t" << rv::toString(rd) << "," << des_offset << "(sp)\n";
}

void backend::Generator::GlobalDefSolve(ir::Instruction *&instr)
{
    // 解析出全局变量名（去掉_global后缀）
    std::string global_var_name = instr->des.name;

    fout << "\tli\ts2," << instr->op1.name << "\n";
    fout << "\tmv\ts1,s2\n";
    fout << "\tla\ts3," << global_var_name << "\n";
    fout << "\tsw\ts1,0(s3)\n";

    global_vars.insert(global_var_name);
}

void backend::Generator::AllocSolve(ir::Instruction *&instr)
{
    std::string var_name = instr->des.name;
    int array_size = 0;

    if (instr->op1.type == ir::Type::IntLiteral)
        array_size = std::stoi(instr->op1.name);

    if (current_func_name != "_global")
    {
        // 先为数组名变量分配4字节空间，用于存储数组基地址偏移量
        varMap.add_operand(instr->des, 4);
        int array_base_offset = varMap.find_operand(instr->des);
        
        // 为数组元素分配空间
        int array_elements_offset = varMap.offset - 4;  // 当前偏移量减4作为数组起始位置
        varMap.offset -= (array_size * 4);  // 为数组元素预留空间
        
        // 将数组基地址偏移量存入变量位置
        fout << "\tli\tt2," << array_elements_offset << "\n";
        fout << "\tsw\tt2," << array_base_offset << "(sp)\n";
    }
}

void backend::Generator::GotoSolve(ir::Instruction *&instr)
{
    // 计算跳转目标的标签（基于当前指令的相对偏移）
    int relative_offset = std::stoi(instr->des.name);
    
    if (instr->op1.type == ir::Type::null)
    {
        // 无条件跳转
        fout << "\tj\t." << current_func_name << "_" << (Instr_ID + relative_offset - 1) << "\n";
    }
    else
    {
        // 条件跳转，先加载条件值到寄存器
        rv::rvREG rs1 = getRs1(instr->op1);
        
        // 当条件不为0时跳转
        fout << "\tbnez\t" << rv::toString(rs1) << ",." << current_func_name << "_" << (Instr_ID + relative_offset - 1) << "\n";
    }
}

