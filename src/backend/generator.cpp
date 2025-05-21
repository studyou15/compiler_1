#include "backend/generator.h"

#include <assert.h>

#define TODO assert(0 && "todo")

int frame_size = 1560;
int Instr_ID = 0;

backend::Generator::Generator(ir::Program &p, std::ofstream &f) : program(p), fout(f), global_vars() {}

// 实现获取浮点常量标签的方法
std::string backend::Generator::getFloatConstantLabel(const std::string &value)
{
    // 检查该浮点常量是否已经有标签
    if (float_constants.find(value) != float_constants.end())
    {
        return float_constants[value];
    }

    // 为新的浮点常量创建标签
    std::string label = "Floatnum" + std::to_string(float_counter++);
    float_constants[value] = label;
    return label;
}

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

    // 先扫描整个程序收集所有浮点常量
    // 这需要遍历所有函数的所有指令
    for (auto &func : program.functions)
    {
        if (func.name == "_global")
            continue;
        for (auto &instr : func.InstVec)
        {
            // 检查指令中的操作数是否包含浮点常量
            if (instr->op1.type == ir::Type::FloatLiteral)
            {
                getFloatConstantLabel(instr->op1.name);
            }
            if (instr->op2.type == ir::Type::FloatLiteral)
            {
                getFloatConstantLabel(instr->op2.name);
            }
            if (instr->des.type == ir::Type::FloatLiteral)
            {
                getFloatConstantLabel(instr->des.name);
            }

            // 检查指令中是否有函数调用
            if (instr->op == ir::Operator::call)
            {
                const auto &callInst = dynamic_cast<const ir::CallInst &>(*instr);
                // 检查函数参数中是否有浮点常量
                for (const auto &arg : callInst.argumentList)
                {
                    if (arg.type == ir::Type::FloatLiteral)
                    {
                        getFloatConstantLabel(arg.name);
                    }
                }
            }
        }
    }

    // 输出所有收集到的浮点常量
    for (const auto &pair : float_constants)
    {
        fout << pair.second << ":\n";
        fout << "\t.float " << pair.first << "\n";
    }

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
        if (it.val.type == ir::Type::FloatLiteral)
            continue;
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
    fout << "\tsw\ts0," + std::to_string(frame_size - 8) + "(sp)\n";
    fout << "\tsw\tt1," + std::to_string(frame_size - 12) + "(sp)\n";
    fout << "\tsw\tt2," + std::to_string(frame_size - 16) + "(sp)\n";
    fout << "\tsw\tt3," + std::to_string(frame_size - 20) + "(sp)\n";
    // fout << "\tsw\tt4," + std::to_string(frame_size - 24) + "(sp)\n";
    // fout << "\tsw\tt5," + std::to_string(frame_size - 28) + "(sp)\n";
    // fout << "\tsw\tt6," + std::to_string(frame_size - 32) + "(sp)\n";
    fout << "\tfmv.w.x\tft0,zero\n";
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
    fout << "\tlw\ts0," + std::to_string(frame_size - 8) + "(sp)\n";
    fout << "\tlw\tt1," + std::to_string(frame_size - 12) + "(sp)\n";
    fout << "\tlw\tt2," + std::to_string(frame_size - 16) + "(sp)\n";
    fout << "\tlw\tt3," + std::to_string(frame_size - 20) + "(sp)\n";
    // fout << "\tlw\tt4," + std::to_string(frame_size - 24) + "(sp)\n";
    // fout << "\tlw\tt5," + std::to_string(frame_size - 28) + "(sp)\n";
    // fout << "\tlw\tt6," + std::to_string(frame_size - 32) + "(sp)\n";
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
                fout << "\tlw\t" << rv::toString(rv::rvREG::X7) << "," << des_offset << "(s0)\n";
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
    if (rs1.type == ir::Type::IntLiteral || (rs1.name[0]>='0'&&rs1.name[0]<='9'))
    {
        fout << "\tli\t" << rv::toString(rv::rvREG::X5) << "," << rs1.name << "\n";
        return rv::rvREG::X5; // t0
    }

    int op1_offset = varMap.find_operand(rs1);
    if (op1_offset != -1)
    {
        // 本地变量，从栈中加载
        fout << "\tlw\t" << rv::toString(rv::rvREG::X5) << "," << op1_offset << "(s0)\n";
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
        fout << "\tlw\t" << rv::toString(rv::rvREG::X6) << "," << op2_offset << "(s0)\n";
    }
    else
    {
        fout << "\tlw\t" << rv::toString(rv::rvREG::X6) << "," << rs2.name << "\n";
    }
    return rv::rvREG::X6; // t1
}

rv::rvFREG backend::Generator::fgetRd(ir::Operand rd, bool p)
{
    if (p)
    {
        if(rd.type == ir::Type::FloatLiteral)
        {
            std::string label = getFloatConstantLabel(rd.name);
            fout << "\tla\t" << "t4," << label << "\n";
            fout << "\tflw\t" << rv::toString(rv::rvFREG::F7) << ",0(t4)" << "\n";
        }
        else
        {
            int des_offset = varMap.find_operand(rd);
            fout << "\tflw\t" << rv::toString(rv::rvFREG::F7) << "," << des_offset << "(s0)\n";
        }
    }
    return rv::rvFREG::F7; // ft7
}

rv::rvFREG backend::Generator::fgetRs1(ir::Operand rs1)
{
    // 对于字面量，加载对应的标签
    if (rs1.type == ir::Type::FloatLiteral)
    {
        std::string label = getFloatConstantLabel(rs1.name);
        fout << "\tla\t" << "t4," << label << "\n";
        fout << "\tflw\t" << rv::toString(rv::rvFREG::F5) << ",0(t4)" << "\n";
        return rv::rvFREG::F5; // ft5
    }

    int op1_offset = varMap.find_operand(rs1);
    if (op1_offset != -1)
    {
        // 本地变量，从栈中加载
        fout << "\tflw\t" << rv::toString(rv::rvFREG::F5) << "," << op1_offset << "(s0)\n";
    }
    else
    {
        fout << "\tflw\t" << rv::toString(rv::rvFREG::F5) << "," << rs1.name << "\n";
    }
    return rv::rvFREG::F5; // ft5
}

rv::rvFREG backend::Generator::fgetRs2(ir::Operand rs2)
{
    // 对于字面量，直接加载到浮点寄存器
    if (rs2.type == ir::Type::FloatLiteral)
    {
        std::string label = getFloatConstantLabel(rs2.name);
        fout << "\tla\t" << "t4," << label << "\n";
        fout << "\tflw\t" << rv::toString(rv::rvFREG::F6) << ",0(t4)" << "\n";
        return rv::rvFREG::F6; // ft6
    }

    int op2_offset = varMap.find_operand(rs2);
    if (op2_offset != -1)
    {
        // 本地变量，从栈中加载
        fout << "\tflw\t" << rv::toString(rv::rvFREG::F6) << "," << op2_offset << "(s0)\n";
    }
    else
    {
        fout << "\tflw\t" << rv::toString(rv::rvFREG::F6) << "," << rs2.name << "\n";
    }
    return rv::rvFREG::F6; // ft6
}

void backend::Generator::paramSolve(const std::vector<ir::Operand> &params)
{
    // 处理函数参数
    for (size_t i = 0; i < params.size(); i++)
    {
        if (params[i].type == ir::Type::IntPtr && global_vars.find(params[i].name) != global_vars.end())
        {
            continue;
        }
        // 将参数添加到变量映射表，假设每个参数占用4字节
        varMap.add_operand(params[i], 4);
        int offset = varMap.find_operand(params[i]);
        // 如果参数少于8个，则从a0-a7寄存器加载到内存
        // RISC-V调用约定中，前8个参数通过a0-a7寄存器传递
        if (i < 8)
        {
            if (params[i].type == ir::Type::Float)
            {
                fout << "\tfsw\tfa" << i << "," << offset << "(s0)\n";
            }
            // a0是X10，逐个对应后续的参数寄存器
            else
            {
                fout << "\tsw\ta" << i << "," << offset << "(s0)\n";
            }
        }
        else
        {
            fout << "\tlw\tt0" << "," << i * 4 - 32 << "(s0)\n";
            fout << "\tsw\tt0" << "," << offset << "(s0)\n";
        }
        // 超过8个的参数需要从调用者的栈帧中读取，这里简化处理
    }
}

void backend::Generator::InstrSolve(ir::Instruction *&instr)
{
    if (current_func_name == "_global")
    {
        if (instr->op == ir::Operator::fmov || instr->op == ir::Operator::fdef || instr->op == ir::Operator::fmul || instr->op == ir::Operator::fsub)
            return;
    }
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
        instr->op1 = instr->op2;
        instr->op2 = temp;
        SltSolve(instr);
        break;
    }
    case ir::Operator::leq:
    {
        ir::Operand temp = instr->op1;
        instr->op1 = instr->op2;
        instr->op2 = temp;
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
    case ir::Operator::fadd:
        FAddSolve(instr);
        break;
    case ir::Operator::fsub:
        FSubSolve(instr);
        break;
    case ir::Operator::fmul:
        FMulSolve(instr);
        break;
    case ir::Operator::fdiv:
        FDivSolve(instr);
        break;
    case ir::Operator::fmov:
        FMovSolve(instr);
        break;
    case ir::Operator::cvt_i2f:
        CvtI2FSolve(instr);
        break;
    case ir::Operator::cvt_f2i:
        CvtF2ISolve(instr);
        break;
    case ir::Operator::flss:
        FLssSolve(instr);
        break;
    case ir::Operator::fdef:
        FDefSolve(instr);
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
    fout << "\tsw\t" << rv::toString(rd) << "," << des_offset << "(s0)\n";
}

void backend::Generator::CallSolve(ir::Instruction *&instr)
{
    const auto &callInst = dynamic_cast<const ir::CallInst &>(*instr);
    const auto &arguments = callInst.argumentList;

    // 处理参数传递：RISC-V调用约定前8个参数通过a0-a7寄存器传递
    for (size_t i = 0; i < arguments.size(); ++i)
    {
        const auto &arg = arguments[i];

        int arg_offset = varMap.find_operand(arg);

        if (arg.type == ir::Type::FloatLiteral)
        {
            std::string label = getFloatConstantLabel(arg.name);
            fout << "\tla\t" << "t4," << label << "\n";
            fout << "\tflw\tfa" << i << ",0(t4)" << "\n";
            continue;
        }
        else if (arg.type == ir::Type::Float)
        {
            fout << "\tflw\tfa" << i << "," << arg_offset << "(s0)\n";
            continue;
        }
        else if(arg.type == ir::Type::FloatPtr)
        {
            fout << "\tlw\tt4," << arg_offset << "(s0)\n";
            fout << "\tadd\ta"<<i<<",t4,s0\n";
            continue;
        }
        if (i < 8)
        {
            if (arg.type == ir::Type::IntLiteral)
            {
                // 整数字面量直接加载到参数寄存器
                fout << "\tli\ta" << i << "," << arg.name << "\n";
            }
            else
            {
                // 查找参数在当前栈帧中的偏移量
                if (arg_offset != -1)
                {
                    // 对于数组参数，要计算并传递实际地址，而不只是偏移量
                    if (arg.type == ir::Type::IntPtr || arg.type == ir::Type::FloatPtr)
                    {
                        // 先加载数组基址偏移量
                        fout << "\tlw\tt3," << arg_offset << "(s0)\n";
                        // 计算实际地址：sp + 数组基址偏移量
                        fout << "\taddi\ta" << i << ",t3," << frame_size << "\n";
                    }
                    else
                    {
                        // 普通变量，直接加载值
                        fout << "\tlw\ta" << i << "," << arg_offset << "(s0)\n";
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
        else
        {
            rv::rvREG rs1 = getRs1(arguments[i]);
            fout << "\tsw\t" << rv::toString(rs1) << "," << 4 * i - 32 << "(sp)\n";
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
        if (instr->des.type == ir::Type::Int)
            fout << "\tsw\ta0," << des_offset << "(s0)\n";
        else
            fout << "\tfsw\tfa0," << des_offset << "(s0)\n";
    }
}

void backend::Generator::ReturnSolve(ir::Instruction *&instr)
{
    if (instr->op1.type != ir::Type::null)
    {
        int op1_offset = varMap.find_operand(instr->op1);

        if (instr->op1.type == ir::Type::Int)
        {
            if (op1_offset != -1)
            {
                fout << "\tlw\ta0," << op1_offset << "(s0)\n";
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
        else
            fout << "\tflw\tfa0," << op1_offset << "(s0)\n";
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
    fout << "\tsw\t" << rv::toString(rd) << "," << des_offset << "(s0)\n";
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
    fout << "\tsw\t" << rv::toString(rd) << "," << des_offset << "(s0)\n";
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
    fout << "\tsw\t" << rv::toString(rd) << "," << des_offset << "(s0)\n";
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
    fout << "\tsw\t" << rv::toString(rd) << "," << des_offset << "(s0)\n";
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
    fout << "\tsw\t" << rv::toString(rd) << "," << des_offset << "(s0)\n";
}

void backend::Generator::EqSolve(ir::Instruction *&instr)
{
    if (instr->op1.type == ir::Type::FloatLiteral || instr->op2.type == ir::Type::FloatLiteral || instr->op1.type == ir::Type::Float)
        return;
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
    fout << "\tsw\t" << rv::toString(rd) << "," << des_offset << "(s0)\n";
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
    fout << "\tsw\t" << rv::toString(rd) << "," << des_offset << "(s0)\n";
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
    fout << "\tsw\t" << rv::toString(rd) << "," << des_offset << "(s0)\n";
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
    fout << "\tsw\t" << rv::toString(rd) << "," << des_offset << "(s0)\n";
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
            fout << "\tsnez\t" << rv::toString(rs1) << "," << rv::toString(rs1) << "\n";
            fout << "\tsnez\t" << rv::toString(rs2) << "," << rv::toString(rs2) << "\n";
            fout << "\tand\t" << rv::toString(rd) << "," << rv::toString(rs1) << "," << rv::toString(rs2) << "\n";
            fout << "\tla\ts3," << instr->des.name << "\n";
            fout << "\tsw\t" << rv::toString(rd) << ",0(s3)\n";
            return;
        }
    }
    fout << "\tsnez\t" << rv::toString(rs1) << "," << rv::toString(rs1) << "\n";
    fout << "\tsnez\t" << rv::toString(rs2) << "," << rv::toString(rs2) << "\n";
    fout << "\tand\t" << rv::toString(rd) << "," << rv::toString(rs1) << "," << rv::toString(rs2) << "\n";
    fout << "\tsw\t" << rv::toString(rd) << "," << des_offset << "(s0)\n";
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
    }

    fout << "\tor\t" << rv::toString(rd) << "," << rv::toString(rs1) << "," << rv::toString(rs2) << "\n";
    fout << "\tsw\t" << rv::toString(rd) << "," << des_offset << "(s0)\n";
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
    fout << "\tsw\t" << rv::toString(rs1) << "," << des_offset << "(s0)\n";
}

void backend::Generator::LoadSolve(ir::Instruction *&instr)
{
    rv::rvREG rd, rs2;
    rd = getRd(instr->des);

    // 获取索引偏移量 (这里的rs2已经是计算好的线性索引)
    rs2 = getRs2(instr->op2);

    int des_offset = varMap.find_operand(instr->des);

    if (des_offset == -1)
    {
        varMap.add_operand(instr->des);
        des_offset = varMap.find_operand(instr->des);
    }

    int array_var_offset = varMap.find_operand(instr->op1);
    if (array_var_offset != -1)
    {
        // 先读取数组基地址偏移量
        fout << "\tlw\tt4," << array_var_offset << "(s0)\n";
        // 计算元素地址：基地址偏移量 + 索引*4
        fout << "\tslli\tt3," << rv::toString(rs2) << ",2\n";
        fout << "\tadd\tt3,t4,t3\n";
        // 加上sp得到最终地址
        fout << "\tadd\tt3,s0,t3\n";
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
    fout << "\tsw\t" << rv::toString(rd) << "," << des_offset << "(s0)\n";
}

void backend::Generator::StoreSolve(ir::Instruction *&instr)
{
    if (instr->des.type != ir::Type::Float && instr->des.type != ir::Type::FloatLiteral)
    {
        rv::rvREG rd, rs2;
        rd = getRd(instr->des, true); // 要存储的值
        rs2 = getRs2(instr->op2);     // 这是计算后的线性索引

        // 获取数组变量偏移量
        int array_var_offset = varMap.find_operand(instr->op1);
        if (array_var_offset != -1)
        {
            // 先读取数组基地址偏移量
            fout << "\tlw\tt4," << array_var_offset << "(s0)\n";
            // 计算元素偏移：索引*4
            fout << "\tslli\tt3," << rv::toString(rs2) << ",2\n";
            // 计算最终偏移量：基地址偏移量 + 索引*4
            fout << "\tadd\tt3,t4,t3\n";
            // 加上sp得到最终地址
            fout << "\tadd\tt3,s0,t3\n";
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
    else
    {
        rv::rvFREG frd;
        rv::rvREG rs2;
        frd = fgetRd(instr->des, true);
        rs2 = getRs2(instr->op2);
        int array_var_offset = varMap.find_operand(instr->op1);

        fout << "\tlw\tt4," << array_var_offset << "(s0)\n";
        fout << "\tslli\tt3," << rv::toString(rs2) << ",2\n";
        fout << "\tadd\tt3,t4,t3\n";
        fout << "\tadd\tt3,s0,t3\n";
        fout << "\tfsw\t" << rv::toString(frd) << ",0(t3)\n";
    }
}

void backend::Generator::FDefSolve(ir::Instruction *&instr)
{

    int des_offset = varMap.find_operand(instr->des);
    varMap.add_operand(instr->des);
    des_offset = varMap.find_operand(instr->des);

    rv::rvFREG frs1 = fgetRs1(instr->op1);
    fout << "\tfsw\t" << rv::toString(frs1) << "," << des_offset << "(s0)\n";
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

    fout << "\tsw\t" << rv::toString(rd) << "," << des_offset << "(s0)\n";
}

void backend::Generator::GlobalDefSolve(ir::Instruction *&instr)
{
    // 解析出全局变量名（去掉_global后缀）
    std::string global_var_name = instr->des.name;

    if (instr->op1.type == ir::Type::IntLiteral)
        fout << "\tli\ts2," << instr->op1.name << "\n";
    else
    {
        int des_offset = varMap.find_operand(instr->op1);
        fout << "\tlw\ts2," << des_offset << "(s0)\n";
    }
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
        int array_elements_offset = varMap.offset - (array_size * 4) + 4; // 当前偏移量减4作为数组起始位置
        varMap.offset -= (array_size * 4);                                // 为数组元素预留空间

        // 将数组基地址偏移量存入变量位置
        fout << "\tli\tt2," << array_elements_offset << "\n";
        fout << "\tsw\tt2," << array_base_offset << "(s0)\n";
    }
}

void backend::Generator::GotoSolve(ir::Instruction *&instr)
{
    // 计算跳转目标的标签（基于当前指令的相对偏移）
    int relative_offset = std::stoi(instr->des.name);

    if (instr->op1.type == ir::Type::null || instr->op1.type == ir::Type::FloatLiteral)
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

void backend::Generator::FAddSolve(ir::Instruction *&instr)
{
    rv::rvFREG frd, frs1, frs2;
    frd = fgetRd(instr->des);
    frs1 = fgetRs1(instr->op1);
    frs2 = fgetRs2(instr->op2);

    int des_offset = varMap.find_operand(instr->des);

    if (des_offset == -1)
    {
        // 检查是否为全局变量
        if (global_vars.find(instr->des.name) != global_vars.end())
        {
            // 是全局变量，生成存储到全局变量的代码
            fout << "\tfadd.s\t" << rv::toString(frd) << "," << rv::toString(frs1) << "," << rv::toString(frs2) << "\n";
            fout << "\tla\ts3," << instr->des.name << "\n";
            fout << "\tfsw\t" << rv::toString(frd) << ",0(s3)\n";
            return;
        }
        else
        {
            // 是局部变量，添加到变量表
            varMap.add_operand(instr->des);
            des_offset = varMap.find_operand(instr->des);
        }
    }

    fout << "\tfadd.s\t" << rv::toString(frd) << "," << rv::toString(frs1) << "," << rv::toString(frs2) << "\n";
    fout << "\tfsw\t" << rv::toString(frd) << "," << des_offset << "(s0)\n";
}

void backend::Generator::FSubSolve(ir::Instruction *&instr)
{
    rv::rvFREG frd, frs1, frs2;
    frd = fgetRd(instr->des);
    frs1 = fgetRs1(instr->op1);
    frs2 = fgetRs2(instr->op2);

    int des_offset = varMap.find_operand(instr->des);

    if (des_offset == -1)
    {
        // 检查是否为全局变量
        if (global_vars.find(instr->des.name) != global_vars.end())
        {
            // 是全局变量，生成存储到全局变量的代码
            fout << "\tfsub.s\t" << rv::toString(frd) << "," << rv::toString(frs1) << "," << rv::toString(frs2) << "\n";
            fout << "\tla\ts3," << instr->des.name << "\n";
            fout << "\tfsw\t" << rv::toString(frd) << ",0(s3)\n";
            return;
        }
        else
        {
            // 是局部变量，添加到变量表
            varMap.add_operand(instr->des);
            des_offset = varMap.find_operand(instr->des);
        }
    }

    fout << "\tfsub.s\t" << rv::toString(frd) << "," << rv::toString(frs1) << "," << rv::toString(frs2) << "\n";
    fout << "\tfsw\t" << rv::toString(frd) << "," << des_offset << "(s0)\n";
}

void backend::Generator::FMulSolve(ir::Instruction *&instr)
{
    rv::rvFREG frd, frs1, frs2;
    frd = fgetRd(instr->des);
    frs1 = fgetRs1(instr->op1);
    frs2 = fgetRs2(instr->op2);

    int des_offset = varMap.find_operand(instr->des);

    if (des_offset == -1)
    {
        // 检查是否为全局变量
        if (global_vars.find(instr->des.name) != global_vars.end())
        {
            // 是全局变量，生成存储到全局变量的代码
            fout << "\tfmul.s\t" << rv::toString(frd) << "," << rv::toString(frs1) << "," << rv::toString(frs2) << "\n";
            fout << "\tla\ts3," << instr->des.name << "\n";
            fout << "\tfsw\t" << rv::toString(frd) << ",0(s3)\n";
            return;
        }
        else
        {
            // 是局部变量，添加到变量表
            varMap.add_operand(instr->des);
            des_offset = varMap.find_operand(instr->des);
        }
    }

    fout << "\tfmul.s\t" << rv::toString(frd) << "," << rv::toString(frs1) << "," << rv::toString(frs2) << "\n";
    fout << "\tfsw\t" << rv::toString(frd) << "," << des_offset << "(s0)\n";
}

void backend::Generator::FDivSolve(ir::Instruction *&instr)
{
    rv::rvFREG frd, frs1, frs2;
    frd = fgetRd(instr->des);
    frs1 = fgetRs1(instr->op1);
    frs2 = fgetRs2(instr->op2);

    int des_offset = varMap.find_operand(instr->des);

    if (des_offset == -1)
    {
        // 检查是否为全局变量
        if (global_vars.find(instr->des.name) != global_vars.end())
        {
            // 是全局变量，生成存储到全局变量的代码
            fout << "\tfdiv.s\t" << rv::toString(frd) << "," << rv::toString(frs1) << "," << rv::toString(frs2) << "\n";
            fout << "\tla\ts3," << instr->des.name << "\n";
            fout << "\tfsw\t" << rv::toString(frd) << ",0(s3)\n";
            return;
        }
        else
        {
            // 是局部变量，添加到变量表
            varMap.add_operand(instr->des);
            des_offset = varMap.find_operand(instr->des);
        }
    }

    fout << "\tfdiv.s\t" << rv::toString(frd) << "," << rv::toString(frs1) << "," << rv::toString(frs2) << "\n";
    fout << "\tfsw\t" << rv::toString(frd) << "," << des_offset << "(s0)\n";
}

void backend::Generator::FMovSolve(ir::Instruction *&instr)
{
    rv::rvFREG frs1;
    frs1 = fgetRs1(instr->op1);

    int des_offset = varMap.find_operand(instr->des);

    // 如果目标变量不在变量表中，将其添加到变量表
    if (des_offset == -1)
    {
        // 检查是否为全局变量
        if (global_vars.find(instr->des.name) != global_vars.end())
        {
            // 是全局变量，生成存储到全局变量的代码
            fout << "\tla\ts3," << instr->des.name << "\n";
            fout << "\tfsw\t" << rv::toString(frs1) << ",0(s3)\n";
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
    fout << "\tfsw\t" << rv::toString(frs1) << "," << des_offset << "(s0)\n";
}

void backend::Generator::CvtI2FSolve(ir::Instruction *&instr)
{
    rv::rvREG rs1;
    rs1 = getRs1(instr->op1);
    rv::rvFREG frd = fgetRd(instr->des);

    int des_offset = varMap.find_operand(instr->des);

    if (des_offset == -1)
    {
        // 检查是否为全局变量
        if (global_vars.find(instr->des.name) != global_vars.end())
        {
            // 是全局变量，生成存储到全局变量的代码
            fout << "\tfcvt.s.w\t" << rv::toString(frd) << "," << rv::toString(rs1) << "\n";
            fout << "\tla\ts3," << instr->des.name << "\n";
            fout << "\tfsw\t" << rv::toString(frd) << ",0(s3)\n";
            return;
        }
        else
        {
            // 是局部变量，添加到变量表
            varMap.add_operand(instr->des);
            des_offset = varMap.find_operand(instr->des);
        }
    }
    fout << "\tfrcsr\tt5\n";
    fout << "\tfsrmi\t2\n";
    fout << "\tfcvt.s.w\t" << rv::toString(frd) << "," << rv::toString(rs1) << "\n";
    fout << "\tfscsr\tt5\n";
    fout << "\tfsw\t" << rv::toString(frd) << "," << des_offset << "(s0)\n";
}

void backend::Generator::CvtF2ISolve(ir::Instruction *&instr)
{
    rv::rvFREG frs1;
    frs1 = fgetRs1(instr->op1);
    rv::rvREG rd = getRd(instr->des);

    int des_offset = varMap.find_operand(instr->des);

    if (des_offset == -1)
    {
        // 检查是否为全局变量
        if (global_vars.find(instr->des.name) != global_vars.end())
        {
            // 是全局变量，生成存储到全局变量的代码
            fout << "\tfcvt.w.s\t" << rv::toString(rd) << "," << rv::toString(frs1) << "\n";
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
    fout << "\tfrcsr\tt5\n";
    fout << "\tfsrmi\t2\n";
    fout << "\tfcvt.w.s\t" << rv::toString(rd) << "," << rv::toString(frs1) << "\n";
    fout << "\tfscsr\tt5\n";
    fout << "\tsw\t" << rv::toString(rd) << "," << des_offset << "(s0)\n";
}

void backend::Generator::FLssSolve(ir::Instruction *&instr)
{
    rv::rvREG rd = getRd(instr->des);
    rv::rvFREG frs1, frs2;
    frs1 = fgetRs1(instr->op1);
    frs2 = fgetRs2(instr->op2);

    int des_offset = varMap.find_operand(instr->des);

    // if (des_offset == -1)
    // {
    //     // 检查是否为全局变量
    //     if (global_vars.find(instr->des.name) != global_vars.end())
    //     {
    //         // 是全局变量，生成存储到全局变量的代码
    //         fout << "\tflt.s\t" << rv::toString(rd) << "," << rv::toString(frs1) << "," << rv::toString(frs2) << "\n";
    //         fout << "\tla\ts3," << instr->des.name << "\n";
    //         fout << "\tsw\t" << rv::toString(rd) << ",0(s3)\n";
    //         return;
    //     }
    //     else
    //     {
    //         // 是局部变量，添加到变量表
    //         varMap.add_operand(instr->des);
    //         des_offset = varMap.find_operand(instr->des);
    //     }
    // }

    fout << "\tflt.s\t" << rv::toString(rd) << "," << rv::toString(frs1) << "," << rv::toString(frs2) << "\n";
    fout << "\tsw\t" << rv::toString(rd) << "," << des_offset << "(s0)\n";
}
