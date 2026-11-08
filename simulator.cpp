#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
#include <algorithm>
using namespace std;

const int instruction = 4096;
unsigned short instruction_memory[instruction];
unsigned short dataMemory[65536];
unsigned short Reg[8];
unsigned short ACC = 0;
unsigned short pc = 0;
unsigned short sp = 0xFFFF
;
int instruction_num = 0;
bool program_ending = false;

bool loader(string fileName) {
    ifstream file(fileName, ios::binary);
    if (!file) {
        cout << "file not open" << endl;
        return false;
    }
    file.seekg(0, ios::end);
    long size = (long)file.tellg();
    file.seekg(0, ios::beg);

    if (size <= 0 || size % 2 != 0 || size / 2 > instruction) {
        cout << "invalid file" << endl;
        return false;
    }

    unsigned char byte[2 * instruction];
    file.read((char *)byte, size);

    instruction_num = (int)(size / 2);
    for (int i = 0; i < instruction_num; i++) {
        instruction_memory[i] = (unsigned short)((byte[2 * i] << 8) | byte[2 * i + 1]);
    }
    return true;
}

bool integer_maker(string input, long &value)
{
    value = 0;
    if (input.size() == 0)
        return false;
    for (size_t i = 0; i < input.size(); i++) {
        char c = input[i];
        if (c < '0' || c > '9')
            return false;
        value = value * 10 + (c - '0');
    }
    return true;
}

int step() {
    cout << "Enter line number or 'end' :";
    string input;
    cin >> input;
    long num = 0;
    if (integer_maker(input, num)) {
        if (num >= 0 && num <= instruction_num)
            return (int)num;
    }
    return -1;
}

void reset() {
    ACC = 0;
    pc = 0;
    sp = 0xFFFF;
    program_ending = false;
    for (int i = 0; i < 8; i++)
        Reg[i] = 0;
    fill(dataMemory, dataMemory + 65536, 0);
}

void error_finder(string message) {
    cout << "Error in " << message << endl;
    exit(1);
}

string In_register() {
    stringstream in_register;
    in_register << "ACC=" << ACC;
    for (int i = 0; i < 8; i++)
        in_register << " R" << i << "=" << Reg[i];
    in_register << " SP=" << sp;
    return in_register.str();
}

void simulation(string &traceLine) {
    if (pc >= instruction_num)
        error_finder("pc is larger than program");
    unsigned short startPC = pc;
    unsigned short binary_code = instruction_memory[pc];
    pc += 1;

    int opcode = (binary_code >> 12) & 0xF;
    int operand_1 = (binary_code >> 8) & 0xF;
    int operand_2 = (binary_code >> 4) & 0xF;
    int imm = binary_code & 0xFF;
    int target = binary_code & 0xFFF;

    if (opcode >= 2 && opcode <= 7) {
        if ((binary_code & 0xF) != 0)
            error_finder("reserved bit error");
        if (opcode == 2) {
            if (operand_1 > 8 || operand_2 > 8)
                error_finder("invalid register");
        } else {
            if (operand_1 > 7)
                error_finder("invalid register");
            if (operand_2 != 0)
                error_finder("operand_2 must be zero");
        }
    }
    else if (opcode == 8 || opcode == 13 || opcode == 15) {
        if ((binary_code & 0xFFF) != 0)
            error_finder("reserved bit error");
    }
    else if (opcode == 14) {
        if (operand_1 > 7)
            error_finder("invalid register");
    }

    switch (opcode) {
    case 0:
            ACC = dataMemory[target];
            break;
    case 1:
            dataMemory[target] = ACC;
            break;
    case 2:
           if (operand_1 == 8)
               ACC = Reg[operand_2];
           else if (operand_2 == 8)
               Reg[operand_1] = ACC;
           else
               Reg[operand_1] = Reg[operand_2];
           break;
    case 3:
            ACC = (unsigned short)(ACC + Reg[operand_1]);
            break;
    case 4:
            ACC = (unsigned short)(ACC - Reg[operand_1]);
            break;
    case 5:
            ACC = (unsigned short)((ACC * Reg[operand_1]) & 0xFFFF);
            break;
    case 6:
            ACC = ACC & Reg[operand_1];
            break;
    case 7:
            ACC = ACC | Reg[operand_1];
            break;
    case 8:
            ACC = (unsigned short)(~ACC);
            break;
    case 9:
            pc = (unsigned short)target;
            break;
    case 10:
            if (ACC == 0)
                pc = (unsigned short)target;
            break;

    case 11:
            if (ACC != 0)
                pc = (unsigned short)target;
            break;

    case 12:
        if (sp == 0)
            error_finder("stack overflow");
        dataMemory[sp] = pc;
        sp -= 1;
        pc = (unsigned short)target;
        break;

    case 13:
        if (sp == 0xFFFF)
            error_finder("unvailabel stak address");
        sp += 1;
        pc = dataMemory[sp];
        break;

    case 14:
            Reg[operand_1] = (unsigned short)imm;
            break;
    case 15:
            program_ending = true;
            break;
    }

    stringstream ss;
    ss << "PC=" << startPC << "  IR=" << hex << uppercase << setw(4) << setfill('0') << binary_code << dec;
    ss << ", " << In_register() << " NEXTPC=" << pc;
    traceLine = ss.str();
}

string register_num(int code) {
    if (code >= 0 && code <= 7)
        return string("R") + (char)('0' + code);
    if (code == 8)
        return "ACC";
    error_finder("register name");
    return "";
}

string disassembler(unsigned short binary_code) {
    int opcode = (binary_code >> 12) & 0xF;
    int operand_1 = (binary_code >> 8) & 0xF;
    int operand_2 = (binary_code >> 4) & 0xF;
    int imm = binary_code & 0xFF;
    int target = binary_code & 0xFFF;

    switch (opcode) {
    case 0:
            return "LD " + to_string(target);
    case 1:
            return "ST " + to_string(target);
    case 2:
            return "MOV " + register_num(operand_1) + ", " + register_num(operand_2);
    case 3:
            return "ADD " + register_num(operand_1);
    case 4:
            return "SUB " + register_num(operand_1);
    case 5:
            return "MUL " + register_num(operand_1);
    case 6:
            return "AND " + register_num(operand_1);
    case 7:
            return "OR " + register_num(operand_1);
    case 8:
            return "NOT";
    case 9:
            return "JMP " + to_string(target);
    case 10:
            return "JZ " + to_string(target);
    case 11:
            return "JNZ " + to_string(target);
    case 12:
            return "CALL " + to_string(target);
    case 13:
            return "RET";
    case 14:
            return "LDI " + register_num(operand_1) + ", " + to_string(imm);
    default:
            return "HALT";
    }
}

int main() {
    bool run_end = false;
    int steps_to_run = 0;
    string filename ,traceLine;;
    cout << "Enter binary file name: ";
    cin >> filename;
    if (!loader(filename)) 
        return 1;

    while (!program_ending) {
        if (steps_to_run == 0 && !run_end) {
            int run = step();
            reset();
            if (run == -1)
                run_end = true;
            else if (run > 0)
                steps_to_run = run;
        }

        simulation(traceLine);
        if (!run_end) {
            if (steps_to_run == 1) {
                cout << traceLine << endl;
            }
            steps_to_run--;
        }
    }
    if (program_ending) {
        cout << "Final State: " << In_register() << " PC=" << pc << endl;
        cout << "\nDisassembler:" << endl;
        for (int i = 0; i < instruction_num; i++)
            cout << disassembler(instruction_memory[i]) << endl;
    }
    return 0;
}