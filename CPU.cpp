#include "CPU.h"


using namespace std;

CPU::CPU() {
    registers = {{"RAX",0},{"RBX",0},{"RCX",0},{"RDX",0},
                 {"RSI",0},{"RDI",0},{"RSP",0},{"RBP",0},{"RIP",0}};
    flags = {{"ZF",false},{"CF",false},{"SF",false},{"OF",false}};
    for(int i=0;i<256;i++) memory[i]=0;

    // set a reasonable initial stack pointer inside our small memory
    registers["RSP"] = 240; // top of stack near end of 256 bytes
}

uint64_t CPU::strToValue(const string &s) {
    // supports decimal and 0x hex prefix
    if (s.rfind("0x", 0) == 0 || s.rfind("0X",0) == 0) {
        return stoull(s.substr(2), nullptr, 16);
    }
    return stoull(s);
}

uint64_t CPU::getRegister(const string &name) const {
    auto it = registers.find(name);
    if (it == registers.end()) throw out_of_range("Unknown register: " + name);
    return it->second;
}
bool CPU::getFlag(const string &name) const {
    auto it = flags.find(name);
    if (it == flags.end()) throw out_of_range("Unknown flag: " + name);
    return it->second;
}
uint8_t CPU::getMemory(size_t addr) const {
    if (addr>=256) throw out_of_range("Memory out of range");
    return memory[addr];
}
void CPU::setMemory(size_t addr,uint8_t value) {
    if (addr>=256) throw out_of_range("Memory out of range");
    memory[addr] = value;
}

// ---------- basic instructions ----------
void CPU::MOV(const string &dest,const string &src){
    if (registers.find(dest) == registers.end()) throw out_of_range("Invalid MOV dest: " + dest);
    if(registers.find(src)!=registers.end()) registers[dest]=registers[src];
    else registers[dest]=strToValue(src);
}
void CPU::ADD(const string &dest,const string &src){
    if (registers.find(dest) == registers.end()) throw out_of_range("Invalid ADD dest: " + dest);
    uint64_t value = (registers.find(src)!=registers.end())?registers[src]:strToValue(src);
    uint64_t a = registers[dest];
    uint64_t result = a + value;
    // Flags: ZF, SF, CF, OF
    flags["ZF"] = (result == 0);
    flags["SF"] = ((result >> 63) & 1);
    flags["CF"] = (result < a); // carry if wrapped
    // OF for signed overflow: sign change when adding same sign operands
    bool sign_a = ((a >> 63) & 1);
    bool sign_b = ((value >> 63) & 1);
    bool sign_r = ((result >> 63) & 1);
    flags["OF"] = ( (sign_a == sign_b) && (sign_r != sign_a) );
    registers[dest]=result;
}
void CPU::SUB(const string &dest,const string &src){
    if (registers.find(dest) == registers.end()) throw out_of_range("Invalid SUB dest: " + dest);
    uint64_t value = (registers.find(src)!=registers.end())?registers[src]:strToValue(src);
    uint64_t a = registers[dest];
    uint64_t result = a - value;
    flags["ZF"] = (result == 0);
    flags["SF"] = ((result >> 63) & 1);
    flags["CF"] = (a < value); // borrow -> carry flag for subtraction
    // OF: signed overflow when signs differ and result sign differs from a
    bool sign_a = ((a >> 63) & 1);
    bool sign_b = ((value >> 63) & 1);
    bool sign_r = ((result >> 63) & 1);
    flags["OF"] = ( (sign_a != sign_b) && (sign_r != sign_a) );
    registers[dest]=result;
}
void CPU::CMP(const string &reg,const string &src){
    if (registers.find(reg) == registers.end()) throw out_of_range("Invalid CMP reg: " + reg);
    uint64_t value = (registers.find(src)!=registers.end())?registers[src]:strToValue(src);
    uint64_t a = registers[reg];
    uint64_t result = a - value;
    flags["ZF"] = (result == 0);
    flags["SF"] = ((result >> 63) & 1);
    flags["CF"] = (a < value);
    bool sign_a = ((a >> 63) & 1);
    bool sign_b = ((value >> 63) & 1);
    bool sign_r = ((result >> 63) & 1);
    flags["OF"] = ( (sign_a != sign_b) && (sign_r != sign_a) );
}

// ---------- advanced ----------
void CPU::MUL(const string &reg1,const string &reg2){
    if (registers.find(reg1)==registers.end() || registers.find(reg2)==registers.end())
        throw out_of_range("Invalid MUL regs");
    __uint128_t r = (__uint128_t)registers[reg1] * (__uint128_t)registers[reg2];
    // store low 64 bits in RAX, high 64 bits in RDX
    registers["RAX"] = (uint64_t)r;
    registers["RDX"] = (uint64_t)(r >> 64);
    uint64_t result_low = registers["RAX"];
    flags["ZF"] = (result_low == 0);
    flags["SF"] = ((result_low >> 63) & 1);
    // CF/OF set if high half != 0 (overflow out of 64 bits)
    flags["CF"] = (registers["RDX"] != 0);
    flags["OF"] = flags["CF"];
}

void CPU::DIV(const string &reg){
    if (registers.find(reg)==registers.end()) throw out_of_range("Invalid DIV reg");
    uint64_t divisor = registers[reg];
    if (divisor == 0) throw runtime_error("Division by zero");
    uint64_t dividend = registers["RAX"];
    registers["RDX"] = dividend % divisor;
    registers["RAX"] = dividend / divisor;
    flags["ZF"] = (registers["RAX"] == 0);
    flags["SF"] = ((registers["RAX"] >> 63) & 1);
    flags["CF"] = false;
    flags["OF"] = false;
}

void CPU::INC(const string &reg){
    if (registers.find(reg)==registers.end()) throw out_of_range("Invalid INC reg");
    uint64_t before = registers[reg];
    registers[reg]++;
    uint64_t after = registers[reg];
    flags["ZF"] = (after == 0);
    flags["SF"] = ((after >> 63) & 1);
    bool sign_before = ((before >> 63) & 1);
    bool sign_after = ((after >> 63) & 1);
    flags["OF"] = (sign_before == 0 && sign_after == 1);
    flags["CF"] = (after < before);
}

void CPU::DEC(const string &reg){
    if (registers.find(reg)==registers.end()) throw out_of_range("Invalid DEC reg");
    uint64_t before = registers[reg];
    registers[reg]--;
    uint64_t after = registers[reg];
    flags["ZF"] = (after == 0);
    flags["SF"] = ((after >> 63) & 1);
    bool sign_before = ((before >> 63) & 1);
    bool sign_after = ((after >> 63) & 1);
    flags["OF"] = (sign_before == 1 && sign_after == 0);
    flags["CF"] = (after > before);
}

void CPU::AND(const string &reg1,const string &reg2){
    if (registers.find(reg1)==registers.end() || registers.find(reg2)==registers.end())
        throw out_of_range("Invalid AND regs");
    registers[reg1] &= registers[reg2];
    flags["ZF"] = (registers[reg1] == 0);
    flags["SF"] = ((registers[reg1] >> 63) & 1);
    flags["CF"] = false;
    flags["OF"] = false;
}

void CPU::OR(const string &reg1,const string &reg2){
    if (registers.find(reg1)==registers.end() || registers.find(reg2)==registers.end())
        throw out_of_range("Invalid OR regs");
    registers[reg1] |= registers[reg2];
    flags["ZF"] = (registers[reg1] == 0);
    flags["SF"] = ((registers[reg1] >> 63) & 1);
    flags["CF"] = false;
    flags["OF"] = false;
}

void CPU::XOR(const string &reg1,const string &reg2){
    if (registers.find(reg1)==registers.end() || registers.find(reg2)==registers.end())
        throw out_of_range("Invalid XOR regs");
    registers[reg1] ^= registers[reg2];
    flags["ZF"] = (registers[reg1] == 0);
    flags["SF"] = ((registers[reg1] >> 63) & 1);
    flags["CF"] = false;
    flags["OF"] = false;
}

// ---------- control flow ----------
void CPU::JMP(size_t addr,size_t &pc){ pc = addr; }
void CPU::JE(size_t addr,size_t &pc){ if(flags["ZF"]) pc = addr; else pc++; }
void CPU::JNE(size_t addr,size_t &pc){ if(!flags["ZF"]) pc = addr; else pc++; }

void CPU::execute(const Instruction &instr,size_t &pc){
    if(instr.op == "MOV") { MOV(instr.arg1, instr.arg2); pc++; }
    else if(instr.op == "ADD") { ADD(instr.arg1, instr.arg2); pc++; }
    else if(instr.op == "SUB") { SUB(instr.arg1, instr.arg2); pc++; }
    else if(instr.op == "CMP") { CMP(instr.arg1, instr.arg2); pc++; }
    else if(instr.op == "MUL") { MUL(instr.arg1, instr.arg2); pc++; }
    else if(instr.op == "DIV") { DIV(instr.arg1); pc++; }
    else if(instr.op == "INC") { INC(instr.arg1); pc++; }
    else if(instr.op == "DEC") { DEC(instr.arg1); pc++; }
    else if(instr.op == "AND") { AND(instr.arg1, instr.arg2); pc++; }
    else if(instr.op == "OR")  { OR(instr.arg1, instr.arg2); pc++; }
    else if(instr.op == "XOR") { XOR(instr.arg1, instr.arg2); pc++; }
    else if(instr.op == "JMP") {
        if(labelMap.find(instr.arg1)==labelMap.end()) throw runtime_error("Unknown label: " + instr.arg1);
        JMP(labelMap[instr.arg1], pc);
    }
    else if(instr.op == "JE") {
        if(labelMap.find(instr.arg1)==labelMap.end()) throw runtime_error("Unknown label: " + instr.arg1);
        JE(labelMap[instr.arg1], pc);
    }
    else if(instr.op == "JNE") {
        if(labelMap.find(instr.arg1)==labelMap.end()) throw runtime_error("Unknown label: " + instr.arg1);
        JNE(labelMap[instr.arg1], pc);
    }
    else {
        // unknown op -> just advance
        pc++;
    }
}

void CPU::buildLabelMap(const vector<Instruction> &program){
    labelMap.clear();
    for(size_t i=0;i<program.size();++i){
        if(!program[i].label.empty()) labelMap[program[i].label] = i;
    }
}

void CPU::displayState() const {
    cout << "Registers:\n";
    for (auto &r : registers) cout << r.first << "=" << r.second << "\n";
    cout << "Flags: ";
    for (auto &f : flags) cout << f.first << "=" << f.second << " ";
    cout << "\nMemory(16 bytes): ";
    for (int i = 0; i < 16; ++i) cout << (int)memory[i] << " ";
    cout << "\n";
}
