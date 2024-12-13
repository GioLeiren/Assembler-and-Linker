Giovanni Minari Zanetti - 202014280
Windows

compilar: g++ montador.cpp -o montador

Rodar:
Windows: montador.exe -p arquivo.asm (pré-processar)
         montador.exe -o output.pre (montar)

Linux:   ./montador -p arquivo.asm (pré-processar)
         ./montador -o output.pre (montar)

Resumo atual: Montador recebe um arquivo .asm de um assembly inventado*, pré-processa ele exandindo as chamadas de macro e removendo comentários, espaços e linhas em branco, retornando um arquivo output.pre. Montador recebe um arquivo .pre e monta a tabela de símbolos declarados na primeira passagem, e na segunda passagem substitui as referências a esses símbolos pelo seu endereço onde são declaradas, e no final retorna um arquivo output.obj contendo o código de máquina do arquivo assembly (opcodes das instruções e endereços dos símbolos)

*No caso, cada instrução e variável ocupam exatamente 1 espaço de memória cada um e também cada instrução gera exatamente 1 instrução de máquina