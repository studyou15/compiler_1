#ifndef GENERARATOR_H
#define GENERARATOR_H

#include "ir/ir.h"
#include "backend/rv_def.h"
#include "backend/rv_inst_impl.h"


#include<unordered_map>
#include<string>
#include<vector>
#include<fstream>
#include<unordered_set>

namespace backend {

// it is a map bewteen variable and its mem addr, the mem addr of a local variable can be identified by ($sp + off)
struct stackVarMap {
    std::unordered_map<std::string, int> _table;
    int offset = -56;
    /**
     * @brief find the addr of a ir::Operand
     * @return the offset
    */
    int find_operand(ir::Operand);

    /**
     * @brief add a ir::Operand into current map, alloc space for this variable in memory 
     * @param[in] size: the space needed(in byte)
     * @return the offset
    */
    void add_operand(ir::Operand, unsigned int size = 4);

};


struct Generator {
    const ir::Program& program;         // the program to gen
    std::ofstream& fout;                 // output file
    stackVarMap varMap;                 // 存储变量和栈内存偏移量的映射
    std::string current_func_name;      // 当前正在处理的函数名
    std::unordered_set<std::string> global_vars;  // 用于存储全局变量名称

    Generator(ir::Program&, std::ofstream&);

    // reg allocate api
    rv::rvREG getRd(ir::Operand,bool p=0);
    rv::rvFREG fgetRd(ir::Operand);
    rv::rvREG getRs1(ir::Operand);
    rv::rvREG getRs2(ir::Operand);
    rv::rvFREG fgetRs1(ir::Operand);
    rv::rvFREG fgetRs2(ir::Operand);

    // generate wrapper function
    void gen();
    void gen_func(const ir::Function&);
    void gen_instr(const ir::Instruction&);

    void GlobalInit(const std::vector<ir::GlobalVal>&);
    void funcSolve(const ir::Function&);
    void paramSolve(const std::vector<ir::Operand>&);
    void InstrSolve(ir::Instruction*&);

    // 指令处理函数
    void AddSolve(ir::Instruction*&);
    void SubSolve(ir::Instruction*&);
    void MulSolve(ir::Instruction*&);
    void DivSolve(ir::Instruction*&);
    void DefSolve(ir::Instruction*&);
    void FDefSolve(ir::Instruction*&);
    void MovSolve(ir::Instruction*&);
    void LoadSolve(ir::Instruction*&);
    void StoreSolve(ir::Instruction*&);
    void ReturnSolve(ir::Instruction*&);
    void CallSolve(ir::Instruction*&);
    void AllocSolve(ir::Instruction*&);
    void RemSolve(ir::Instruction*&);
    // 浮点指令处理函数
    void FAddSolve(ir::Instruction*&);
    void FSubSolve(ir::Instruction*&);
    void FMulSolve(ir::Instruction*&);
    void FDivSolve(ir::Instruction*&);
    void FMovSolve(ir::Instruction*&);
    void OrSolve(ir::Instruction*&);
    void AndSolve(ir::Instruction*&);
    // 类型转换指令
    void CvtI2FSolve(ir::Instruction*&);
    void CvtF2ISolve(ir::Instruction*&);

    // 条件判断指令
    void EqSolve(ir::Instruction*&);
    void NeqSolve(ir::Instruction*&);
    void SltSolve(ir::Instruction*&);
    void SltXSolve(ir::Instruction*&);
    void SeqzSolve(ir::Instruction*&);
    // 跳转指令
    void GotoSolve(ir::Instruction*&);
    
    // 全局变量初始化
    void GlobalDefSolve(ir::Instruction*&);
};



} // namespace backend


#endif