#ifndef MONTADOR_HPP
#define MONTADOR_HPP

#include <string>
#include <map>
#include <vector>

// Declaração de funções principais
bool isValidNumber(const std::string& str);
std::vector<std::string> splitString(const std::string& input);
void preprocess(const std::string& inputFile, const std::string& outputFile);
void firstPass(const std::string& inputFile);
std::string translateToMachineCode(const std::string& line);
std::string replaceWithAddress(const std::string& line);
void secondPass(const std::string& outputFile);

#endif // MONTADOR_HPP