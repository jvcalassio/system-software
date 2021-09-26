#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <algorithm>
#include <utility>
#include <vector>

using namespace std;

// identificadores de erro
enum errors {
    ERROR_INVALID_SYMBOL = 100,
    ERROR_DUPLICATE_SYMBOL,
    ERROR_INEXISTENT_SYMBOL,
    ERROR_INVALID_INSTRUCTION,
    ERROR_INEXISTENT_INSTRUCTION,
    ERROR_INVALID_OPERATOR_COUNT,
    ERROR_INVALID_OPERATOR_TYPE,
    ERROR_INVALID_TOKEN
};


// instruction_table["add"] = { opcode 1 , 2 palavras }
map<string, pair<int,int>> instruction_table = {
    {"add", {1,2}},
    {"sub", {2,2}},
    {"mul", {3,2}},
    {"div", {4,2}},
    {"jmp", {5,2}},
    {"jmpn", {6,2}},
    {"jmpp", {7,2}},
    {"jmpz", {8,2}},
    {"copy", {9,3}},
    {"load", {10,2}},
    {"store", {11,2}},
    {"input", {12,2}},
    {"output", {13,2}},
    {"stop", {14,1}},
};

map<string, int> directives_table = {
    {"const", 1},
    {"space", 1}
};

map<string, int> symbol_table;

int error_flag = 0; // flag de erro, se 1, nao gera arquivo objeto


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
 * Identifica tipo de erro e exibe mensagem
 * Marca flag de erro
 */
void throw_error(int error, int line, string params[]) {
    string cause;
    string types[3] = {"lexico", "sintatico", "semantico"};
    int type;

    if(error == ERROR_INVALID_SYMBOL) {
        type = 0; // ok
        cause = "Rotulo invalido (" + params[0] + ")";
    } else if(error == ERROR_DUPLICATE_SYMBOL) {
        type = 2;
        cause = "Rotulo duplicado (" + params[0] + ")";
    } else if(error == ERROR_INEXISTENT_SYMBOL) {
        type = 2;
        cause = "Rotulo inexistente (" + params[0] + ")";
    } else if(error == ERROR_INVALID_INSTRUCTION) {
        type = 0;
        cause = "Instrucao ou diretiva invalida (" + params[0] + ")";
    } else if(error == ERROR_INEXISTENT_INSTRUCTION) {
        type = 1;
        cause = "Instrucao ou diretiva inexistente (" + params[0] + ")";
    } else if(error == ERROR_INVALID_OPERATOR_COUNT) {
        type = 1;
        cause = "Instrucao ou diretiva (" + params[0] + ") contem " + params[1] + " operandos (deve ser " + params[2] + ")";
    } else if(error == ERROR_INVALID_OPERATOR_TYPE) {
        type = 1;
        cause = "Instrucao ou diretiva (" + params[0] + ") com tipo de operando invalido";
    } else if(error == ERROR_INVALID_TOKEN) {
        type = 1;
        cause = "Token invalido (" + params[0] + ")";
    }

    cout << "Erro " << types[type] << " na linha " << line << ":" << endl;
    cout << "\t" << cause << endl;

    error_flag = 1; // evita geracao de codigo objeto
}

/**
 * Faz o pre-processamento de linhas do arquivo:
 * - remove comentarios
 * - transforma tudo em lowercase
 * - remove tabs e espacos duplicados
 */
string line_preprocessing(string line) {
    // remover comentario
    size_t comment = line.find(";");
    if(comment != string::npos)
        line.erase(line.begin() + comment, line.end());

    // remove tabs e transforma em lowercase
    for(auto &c : line) {
        if(c == '\t')
            c = ' ';
        else
            c = tolower(c);
    }

    // remove espacos duplicados
    for(int i=line.size()-1; i >= 0; i--) {
        if(line[i]==' ' && line[i]==line[i-1]) {
            line.erase( line.begin() + i );
        }
    }

    // remover espacos antes e depois
    line.erase(0, line.find_first_not_of(" "));
    line.erase(line.find_last_not_of(" ") + 1);

    return line;
}

/**
 * Funcao auxiliar para designar funcao para cada token
 */
string get_token_key(int number) {
    string token_keys[3] = {"label", "instruction", "params"};

    if(number > 2) {
        return "error";
    } else {
        return token_keys[number];
    }
}

/**
 * Faz a separacao de rotulos, operacoes e operandos para a primeira passagem
 */
map<string,string> tokenize(string line) {
    map<string, string> tokens;
    size_t start = 0, end = 0;

    int key_count = 1;

    // procura por rotulo
    size_t label_end = line.find(":");
    if(label_end != string::npos){
        string label = line.substr(0, label_end);

        // corrige espaco ao final da label (LABEL :)
        if(label[label.size() - 1] == ' ') {
            label = label.substr(0, label.size() - 1);
        }

        // garante letra/undescore no comeco, e sem caracteres especiais. maximo 50
        if( (label[0] >= '0' && label[0] <= '9') || (label.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890_") != std::string::npos) || label.size() > 50) {
            // erro rotulo invalido
            tokens["invalid_label_error"] = label;
        }

        if(directives_table.count(label) || instruction_table.count(label)) {
            tokens["invalid_token"] = label + " nao pode ser rotulo, pois eh reservado para instrucao";
        }

        tokens["label"] = label;
        start = label_end + 1;
    }

    while ((end = line.find(" ", start)) != string::npos) {
        string tmp = line.substr(start, end - start);
        if(tmp != "") {    

            // garante letra/undescore no comeco, e sem caracteres especiais. maximo 50
            if( (tmp[0] >= '0' && tmp[0] <= '9') || (tmp.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890_") != std::string::npos) || (tmp.size() > 50) ) {
                // erro instrucao invalida
                if(key_count == 1) // instrucao
                    tokens["invalid_instruction_error"] = tmp;
                else if(key_count >= 2)
                    tokens["error"] = tmp;
            }

            tokens[ get_token_key(key_count) ] = tmp;

            key_count++;
        }
        start = end + 1;
    }
    
    string tmp = line.substr(start);
    if (tmp != "") {
        tokens[ get_token_key(key_count) ] = tmp;
    }

    return tokens;
}

/******************************************************************
 * Realiza a primeira passagem: construcao da tabela de simbolos
 ******************************************************************/
void first_pass(char* file_path) {
    ifstream file(file_path);

    string line;
    int pos_counter = 0;
    int data_pos_counter = 0;
    int line_counter = 1;

    map<string,int> temp_symbol_table;

    int current_section = 0; // primeiro DATA, depois TEXT

    while(getline(file, line)) {
        // garante funcionamento em arquivos terminados com CRLF
        size_t windows_break = line.find(13);
        if(windows_break != string::npos)
            line.erase(line.begin() + windows_break, line.end());

        if(!line.empty()) {
            string processed_line = line_preprocessing(line);
            
            if(processed_line == "section data") { // define que labels sao dados
                current_section = 0;
                line_counter++;
                continue;
            } else if(processed_line == "section text") { // define de labels sao instrucoes
                current_section = 1;
                line_counter++;
                continue;
            }

            // separa elementos da linha
            map<string,string> tokens = tokenize(processed_line);
    
            if(tokens.count("invalid_label_error")) {
                string symb[1] = { tokens["invalid_label_error"] };
                throw_error(ERROR_INVALID_SYMBOL, line_counter, symb);
            }

            if(tokens.count("invalid_instruction_error")) {
                string symb[1] = { tokens["invalid_instruction_error"] };
                throw_error(ERROR_INVALID_INSTRUCTION, line_counter, symb);
            }

            if(tokens.count("invalid_token")) {
                string symb[1] = { tokens["invalid_token"] };
                throw_error(ERROR_INVALID_TOKEN, line_counter, symb);
            }

            if(tokens.count("label")) {
                if(symbol_table.count(tokens["label"]) || temp_symbol_table.count(tokens["label"])) {
                    string symb[1] = { tokens["label"] };
                    throw_error(ERROR_DUPLICATE_SYMBOL, line_counter, symb);
                } else {
                    if(current_section == 0) { // section data
                        temp_symbol_table[ tokens["label"] ] = data_pos_counter;
                    } else {
                        symbol_table[ tokens["label"] ] = pos_counter;
                    }
                }

            }

            if(tokens.count("instruction")) {
                if(instruction_table.count( tokens["instruction"] )) { // se existe instrucao
                    pos_counter = pos_counter + instruction_table[ tokens["instruction"] ].second;
                } else {
                    if(directives_table.count( tokens["instruction"] )) { // se existe diretiva
                        if(current_section == 0) {
                            data_pos_counter = data_pos_counter + directives_table[ tokens["instruction"] ];
                        } else {
                            pos_counter = pos_counter + directives_table[ tokens["instruction"] ];
                        }
                    } else {
                        string symb[1] = {tokens["instruction"]};
                        throw_error(ERROR_INEXISTENT_INSTRUCTION, line_counter, symb);
                    }
                }
            }
        }

        line_counter++;
    }

    // adiciona elementos do data na tabela de simbolos,
    // ja considerando que vai estar no final
    for(auto &i : temp_symbol_table) {
        symbol_table[ i.first ] = pos_counter + i.second;
    }

    file.close();
}


/**
 * Faz a separacao de rotulos, operacoes e operandos para a segunda passagem
 */
int line_elements(string line, int line_number, int pos_counter, vector<int> &obj_code, vector<int> &tmp_data) {
    size_t start = 0;
    string tmp;

    size_t label_end = line.find(":");
    if(label_end != string::npos)
        start = label_end + 1;

    // linha sem rotulo
    vector<string> elements = split(line.substr(start), " ");

    if(elements.size() > 2) { // nenhuma instrucao/diretiva tem mais de 1 espaco (ignorando rotulo)
        if(directives_table.count(elements[0]) || instruction_table.count(elements[0])) {
            string qtd_args;
            if(elements[0] == "const") qtd_args = "1";
            else if(elements[0] == "space") qtd_args = "0";
            else if(elements[0] == "copy") qtd_args = "2, separados por virgula";
            else qtd_args = to_string(instruction_table[ elements[0] ].second - 1);

            string symb[3] = { elements[0], to_string(elements.size() - 1), qtd_args };
            throw_error(ERROR_INVALID_OPERATOR_COUNT, line_number, symb);
        } else {
            string symb[1] = { elements[0] };
            throw_error(ERROR_INEXISTENT_INSTRUCTION, line_number, symb);
        }
    } else if(elements.size() > 0) {
        if(directives_table.count(elements[0])) { // se for diretiva
           if(elements.size() == 2 && elements[0] == "const") {
                // garante que const so aceita numeros positivos ou negativos
                if( (elements[1][0] == '-' || (elements[1][0] >= '0' && elements [1][0] <= '9')) &&
                    elements[1].substr(1).find_first_not_of("01234567890") == std::string::npos) {
                    tmp_data.push_back( atoi( elements[1].c_str() ) );
                } else {
                    string symb[1] = { elements[0] };
                    throw_error(ERROR_INVALID_OPERATOR_TYPE, line_number, symb);
                }
            } else if(elements.size() == 1 && elements[0] == "space") {
                tmp_data.push_back(0);
            } else {
                string qtd_args = (elements[0] == "const" ? "1" : "0");
                string symb[3] = { elements[0], to_string(elements.size() - 1), qtd_args };
                throw_error(ERROR_INVALID_OPERATOR_COUNT, line_number, symb);
            }
        } else if(instruction_table.count(elements[0])) { // se for instrucao
            if(elements.size() > 1) {
                vector<string> args = split(elements[1], ",");

                for(string arg : args) {
                    // garante tipo e quantidade de argumentos
                    if( (arg[0] >= '0' && arg[0] <= '9') || (arg.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890_") != std::string::npos) || arg.size() > 50) {
                        string symb[1] = { elements[0] };
                        throw_error(ERROR_INVALID_OPERATOR_TYPE, line_number, symb);
                    } else if(!symbol_table.count(arg)) {
                        string symb[1] = { arg };
                        throw_error(ERROR_INEXISTENT_SYMBOL, line_number, symb);
                    }
                }

                // verifica numero de operandos
                if(args.size() == instruction_table[ elements[0] ].second - 1) {
                    // gera codigo obj

                    obj_code.push_back( instruction_table[ elements[0] ].first ); // opcode

                    for(string arg : args) {
                        if(symbol_table.count(arg)) {
                            obj_code.push_back( symbol_table[arg] ); // operando
                        }
                    }

                    pos_counter = pos_counter + instruction_table[ elements[0] ].second;
                } else {
                    string symb[3] = { elements[0], to_string(args.size()), to_string( instruction_table[ elements[0] ].second - 1 ) };
                    throw_error(ERROR_INVALID_OPERATOR_COUNT, line_number, symb);
                }
            } else { // pro caso do stop que nao tem argumentos
                if((instruction_table[ elements[0] ].second - 1) == 0) {
                    // gera codigo obj
                    obj_code.push_back( instruction_table[ elements[0] ].first ); // opcode
                    pos_counter = pos_counter + instruction_table[ elements[0] ].second;
                } else { // caso de instrucao valida, mas sem argumentos
                    string symb[3] = { elements[0], "0", to_string( instruction_table[ elements[0] ].second - 1 ) };
                    throw_error(ERROR_INVALID_OPERATOR_COUNT, line_number, symb);
                }
            }
        } else {
            string symb[1] = { elements[0] };
            throw_error(ERROR_INEXISTENT_INSTRUCTION, line_number, symb);
        }
    }

    return pos_counter;
}

/******************************************************************
 * Realiza a segunda passagem: montagem
 ******************************************************************/
vector<int> second_pass(char* file_path) {
    ifstream file(file_path);

    string line;
    int pos_counter = 0;
    int line_counter = 1;

    vector<int> obj_code; // salva o resultado final de codigo objeto
    vector<int> tmp_data; // salva o data, que vai pro final

    while(getline(file, line)) {
        size_t windows_break = line.find(13);
        if(windows_break != string::npos)
            line.erase(line.begin() + windows_break, line.end());

        string processed_line = line_preprocessing(line);
        if(!processed_line.empty() && processed_line != "section data" && processed_line != "section text") {
            pos_counter = line_elements(processed_line, line_counter, pos_counter, obj_code, tmp_data);
        }
        line_counter++;
    }

    // adiciona secao de dados ao final do arquivo objeto
    for(auto &i : tmp_data) {
        obj_code.push_back(i);
    }

    file.close();

    return obj_code;
}

int main(int argc, char* argv[]) {
    if(argc < 2) {
        cout << "Execute o programa especificando qual arquivo sera utilizado: ./montador programa.asm" << endl;
        return 0;
    }

    char* file_path = argv[1];

    // separa nome do arquivo
    string file_name = file_path;
    size_t endf = file_name.find(".asm");
    if(endf != string::npos)
        file_name.erase(file_name.begin() + endf, file_name.end());

    // primeira e segunda passagens
    first_pass(file_path);
    vector<int> obj_code = second_pass(file_path);

    // se nao houver erros, escreve o arquivo objeto
    if(!error_flag) {
        ofstream binary;
        binary.open(file_name + ".obj");

        for(int &w : obj_code) {
            binary << w << " ";
        }

        binary.close();
    }

    return 0;
}