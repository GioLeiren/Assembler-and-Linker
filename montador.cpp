#include "montador.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>

// Tabelas globais
std::map<std::string, int> symbolTable;
std::vector<std::string> programLines;
std::map<std::string, std::pair<int, std::vector<std::string>>> MNT; // Nome -> <Posição na MDT, Nº de argumentos>
std::vector<std::vector<std::string>> MDT;     // Corpo das macros, armazenado linha por linha

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Uso: ./montador -o input.asm (linux) ou montador.exe -o input.asm (windows)\n";
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

std::vector<std::string> splitString(const std::string& input) {
    std::vector<std::string> result;
    std::stringstream ss(input);
    std::string item;

    while (std::getline(ss, item, ',')) {
        result.push_back(item);
    }

    return result;
}

void preprocess(const std::string& inputFile, const std::string& outputFile) {
    std::ifstream input(inputFile);
    std::ofstream output(outputFile);
    
    if (!input.is_open()) {
        std::cerr << "Error opening input file: " << inputFile << "\n";
        return;
    }

    if (!output.is_open()) {
        std::cerr << "Error creating output file: " << outputFile << "\n";
        return;
    }

    std::string line;
    bool inMacroDefinition = false;
    std::string currentMacroName;
    std::vector<std::string> currentMacroArgs;
    int mdtIndex = 0;

    while (std::getline(input, line)) {
        // Remove comments
        size_t commentPos = line.find(';');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        // Trim leading and trailing whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        // Skip empty lines
        if (line.empty()) continue;

        // Check for macro definition start
        if (line.find("MACRO") != std::string::npos && 
            line.find("ENDMACRO") == std::string::npos) {
            // Parse macro name and arguments
            size_t macroPos = line.find(':');
            if (macroPos != std::string::npos) {
                currentMacroName = line.substr(0, macroPos);
                
                // Extract arguments
                std::string argsStr = line.substr(macroPos + 7); // After " MACRO"
                std::istringstream argStream(argsStr);
                std::string arg;
                currentMacroArgs.clear();

                while (std::getline(argStream, arg, ',')) {
                    // Trim whitespace from argument
                    arg.erase(0, arg.find_first_not_of(" \t"));
                    arg.erase(arg.find_last_not_of(" \t") + 1);
                    if (!arg.empty()) {
                        currentMacroArgs.push_back(arg);
                    }
                }

                // Store macro info in MNT
                MNT[currentMacroName] = {mdtIndex, currentMacroArgs};
                inMacroDefinition = true;
                continue;
            }
        }

        // Store macro body in MDT
        if (inMacroDefinition) {
            // Check for macro definition end
            if (line.find("ENDMACRO") != std::string::npos) {
                inMacroDefinition = false;
                continue;
            }

            // Replace formal arguments with placeholders
            std::vector<std::string> processedLine;
            std::istringstream iss(line);
            std::string token;
            std::vector<std::string> lineTokens;

            // First, collect all tokens
            while (iss >> token) {
                size_t tokenComma = token.find(',');
                token = token.substr(0, tokenComma);

                lineTokens.push_back(token);
            }

            // Process each token
            for (const auto& tok : lineTokens) {
                bool replaced = false;
                for (size_t i = 0; i < currentMacroArgs.size(); ++i) {
                    if (tok == currentMacroArgs[i]) {
                        processedLine.push_back("#" + std::to_string(i + 1));
                        replaced = true;
                        break;
                    }
                }
                
                if (!replaced) {
                    processedLine.push_back(tok);
                }
            }

            MDT.push_back(processedLine);
            mdtIndex++;
            continue;
        }

        // Check for macro invocation
        bool macroExpanded = false;
        for (const auto& [macroName, macroInfo] : MNT) {
            if (line.find(macroName) != std::string::npos) {
                // Extract macro arguments
                std::vector<std::string> actualArgs;
                size_t macroPos = line.find(macroName);
                std::string argsStr = line.substr(macroPos + macroName.length());
                std::istringstream argStream(argsStr);
                std::string arg;

                while (std::getline(argStream, arg, ',')) {
                    // Trim whitespace from argument
                    arg.erase(0, arg.find_first_not_of(" \t"));
                    arg.erase(arg.find_last_not_of(" \t") + 1);
                    if (!arg.empty()) {
                        actualArgs.push_back(arg);
                    }
                }

                // Verify argument count
                if (actualArgs.size() != macroInfo.second.size()) {
                    std::cerr << "Error: Incorrect number of arguments for macro " << macroName << "\n";
                    continue;
                }

                // Expand macro
                int macroStart = macroInfo.first;
                for (int i = macroStart; i < (int)MDT.size(); ++i) {
                    std::ostringstream expandedLine;
                    for (const auto& token : MDT[i]) {
                        if (token[0] == '#' && std::isdigit(token[1])) {
                            int argIndex = std::stoi(token.substr(1)) - 1;
                            expandedLine << actualArgs[argIndex] << " ";
                        } else {
                            expandedLine << token << " ";
                        }
                    }
                    line = expandedLine.str();
                    line.erase(remove_if(line.begin(), line.end(), isspace), line.end());
                    output << line << "\n";
                }

                macroExpanded = true;
                break;
            }
        }

        // Write non-macro lines to output
        if (!macroExpanded && !inMacroDefinition) {
            line.erase(remove_if(line.begin(), line.end(), isspace), line.end());
            output << line << "\n";
        }
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
        if (line.find("SPACE") != std::string::npos) {
            size_t spacePos = line.find("SPACE");
            std::string spaceAlloc = line.substr(spacePos + 5);
            std::cerr << "spaceAlloc: " << spaceAlloc << "\n";
            locationCounter += stoi(spaceAlloc);
        }
        else if (line.find("STOP") != std::string::npos) {
            locationCounter++;
        }
        else if (line.find("CONST") != std::string::npos) {
            locationCounter++;
        }
        else if (line.find("INPUT") != std::string::npos) {
            locationCounter += 2;
        }
        else if (line.find("OUTPUT") != std::string::npos) {
            locationCounter += 2;
        }
        else if (line.find("LOAD") != std::string::npos) {
            locationCounter += 2;
        }
        else if (line.find("STORE") != std::string::npos) {
            locationCounter += 2;
        }
        else if (line.find("ADD") != std::string::npos) {
            locationCounter += 2;
        }
        else if (line.find("SUB") != std::string::npos) {
            locationCounter += 2;
        }
        else if (line.find("MUL") != std::string::npos) {
            locationCounter += 2;
        }
        else if (line.find("DIV") != std::string::npos) {
            locationCounter += 2;
        }
        else if (line.find("JMP") != std::string::npos) {
            locationCounter += 2;
        }
        else if (line.find("JMPN") != std::string::npos) {
            locationCounter += 2;
        }
        else if (line.find("JMPP") != std::string::npos) {
            locationCounter += 2;
        }
        else if (line.find("JMPZ") != std::string::npos) {
            locationCounter += 2;
        }
        else if (line.find("COPY") != std::string::npos) {
            locationCounter += 3;
        }
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
