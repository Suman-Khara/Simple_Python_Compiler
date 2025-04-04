#include "parser.cpp"

int main() {
    string fileName = "test.py";

    SymbolTable symbolTable;
    LiteralTable literalTable;
    Lexer lexer;
    vector<Token> tokens = lexer.analyze(fileName, symbolTable, literalTable);
    SLR1Parser parser("grammar.json");
    parser.parse(tokens);
    cout << "Lexical and Syntax Analysis completed successfully.\n";
}