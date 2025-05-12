#include "backend/generator.h"

#include <assert.h>

#define TODO assert(0 && "todo")

int frame_size = 520;
int Instr_ID = 0;
backend::Generator::Generator(ir::Program &p, std::ofstream &f) : program(p), fout(f) {}

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
        if (it.maxlen == 0)
        {
            fout << it.val.name + ":";
            fout << "\t.word\t0";
        }
        else
        {
            fout << it.val.name + ":";
            fout << "\t.space\t" + std::to_string(it.maxlen * 4);
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
    // fout <<"\tsw\ts1," + std::to_string(frame_size - 12) + "(sp)\n";
    // fout <<"\tsw\ts2," + std::to_string(frame_size - 16) + "(sp)\n";
    // fout <<"\tsw\ts3," + std::to_string(frame_size - 20) + "(sp)\n";
    fout << "\taddi\ts0,sp," + std::to_string(frame_size) + "\n";
    // this->sentences.push_back("\tfmv.w.x\tft0,zero");

    paramSolve(func.ParameterList);

    for (auto it : func.InstVec)
    {
        fout << "#" + std::to_string(Instr_ID++) + ": " + it->draw() + "\n";
        InstrSolve(it);
    }

    fout << func.name + "_return:\n";
    fout << "\tlw\tra," + std::to_string(frame_size - 4) + "(sp)\n";
    fout << "\tlw\ts0," + std::to_string(frame_size - 8) + "(sp)\n";
    // fout <<"\tlw\ts1," + std::to_string(frame_size - 12) + "(sp)\n";
    // fout <<"\tlw\ts2," + std::to_string(frame_size - 16) + "(sp)\n";
    // fout <<"\tlw\ts3," + std::to_string(frame_size - 20) + "(sp)\n";
    fout << "\taddi\tsp,sp," + std::to_string(frame_size) + "\n";
    fout << "\tjr\tra\n";
    fout << "\t.size\t" + func.name + ",\t.-" + func.name + "\n";
}

rv::rvREG backend::Generator::getRd(ir::Operand rd)
{
    // 固定使用t2(x7)作为目标寄存器
    return rv::rvREG::X7; // t2
}

rv::rvREG backend::Generator::getRs1(ir::Operand rs1)
{
    int op1_offset = varMap.find_operand(rs1);
    if (op1_offset != -1)
    {
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
    int op2_offset = varMap.find_operand(rs2);
    if (op2_offset != -1)
    {
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
    case ir::Operator::mov:
        MovSolve(instr);
        break;
    case ir::Operator::load:
        LoadSolve(instr);
        break;
    case ir::Operator::store:
        StoreSolve(instr);
        break;
    // 浮点指令
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
    // 类型转换指令
    case ir::Operator::cvt_i2f:
        CvtI2FSolve(instr);
        break;
    case ir::Operator::cvt_f2i:
        CvtF2ISolve(instr);
        break;
    default:
        // 未实现的指令，输出警告
        fout << "\t# 未实现的指令: " << toString(instr->op) << "\n";
        break;
    }
}

void backend::Generator::CallSolve(ir::Instruction *&instr)
{
    for (auto argument : dynamic_cast<const ir::CallInst &>(*instr).argumentList)
    {
        // 传参
    }
    fout << "\tcall\t" + instr->op1.name + "\n"; 
}

void backend::Generator::ReturnSolve(ir::Instruction *&instr)
{
    if (instr->op1.type != ir::Type::null)
    {
        switch (instr->op1.type)
        {
        case ir::Type::Int:
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
        break;

        case ir::Type::IntLiteral:
        {
            fout << "\tli\ta0," << instr->op1.name << "\n";
        }
        break;

        case ir::Type::Float:
        {
            int op1_offset = varMap.find_operand(instr->op1);

            if (op1_offset != -1)
            {
                fout << "\tflw\tfa0," << op1_offset << "(sp)\n";
            }
            else
            {
                fout << "\tflw\tfa0," << instr->op1.name << "\n";
            }
        }
        break;

        case ir::Type::FloatLiteral:
        {
            // 浮点立即数 - RISC-V不支持直接加载浮点立即数
            // 需要先将其放入内存，再加载
            // 生成一个唯一的标签
            std::string float_label = ".LC" + std::to_string(Instr_ID) + "_" + current_func_name;

            // 在数据段添加浮点常量
            fout << "\t.section\t.rodata\n";
            fout << "\t.align\t2\n";
            fout << float_label << ":\n";
            fout << "\t.word\t" << std::stoi(instr->op1.name) << "\n"; // 以二进制形式存储浮点数
            fout << "\t.text\n";

            // 加载到fa0寄存器
            fout << "\tla\tt0," << float_label << "\n";
            fout << "\tflw\tfa0,0(t0)\n";
        }
        break;

        default:
            fout << "\t# 未处理的返回值类型\n";
            break;
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
        varMap.add_operand(instr->des);
        des_offset = varMap.find_operand(instr->des);
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
        varMap.add_operand(instr->des);
        des_offset = varMap.find_operand(instr->des);
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
        varMap.add_operand(instr->des);
        des_offset = varMap.find_operand(instr->des);
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
        varMap.add_operand(instr->des);
        des_offset = varMap.find_operand(instr->des);
    }

    fout << "\tdiv\t" << rv::toString(rd) << "," << rv::toString(rs1) << "," << rv::toString(rs2) << "\n";
    fout << "\tsw\t" << rv::toString(rd) << "," << des_offset << "(sp)\n";
}

void backend::Generator::MovSolve(ir::Instruction *&instr)
{
    rv::rvREG rd, rs1;
    rd = getRd(instr->des);
    rs1 = getRs1(instr->op1);

    int des_offset = varMap.find_operand(instr->des);

    // 如果目标变量不在变量表中，将其添加到变量表
    if (des_offset == -1)
    {
        varMap.add_operand(instr->des);
        des_offset = varMap.find_operand(instr->des);
    }

    // 执行移动操作 (使用add指令和零寄存器实现mov)
    fout << "\tadd\t" << rv::toString(rd) << "," << rv::toString(rs1) << ",zero\n";

    // 存储结果到内存
    fout << "\tsw\t" << rv::toString(rd) << "," << des_offset << "(sp)\n";
}

void backend::Generator::LoadSolve(ir::Instruction *&instr)
{
    rv::rvREG rd, rs1, rs2;
    rd = getRd(instr->des);
    rs1 = getRs1(instr->op1); // 数组基地址
    rs2 = getRs2(instr->op2); // 偏移量

    int des_offset = varMap.find_operand(instr->des);

    // 如果目标变量不在变量表中，将其添加到变量表
    if (des_offset == -1)
    {
        varMap.add_operand(instr->des);
        des_offset = varMap.find_operand(instr->des);
    }

    // 计算数组元素地址：基地址 + 偏移量*4 (假设每个元素4字节)
    fout << "\tslli\tt3," << rv::toString(rs2) << ",2\n"; // 偏移量*4，存入临时寄存器t3
    fout << "\tadd\tt3," << rv::toString(rs1) << ",t3\n"; // 基地址+偏移量*4，得到元素地址

    // 从计算出的地址加载值到目标寄存器
    fout << "\tlw\t" << rv::toString(rd) << ",0(t3)\n";

    // 存储结果到目标变量
    fout << "\tsw\t" << rv::toString(rd) << "," << des_offset << "(sp)\n";
}

void backend::Generator::StoreSolve(ir::Instruction *&instr)
{
    rv::rvREG rd, rs1, rs2;
    rd = getRd(instr->des);   // 要存储的值
    rs1 = getRs1(instr->op1); // 数组基地址
    rs2 = getRs2(instr->op2); // 偏移量

    // 计算数组元素地址：基地址 + 偏移量*4 (假设每个元素4字节)
    fout << "\tslli\tt3," << rv::toString(rs2) << ",2\n"; // 偏移量*4，存入临时寄存器t3
    fout << "\tadd\tt3," << rv::toString(rs1) << ",t3\n"; // 基地址+偏移量*4，得到元素地址

    // 存储值到计算出的地址
    fout << "\tsw\t" << rv::toString(rd) << ",0(t3)\n";
}

rv::rvFREG backend::Generator::fgetRd(ir::Operand rd)
{
    // 固定使用ft2寄存器作为目标寄存器
    return rv::rvFREG::F3; // ft3
}

rv::rvFREG backend::Generator::fgetRs1(ir::Operand rs1)
{
    int op1_offset = varMap.find_operand(rs1);
    if (op1_offset != -1)
    {
        // 从栈中加载浮点值
        fout << "\tflw\t" << rv::toString(rv::rvFREG::F0) << "," << op1_offset << "(sp)\n";
    }
    else
    {
        // 从全局变量加载浮点值
        fout << "\tflw\t" << rv::toString(rv::rvFREG::F0) << "," << rs1.name << "\n";
    }
    return rv::rvFREG::F0; // ft0
}

rv::rvFREG backend::Generator::fgetRs2(ir::Operand rs2)
{
    int op2_offset = varMap.find_operand(rs2);
    if (op2_offset != -1)
    {
        // 从栈中加载浮点值
        fout << "\tflw\t" << rv::toString(rv::rvFREG::F1) << "," << op2_offset << "(sp)\n";
    }
    else
    {
        // 从全局变量加载浮点值
        fout << "\tflw\t" << rv::toString(rv::rvFREG::F1) << "," << rs2.name << "\n";
    }
    return rv::rvFREG::F2; // ft2
}

void backend::Generator::FAddSolve(ir::Instruction *&instr)
{
    rv::rvFREG rd, rs1, rs2;
    rd = fgetRd(instr->des);
    rs1 = fgetRs1(instr->op1);
    rs2 = fgetRs2(instr->op2);

    int des_offset = varMap.find_operand(instr->des);

    if (des_offset == -1)
    {
        varMap.add_operand(instr->des);
        des_offset = varMap.find_operand(instr->des);
    }

    // 执行浮点加法
    fout << "\tfadd.s\t" << rv::toString(rd) << "," << rv::toString(rs1) << "," << rv::toString(rs2) << "\n";

    // 存储结果到内存
    fout << "\tfsw\t" << rv::toString(rd) << "," << des_offset << "(sp)\n";
}

void backend::Generator::FSubSolve(ir::Instruction *&instr)
{
    rv::rvFREG rd, rs1, rs2;
    rd = fgetRd(instr->des);
    rs1 = fgetRs1(instr->op1);
    rs2 = fgetRs2(instr->op2);

    int des_offset = varMap.find_operand(instr->des);

    if (des_offset == -1)
    {
        varMap.add_operand(instr->des);
        des_offset = varMap.find_operand(instr->des);
    }

    // 执行浮点减法
    fout << "\tfsub.s\t" << rv::toString(rd) << "," << rv::toString(rs1) << "," << rv::toString(rs2) << "\n";

    // 存储结果到内存
    fout << "\tfsw\t" << rv::toString(rd) << "," << des_offset << "(sp)\n";
}

void backend::Generator::FMulSolve(ir::Instruction *&instr)
{
    rv::rvFREG rd, rs1, rs2;
    rd = fgetRd(instr->des);
    rs1 = fgetRs1(instr->op1);
    rs2 = fgetRs2(instr->op2);

    int des_offset = varMap.find_operand(instr->des);

    if (des_offset == -1)
    {
        varMap.add_operand(instr->des);
        des_offset = varMap.find_operand(instr->des);
    }

    // 执行浮点乘法
    fout << "\tfmul.s\t" << rv::toString(rd) << "," << rv::toString(rs1) << "," << rv::toString(rs2) << "\n";

    // 存储结果到内存
    fout << "\tfsw\t" << rv::toString(rd) << "," << des_offset << "(sp)\n";
}

void backend::Generator::FDivSolve(ir::Instruction *&instr)
{
    rv::rvFREG rd, rs1, rs2;
    rd = fgetRd(instr->des);
    rs1 = fgetRs1(instr->op1);
    rs2 = fgetRs2(instr->op2);

    int des_offset = varMap.find_operand(instr->des);

    if (des_offset == -1)
    {
        varMap.add_operand(instr->des);
        des_offset = varMap.find_operand(instr->des);
    }

    // 执行浮点除法
    fout << "\tfdiv.s\t" << rv::toString(rd) << "," << rv::toString(rs1) << "," << rv::toString(rs2) << "\n";

    // 存储结果到内存
    fout << "\tfsw\t" << rv::toString(rd) << "," << des_offset << "(sp)\n";
}

void backend::Generator::FMovSolve(ir::Instruction *&instr)
{
    rv::rvFREG rd, rs1;
    rd = fgetRd(instr->des);
    rs1 = fgetRs1(instr->op1);

    int des_offset = varMap.find_operand(instr->des);

    if (des_offset == -1)
    {
        varMap.add_operand(instr->des);
        des_offset = varMap.find_operand(instr->des);
    }

    // 执行浮点移动操作，直接使用fsgnj.s实现fmv.s
    fout << "\tfsgnj.s\t" << rv::toString(rd) << "," << rv::toString(rs1) << "," << rv::toString(rs1) << "\n";

    // 存储结果到内存
    fout << "\tfsw\t" << rv::toString(rd) << "," << des_offset << "(sp)\n";
}

void backend::Generator::CvtI2FSolve(ir::Instruction *&instr)
{
    // 整数到浮点数的转换
    rv::rvREG rs1 = getRs1(instr->op1); // 获取整数寄存器
    rv::rvFREG rd = fgetRd(instr->des); // 获取浮点寄存器

    int des_offset = varMap.find_operand(instr->des);

    if (des_offset == -1)
    {
        varMap.add_operand(instr->des);
        des_offset = varMap.find_operand(instr->des);
    }

    // 整数转浮点数
    fout << "\tfcvt.s.w\t" << rv::toString(rd) << "," << rv::toString(rs1) << "\n";

    // 存储结果到内存
    fout << "\tfsw\t" << rv::toString(rd) << "," << des_offset << "(sp)\n";
}

void backend::Generator::CvtF2ISolve(ir::Instruction *&instr)
{
    // 浮点数到整数的转换
    rv::rvFREG rs1 = fgetRs1(instr->op1); // 获取浮点寄存器
    rv::rvREG rd = getRd(instr->des);     // 获取整数寄存器

    int des_offset = varMap.find_operand(instr->des);

    if (des_offset == -1)
    {
        varMap.add_operand(instr->des);
        des_offset = varMap.find_operand(instr->des);
    }

    // 浮点数转整数，默认使用rtz舍入模式
    fout << "\tfcvt.w.s\t" << rv::toString(rd) << "," << rv::toString(rs1) << ",rtz\n";

    // 存储结果到内存
    fout << "\tsw\t" << rv::toString(rd) << "," << des_offset << "(sp)\n";
}