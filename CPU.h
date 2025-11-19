#ifndef CPU_H
#define CPU_H

#include <unordered_map>
#include <vector>
#include <string>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>

using namespace std;

struct Instruction {
    string label;  // optional label
    string op;
    string arg1;
    string arg2;
    Instruction() : label(""), op(""), arg1(""), arg2("") {}
    Instruction(const string &l, const string &o, const string &a1, const string &a2 = "")
        : label(l), op(o), arg1(a1), arg2(a2) {}
};

enum PipelineStage { FETCH, DECODE, EXECUTE, WRITEBACK };

class CPU {
private:
    unordered_map<string,uint64_t> registers;
    unordered_map<string,bool> flags;
    uint8_t memory[256];

    // label -> program index
    map<string,size_t> labelMap;

    uint64_t strToValue(const string &s);

public:
    CPU();

    uint64_t getRegister(const string &name) const;
    bool getFlag(const string &name) const;
    uint8_t getMemory(size_t addr) const;
    void setMemory(size_t addr,uint8_t value);

    // Basic arithmetic / data
    void MOV(const string &dest,const string &src);
    void ADD(const string &dest,const string &src);
    void SUB(const string &dest,const string &src);
    void CMP(const string &reg,const string &src);

    // Advanced operations
    void MUL(const string &reg1, const string &reg2);
    void DIV(const string &reg);
    void INC(const string &reg);
    void DEC(const string &reg);
    void AND(const string &reg1, const string &reg2);
    void OR(const string &reg1, const string &reg2);
    void XOR(const string &reg1, const string &reg2);

    // Control flow
    void JMP(size_t addr,size_t &pc);
    void JE(size_t addr,size_t &pc);
    void JNE(size_t addr,size_t &pc);

    // Execute instruction; pc is updated inside
    void execute(const Instruction &instr,size_t &pc);

    // Build label map from program (label -> index)
    void buildLabelMap(const vector<Instruction> &program);

    void displayState() const;
};

#endif // CPU_H
