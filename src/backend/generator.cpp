#include "backend/generator.h"

#include <assert.h>

#define TODO assert(0 && "todo")

int frame_size      = 520;
int Instr_ID        =   0;
backend::Generator::Generator(ir::Program &p, std::ofstream &f) : program(p), fout(f) {}

int backend::stackVarMap::find_operand(ir::Operand root)
{
    if(_table.find(root) == _table.end())
        return -1; // 全局
    else
        return _table[root]; //局部地址
}

void backend::stackVarMap::add_operand(ir::Operand root, unsigned int size)
{
    auto it = _table.find(root);
    if (it != _table.end())
        return ;
    _table[root] = offset;
    offset       = offset - size;
}

void backend::Generator::gen()
{
    fout << "\t.option nopic\n";
    fout << "\t.data\n";

    GlobalInit(program.globalVal);
    fout << "\t.text\n";

    for(auto it:program.functions)
    {
        funcSolve(it);
    }
}

void backend::Generator::GlobalInit(const std::vector<ir::GlobalVal> &global)
{
    for (auto it : global)
    {
        if(it.maxlen == 0)
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

void backend::Generator::funcSolve(const ir::Function& func)
{
    //是否可以不保存s1->s3的寄存器
    fout << "\t.align\t2\n";
    fout << "\t.globl\t" + func.name + "\n";
    fout << "\t.type\t" + func.name + ",\t" + "@function" + "\n";
    fout << func.name + ":\n";

    fout <<"\taddi\tsp,sp,-" + std::to_string(frame_size) + "\n";
    fout <<"\tsw\tra," + std::to_string(frame_size - 4) + "(sp)\n";
    fout <<"\tsw\ts0," + std::to_string(frame_size - 8) + "(sp)\n";
    fout <<"\tsw\ts1," + std::to_string(frame_size - 12) + "(sp)\n";
    fout <<"\tsw\ts2," + std::to_string(frame_size - 16) + "(sp)\n";
    fout <<"\tsw\ts3," + std::to_string(frame_size - 20) + "(sp)\n";
    fout <<"\taddi\ts0,sp," + std::to_string(frame_size) + "\n";
    // this->sentences.push_back("\tfmv.w.x\tft0,zero");

    paramSolve(func.ParameterList);

    for(auto it:func.InstVec)
    {
        fout << "#" + std::to_string(Instr_ID++) + ": " + it->draw();
        InstrSolve(it);
    }

    fout <<func.name + "_return:\n";
    fout <<"\tlw\tra," + std::to_string(frame_size - 4) + "(sp)\n";
    fout <<"\tlw\ts0," + std::to_string(frame_size - 8) + "(sp)\n";
    fout <<"\tlw\ts1," + std::to_string(frame_size - 12) + "(sp)\n";
    fout <<"\tlw\ts2," + std::to_string(frame_size - 16) + "(sp)\n";
    fout <<"\tlw\ts3," + std::to_string(frame_size - 20) + "(sp)\n";
    fout <<"\taddi\tsp,sp," + std::to_string(frame_size) + "\n";
    fout <<"\tjr\tra\n";
    fout <<"\t.size\t" + func.name + ",\t.-" + func.name + "\n";

}

rv::rvREG getRd(ir::Operand rd)
{
    return rv::rvREG::X7;
}

rv::rvREG getRs1(ir::Operand rs1)
{

    return rv::rvREG::X5;
}

rv::rvREG getRs2(ir::Operand rs2)
{
    return rv::rvREG::X6;
}

void backend::Generator::paramSolve(const std::vector<ir::Operand>& params)
{
    for(auto it:params)
    {
        return;
    }
}

void backend::Generator::InstrSolve(ir::Instruction*& instr)
{
    switch (instr->op)
    {
    case ir::Operator::add:
        AddSolve(instr);
        break;
    default:
        break;
    }
}

void backend::Generator::AddSolve(ir::Instruction*& instr)
{
    rv::rvREG rd, rs1, rs2;
    rd = getRd(instr->des);
    rs1 = getRs1(instr->op1);
    rs2 = getRs2(instr->op2);
}