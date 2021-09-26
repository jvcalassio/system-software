#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>

using namespace std;

// [ opcode ] = { qtd_argumentos, mnemonico }
// instruction_table[ 2 ] = { 1 argumento, instrucao "sub" }
map<int, pair<int,string>> instruction_table = {
    {1, {1, "add"}},
    {2, {1, "sub"}},
    {3, {1, "mul"}},
    {4, {1, "div"}},
    {5, {1, "jmp"}},
    {6, {1, "jmpn"}},
    {7, {1, "jmpp"}},
    {8, {1, "jmpz"}},
    {9, {2, "copy"}},
    {10, {1, "load"}},
    {11, {1, "store"}},
    {12, {1, "input"}},
    {13, {1, "output"}},
    {14, {0, "stop"}}
};

/**
 * Implementacao de string split
 * Ignora strings em branco
 * */
vector<string> split(string str, string delimiter) {
    size_t start = 0, end = 0;
    vector<string> splitted;

    while ((end = str.find(delimiter, start)) != string::npos) {
        string tmp = str.substr(start, end - start);
        if(tmp != "") {    
            splitted.push_back(tmp);
        }
        start = end + 1;
    }

    string tmp = str.substr(start);
    if (tmp != "") {
        splitted.push_back(tmp);
    }

    return splitted;
}

/**
 * Faz leitura do arquivo objeto
 * */
vector<int> read_code(char* file_path) {
    ifstream file(file_path);

    string line;

    vector<int> code;

    while(getline(file, line)) {
        vector<string> separated = split(line, " ");

        for(string i : separated) {
            code.push_back( atoi( i.c_str() ) );
        }
    }

    file.close();

    return code;
}

/**
 * Escreve arquivo de output
 * */
void write_code(string file_name, vector<int> code) {
    ofstream output;
    output.open(file_name + ".out");

    for(int &w : code) {
        output << w << endl;
    }

    output.close();
}

/**
 * Realiza simulacao
 * */
vector<int> simulate(vector<int> code) {
    int PC = 0;
    int ACC = 0;

    vector<int> output;

    while(PC < code.size()) {
        // codigo da instrucao em PC
        int instruction = code[PC];
        // mapeia codigo para qtd operandos / string identificadora
        pair<int, string> props = instruction_table[instruction];

        // salva PC original para saber se houve jump ao final
        int original_pc = PC;
        // flag de stop
        int stopped = 0;
        // flag de output
        int has_output = 0;

        // tratamento de cada instrucao individualmente
        if(props.second == "add") {
            ACC = ACC + code[ code[PC + 1] ];
        } else if(props.second == "sub") {
            ACC = ACC - code[ code[PC + 1] ];
        } else if(props.second == "mul") {
            ACC = ACC * code[ code[PC + 1] ];
        } else if(props.second == "div") {
            ACC = ACC / code[ code[PC + 1] ];
        } else if(props.second == "jmp") {
            PC = code[PC + 1];
        } else if(props.second == "jmpn") {
            if(ACC < 0) {
                PC = code[PC + 1];
            }
        } else if(props.second == "jmpp") {
            if(ACC > 0) {
                PC = code[PC + 1];
            }
        } else if(props.second == "jmpz") {
            if(ACC == 0) {
                PC = code[PC + 1];
            }
        } else if(props.second == "copy") {
            code[ code[PC + 2] ] = code[ code[PC + 1] ];
        } else if(props.second == "load") {
            ACC = code[ code[PC + 1] ];
        } else if(props.second == "store") {
            code[ code[PC + 1] ] = ACC; 
        } else if(props.second == "input") { // input
            int in;
            cin >> in;
            code[ code[PC + 1] ] = in;
        } else if(props.second == "output") {
            output.push_back( code[ code[PC + 1] ] );
            has_output = 1;
        } else if(props.second == "stop") {
            stopped = 1;
        }

        if(original_pc == PC) {
            PC += props.first + 1;
        }

        cout << "PC : " << PC << endl;
        cout << "ACC : " << ACC << endl;

        if(has_output) {
            cout << "OUTPUT: " << output.back() << endl;
        }

        cout << endl;

        if(stopped) break;
    }

    return output;
}

int main(int argc, char* argv[]) {
    if(argc < 2) {
        cout << "Execute o programa especificando qual arquivo sera utilizado: ./simulador programa.obj" << endl;
        return 0;
    }

    char* file_path = argv[1];

    // separa nome do arquivo
    string file_name = file_path;
    size_t endf = file_name.find(".obj");
    if(endf != string::npos)
        file_name.erase(file_name.begin() + endf, file_name.end());

    vector<int> code = read_code(file_path);

    vector<int> output = simulate(code);

    write_code(file_name, output);

    return 0;
}