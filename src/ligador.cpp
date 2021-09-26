#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <utility>

using namespace std;

map<string, int> global_def_table; // tabela global de definicoes

/**
 * Implementacao de string split
 * Ignora strings em branco
 */
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
 * Extrai a tabela de definicoes de um arquivo objeto
 * */
int read_dt(char* file_path, int correction_factor, string *file_name) {
    ifstream file(file_path);

    string line;
    int file_size = 0;

    while(getline(file, line)) {
        vector<string> line_data = split(line, " ");

        string type = line_data[0], first = line_data[1];
        string second = "";

        if(line_data.size() == 3) { // verifica se existe um terceiro elemento na linha
            second = line_data[2];
        }

        if(type == "S") { // tamanho do programma
            file_size = atoi(first.c_str());
        } else if(type == "D") { // tabela de definicoes
            global_def_table[ first ] = atoi(second.c_str()) + correction_factor;
        } else if(type == "H") { // nome do arquivo
            if(*file_name == "")
                *file_name = first + "_linked";
        }

    }

    file.close();

    return file_size;
}

/**
 * Interpreta cada arquivo individualmente, aplicando
 * o fator de correcao nas linhas do texto
 * */
int link_file(char* file_path, int correction_factor, vector<int> &obj_code) {
    ifstream file(file_path);

    string line;
    int file_size = 0;
    vector<int> reloc_info;
    vector<int> tmp_obj_code;
    vector<pair<string,int>> use_table;

    while(getline(file, line)) {
        vector<string> line_data = split(line, " ");

        string type = line_data[0], first = line_data[1];
        string second = "";

        if(line_data.size() == 3) { // verifica se existe um terceiro elemento na linha
            second = line_data[2];
        }

        if(type == "S") { // tamanho do programma
            file_size = atoi(first.c_str());
        } else if(type == "R") { // informacao de relocacao
            for(int i=0;i<first.size();i++) {
                reloc_info.push_back( first[i] - '0' );
            }
        } else if(type == "U") { // tabela de uso
            use_table.push_back(make_pair(first, correction_factor + atoi(second.c_str())));
        } else if(type == "T") { // texto
            tmp_obj_code.push_back(atoi(first.c_str()));
            if(second != "")
                tmp_obj_code.push_back(atoi(second.c_str()));
        }

    }

    file.close();

    // verifica quais linhas do arquivo objeto precisam ser realocadas,
    // e aplica o fator de correcao
    for(int i=0;i<tmp_obj_code.size();i++) {
        if(reloc_info[i] == 1) {
            obj_code.push_back(tmp_obj_code[i] + correction_factor);
        } else {
            obj_code.push_back(tmp_obj_code[i]);
        }
    }

    // corrige enderecos que estao na tabela de uso do arquivo
    for(auto &i : use_table) {
        int pos = global_def_table[ i.first ];
        obj_code[ i.second ] = pos;
    }

    return file_size;
}

int main(int argc, char* argv[]) {
    if(argc < 2) {
        cout << "Execute o programa especificando qual arquivo sera utilizado: ./ligador programa.obj" << endl;
        return 0;
    } else if(argc > 4) {
        cout << "Sao permitidos apenas 3 modulos, no total. Execute novamente com um, dois, ou tres modulos." << endl;
        return 0;
    }

    int module_count = argc - 1;
    int correction_factor = 0;
    string file_name = "";

    // gera tabela global de definicoes
    for(int i=0;i<module_count;i++) {
        char* file_path = argv[1+i];
        correction_factor = read_dt(file_path, correction_factor, &file_name);
    }

    correction_factor = 0;
    vector<int> obj_code;
    // faz a ligacao, na ordem dada
    for(int i=0;i<module_count;i++) {
        char* file_path = argv[1+i];
        correction_factor = link_file(file_path, correction_factor, obj_code);
    }

    ofstream binary;
    binary.open(file_name + ".obj"); // nome do arquivo: nome do primeiro + _linked.obj
    // gera o arquivo final
    for(auto &i : obj_code) {
        binary << i << " ";
    }

    binary.close();

    return 0;
}