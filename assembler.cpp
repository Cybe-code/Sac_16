#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
using namespace std;
const int Line = 4096;
const int Instruction  = 4096;
const int Label = 4096;

enum Opcode {
    LD = 0, ST = 1, MOV = 2, ADD = 3,
    SUB = 4, MUL = 5, AND = 6, OR = 7,
    NOT = 8, JMP = 9, JZ = 10, JNZ = 11,
    CALL = 12, RET = 13, LDI = 14, HALT = 15 };

string line_line[Line];
int line_count = 0;
string label_name[Label];
int label_address[Label];
int label_count = 0;
unsigned short binary[Instruction];
int wordCount = 0;

string space_remover(string input) {
    int start = 0;
    int end = (int)input.size() - 1;
    while (start <= end && input[start] == ' ')
        start++;
    while (end >= start && input[end] == ' ')
        end--;
    return input.substr(start, end - start + 1);
}

string line_cleaner(string input) {
    size_t sign = input.find(';');
    if (sign != string::npos)
        input = input.substr(0, sign);
    string clean_line = "";
    for (size_t i = 0; i < input.size(); i++) {
        char character = input[i];
        if (character == '\t' || character == ',' || character == '\r')
            character = ' ';
        if (character >= 'a' && character <= 'z')
            character = character - 'a' + 'A';
        clean_line += character;
    }
    return space_remover(clean_line);
}

bool correct_label(string input) {
    if (input.size() == 0)

        return false;
    for (size_t i = 0; i < input.size(); i++) {
        char character = input[i];
        bool letter = (character >= 'A' && character <= 'Z') || character == '_';
        bool digit  = (character >= '0' && character <= '9');
        if ((i == 0 && !letter) || (!letter && !digit))
            return false;
    }
    return true;
}

int label_instruction(string line, string &label, string &instruction) {
    label = "";
    instruction = line;
    size_t sign = line.find(':');
    if (sign == string::npos)
        return 0;
    label = space_remover(line.substr(0, sign));
    instruction = space_remover(line.substr(sign + 1));
    if ((!correct_label(label)) || (instruction.find(':') != string::npos))
        return 1;
    
    return 0;
}

int label_finder(string input) {
    for (int i = 0; i < label_count; i++) {
        if (label_name[i] == input)
            return i;
    }
    return -1;
}

bool integer_maker(string text, int &value) {
    if (text.size() == 0)
        return false;
    int base = 10;
    size_t i = 0;
    if (text.size() > 2 && text[0] == '0' && text[1] == 'X') {
        base = 16;
        i = 2;
    }
    long num = 0;
    for (; i < text.size(); i++) {
        char character = text[i];
        int digit;
        if (character >= '0' && character <= '9')
            digit = character - '0';
        else if (base == 16 && character >= 'A' && character <= 'F')
            digit = character - 'A' + 10;
        else
            return false;
        num = num * base + digit;
        if (num > 4096)
            return false;
    }
    value = (int)num;
    return true;
}

int register_num(string input) {
    if (input == "ACC")
        return 8;
    if (input.size() == 2 && input[0] == 'R' && input[1] >= '0' && input[1] <= '7') {
        return input[1] - '0';
    }
    return -1;
}

int opcode(string input) {
    if (input == "LD")
        return LD;
    if (input == "ST")
        return ST;
    if (input == "MOV")
        return MOV;
    if (input == "ADD")
        return ADD;
    if (input == "SUB")
        return SUB;
    if (input == "MUL")
        return MUL;
    if (input == "AND")
        return AND;
    if (input == "OR")
        return OR;
    if (input == "NOT")
        return NOT;
    if (input == "JMP")
        return JMP;
    if (input == "JZ")
        return JZ;
    if (input == "JNZ")
        return JNZ;
    if (input == "CALL")
        return CALL;
    if (input == "RET")
        return RET;
    if (input == "LDI")
        return LDI;
    if (input == "HALT")
        return HALT;
    return -1;
}

void error_finder(int line_num) {
    cout << "Error in line " << line_num << endl;
    exit(1);
}

bool ending_label(int line_num, string input, int &ending_label) {
    char first_char = input[0];
    bool digit = (first_char >= '0' && first_char <= '9');
    if (digit) {
        if (!integer_maker(input, ending_label)) {
            error_finder(line_num);
            return false;
        }
    }
    else if (correct_label(input)) {
        int index = label_finder(input);
        if (index == -1) {
            error_finder(line_num);
            return false;
        }
        ending_label = label_address[index];
    }
    else {
        error_finder(line_num);
        return false;
    }
    if (ending_label < 0 || ending_label > 4095) {
        error_finder(line_num);
        return false;
    }
    return true;
}

int encoder(int line_num, string word[], int number) {
    string name = word[0];
    int op = opcode(name);
    if (op == -1) {
        error_finder(line_num);
        return -1;
    }
    int operand = number - 1;
    int register_1 = 0;
    int register_2 = 0;
    switch (op) {
        case LD:
        case ST: {
            if (operand != 1) {
                error_finder(line_num);
                return -1;
            }
            int addr = 0;
            if (!integer_maker(word[1], addr) || addr > 4095) {
                error_finder(line_num);
                return -1;
            }
            return (op << 12) | addr;
        }

        case MOV: {
            if (operand != 2) {
                error_finder(line_num);
                return -1;
            }
            register_1 = register_num(word[1]);
            register_2 = register_num(word[2]);
            if (register_1 == -1 || register_2 == -1) {
                error_finder(line_num);
                return -1;
            }
            return (op << 12) | (register_1 << 8) | (register_2 << 4);
        }

        case ADD:
        case SUB:
        case MUL:
        case AND:
        case OR: {
            if (operand != 1) {
                error_finder(line_num);
                return -1;
            }
            register_1 = register_num(word[1]);
            if (register_1 == -1 || register_1 == 8) {
                error_finder(line_num);
                return -1;
            }
            return (op << 12) | (register_1 << 8);
        }

        case NOT:
        case RET:
        case HALT: {
            if (operand != 0) {
                error_finder(line_num);
                return -1;
            }
            return op << 12;
        }

        case JMP:
        case JZ:
        case JNZ:
        case CALL: {
            if (operand != 1) {
                error_finder(line_num);
                return -1;
            }
            int target = 0;
            if (!ending_label(line_num, word[1], target))
                return -1;
            return (op << 12) | target;
        }

        case LDI: {
            if (operand != 2) {
                error_finder(line_num);
                return -1;
            }
            register_1 = register_num(word[1]);
            if (register_1 == -1 || register_1 == 8) {
                error_finder(line_num);
                return -1;
            }
            int imm = 0;
            if (!integer_maker(word[2], imm)) {
                error_finder(line_num);
                return -1;
            }
            if (imm < 0 || imm > 255) {
                error_finder(line_num);
                return -1;
            }
            return (op << 12) | (register_1 << 8) | imm;
        }
    }
    return -1;
}

int main( ) {
    string input = "";
    string output = "C:/Users/Ali/CLionProjects/sac16_assembler/bincode.bin";
    string line;
    int address = 0;

    cout << "Enter your assembly file name: ";
    cin >> input;
    ifstream inFile(input);
    if (!inFile) {
        cout << "File not opened" << endl;
        return 1;
    }

    while (getline(inFile, line)) {
        if (line_count >= Line)
            error_finder(0);
        line_line[line_count] = line_cleaner(line);
        line_count++;
    }
    inFile.close();

    for (int i = 0; i < line_count; i++) {
        string label, instruction;
        int status = label_instruction(line_line[i], label, instruction);
        if (status != 0)
            error_finder(i + 1);

        if (label != "") {
            if (label_finder(label) != -1)
                error_finder(i + 1);
            else if (label_count >= Label)
                error_finder(i + 1);
            else {
                label_name[label_count] = label;
                label_address[label_count] = address;
                label_count++;
            }
        }

        if (instruction != "") {
            address++;
            if (address > Instruction)
                error_finder(i + 1);
        }
    }

    for (int i = 0; i < line_count; i++) {
        string label, instruction;
        label_instruction(line_line[i], label, instruction);
        if (instruction == "")
            continue;
        if (wordCount >= Instruction)
            break;

        string word[3];
        int count = 0;
        istringstream words(instruction);
        string temp;
        while (words >> temp) {
            if (count < 3)
                word[count] = temp;
            else
                error_finder(i+1);
            count++;
        }
        binary[wordCount] = encoder(i + 1, word, count);
            wordCount++;
    }
    ofstream binFile(output, ios::binary);
    for (int i = 0; i < wordCount; i++) {
        unsigned char byte1 = (binary[i] >> 8) & 0xFF;
        unsigned char byte2  = binary[i] & 0xFF;
        binFile.put((char)byte1);
        binFile.put((char)byte2);
    }
    binFile.close();
    for (int i = 0; i < wordCount; i++) {
        unsigned short value = binary[i];
        for (int bit = 15; bit >= 0; bit--) {
            cout << ((value >> bit) & 1);
            if (bit % 4 == 0 && bit != 0)
                cout << "_";
        }
        cout << endl;
    }
    cout << endl;
    return 0;
}