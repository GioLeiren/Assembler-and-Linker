#include "montador.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>

// Tabelas globais
std::map<std::string, int> symbolTable;
std::vector<std::string> programLines;

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Uso: ./montador -o input.asm\n";
        return 1;
    }

    std::string mode = argv[1];
    std::string inputFile = argv[2];

    if (mode == "-p") {
        if (inputFile.substr(inputFile.length() - 3) != "asm"){
            std::cerr << "Arquivo precisa ser .asm!\n";
            return 1;
        }
        preprocess(inputFile, "output.pre");
    }
    else if (mode == "-o") {
        if (inputFile.substr(inputFile.length() - 3) != "pre"){
            std::cerr << "Arquivo precisa ser .pre!\n";
            return 1;
        }
        firstPass(inputFile);
        secondPass("output.obj");
    } else {
        std::cerr << "Modo inválido.\n";
        return 1;
    }

    return 0;
}

bool isValidNumber(const std::string& str) {
    if (str.empty()) return false; // String vazia não é válida
    size_t start = (str[0] == '-' || str[0] == '+') ? 1 : 0; // Permite sinais
    return std::all_of(str.begin() + start, str.end(), ::isdigit);
}

void preprocess(const std::string& inputFile, const std::string& outputFile) {
    std::ifstream input(inputFile);
    std::ofstream output(outputFile);

    if (!input.is_open()) {
        std::cerr << "Erro ao abrir o arquivo de entrada: " << inputFile << "\n";
        return;
    }

    if (!output.is_open()) {
        std::cerr << "Erro ao criar o arquivo de saída: " << outputFile << "\n";
        return;
    }

    std::map<std::string, int> equTable; // Para armazenar rótulos definidos por EQU
    std::string line;
    bool ignoreLine = false;

    while (std::getline(input, line)) {
        // Remover comentários (; e tudo depois dele)
        size_t commentPos = line.find(';');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        // Normalizar espaços e tabulações
        line.erase(remove_if(line.begin(), line.end(), isspace), line.end());

        // Ignorar linhas vazias após limpeza
        if (line.empty()) continue;

        // Processar diretivas EQU
        if (line.find("EQU") != std::string::npos) {
            size_t labelEnd = line.find(":");
            size_t directiveEnd = line.find("EQU");
            std::cerr << "labelEnd: " << labelEnd << "\n";
            std::string label = line.substr(0, labelEnd);
            std::cerr << "label: " << label << "\n";
            std::string value = line.substr(directiveEnd + 3);
            std::cerr << "value " << value << "\n";

            // Validar o valor antes de tentar convertê-lo
            if (!isValidNumber(value)) {
                std::cerr << "Erro: Valor inválido na diretiva EQU: " << value << "\n";
                return;
            }

            equTable[label] = stoi(value); // Converte valor para inteiro
            continue;
        }

        // Substituir rótulos definidos por EQU
        for (const auto& [label, value] : equTable) {
            size_t pos;
            while ((pos = line.find(label)) != std::string::npos) {
                line.replace(pos, label.length(), std::to_string(value));
            }
        }

        // Processar diretivas IF
        if (line.find("IF") != std::string::npos) {
            size_t ifPos = line.find("IF");
            std::string condition = line.substr(ifPos + 2);
            std::cerr << "condition: " << condition << "\n";
            ignoreLine = (stoi(condition) == 0);
            std::cerr << "ignoreLine: " << ignoreLine << "\n";
            
            continue;
        }

        // Ignorar linhas condicionais não atendidas
        if (ignoreLine) {
            ignoreLine = false;
            continue;
        }

        // Escrever a linha pré-processada no arquivo de saída
        output << line << "\n";
    }

    input.close();
    output.close();
}

void firstPass(const std::string& inputFile) {
    std::ifstream file(inputFile);
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir o arquivo de entrada: " << inputFile << "\n";
        return;
    }

    int locationCounter = 0; // Contador de localização para rastrear endereços
    std::string line;

    while (std::getline(file, line)) {
        // Ignorar linhas vazias
        if (line.empty()) continue;

        // Verificar se a linha contém um rótulo
        size_t labelPos = line.find(':');
        if (labelPos != std::string::npos) {
            std::string label = line.substr(0, labelPos); // Extrair o rótulo
            label.erase(remove_if(label.begin(), label.end(), isspace), label.end()); // Remover espaços extras

            // Verificar se o rótulo já existe
            if (symbolTable.find(label) != symbolTable.end()) {
                std::cerr << "Erro: Rótulo '" << label << "' redefinido na linha: " << line << "\n";
                return;
            }

            // Adicionar o rótulo à tabela de símbolos
            symbolTable[label] = locationCounter;

            // Remover o rótulo da linha para continuar o processamento
            line = line.substr(labelPos + 1);
        }

        // Adicionar a linha processada à lista de linhas do programa
        programLines.push_back(line);

        // Atualizar o contador de localização
        // Assumimos que cada instrução ou diretiva ocupa 1 unidade de memória (ajuste conforme necessário)
        locationCounter++;
    }

    file.close();

    // Para depuração: imprimir a tabela de símbolos
    std::cout << "Tabela de Símbolos:\n";
    for (const auto& [label, address] : symbolTable) {
        std::cout << label << " -> " << address << "\n";
    }
}

std::string translateToMachineCode(const std::string& line) {
    // Implementação simplificada para traduzir linhas para código de máquina
    if (line.find("MOV") != std::string::npos) {
        return "01";
    } else if (line.find("ADD") != std::string::npos) {
        return "02";
    } else if (line.find("SUB") != std::string::npos) {
        return "03";
    } else if (line.find("JNZ") != std::string::npos) {
        return "04";
    }
    return "";
}

void secondPass(const std::string& outputFile) {
    std::ofstream file(outputFile);
    if (!file.is_open()) {
        std::cerr << "Erro ao criar o arquivo de saída: " << outputFile << "\n";
        return;
    }

    for (const auto& line : programLines) {
        std::string processedLine = line; // Copiar a linha para processar
        std::string outputCode;          // Código gerado para a linha

        // Substituir rótulos pelos endereços na tabela de símbolos
        for (const auto& [label, address] : symbolTable) {
            size_t pos;
            while ((pos = processedLine.find(label)) != std::string::npos) {
                processedLine.replace(pos, label.length(), std::to_string(address));
            }
        }

        // Gerar código de máquina para a linha
        // Exemplo simplificado: aqui você deve implementar a lógica real de tradução
        outputCode = translateToMachineCode(processedLine);

        // Escrever o código gerado no arquivo
        if (!outputCode.empty()) {
            file << outputCode << " ";
        }
    }

    file.close();
    std::cout << "Arquivo de saída gerado: " << outputFile << "\n";
}
