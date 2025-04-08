#include "parser.cpp"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: ./simple_python_compiler <filename.py>" << endl;
        return 1;
    }
    string fileName = argv[1];
    if (fileName.size() < 3 || fileName.substr(fileName.size() - 3) != ".py") {
        cerr << "Error: Only .py files are allowed." << endl;
        return 1;
    }
    SymbolTable symbolTable;
    LiteralTable literalTable;
    Lexer lexer;
    vector<Token> tokens = lexer.analyze(fileName, symbolTable, literalTable);
    SLR1Parser parser("grammar.json");
    parser.parse(tokens);
    return 0;
}