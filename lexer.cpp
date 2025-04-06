#include <bits/stdc++.h>
#include "symbol_table.cpp"
#include "literal_table.cpp"

using namespace std;

struct Token {
    string type;
    string value;
    int line;
};

class Lexer {
private:
    static string trim(const string& s);
    static int countLeadingSpaces(const string& s);
    static string removeComments(const string& line);
    static string classifyToken(const string& tokenStr);
    static vector<Token> tokenizeLineContent(const string& line, int lineNumber);

public:
    static vector<Token> analyze(const string& fileName, SymbolTable& symbolTable, LiteralTable& literalTable);
};

// Implementations
string Lexer::trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    return (start == string::npos) ? "" : s.substr(start, end - start + 1);
}

int Lexer::countLeadingSpaces(const string& s) {
    int count = 0;
    for (char c : s) {
        if (c == ' ')
            count++;
        else if (c == '\t')
            count += 4;
        else
            break;
    }
    return count;
}

string Lexer::removeComments(const string& line) {
    size_t pos = line.find('#');
    return (pos != string::npos) ? line.substr(0, pos) : line;
}

string Lexer::classifyToken(const string& tokenStr) {
    static regex strPattern(R"((\"[^\"]*\"|'[^']*'))");
    static regex floatPattern(R"(\b\d+\.\d+\b)");
    static regex intPattern(R"(\b\d+\b)");
    static regex boolPattern(R"(\b(True|False)\b)");
    static regex keywordPattern(R"(\b(if|else|elif|print|input|int|float|bool|str|list)\b)");
    static regex operatorPattern(R"((\+|\-|\*|\/|==|!=|<=|>=|<|>|=|&|\||\^|~|\b(and|or|not)\b))");
    static regex separatorPattern(R"([(){}:,\[\]])");
    static regex identifierPattern(R"(\b[a-zA-Z_][a-zA-Z0-9_]*\b)");

    if (regex_match(tokenStr, keywordPattern)) return "KEYWORD";
    if (regex_match(tokenStr, operatorPattern)) return "OPERATOR";
    if (regex_match(tokenStr, separatorPattern)) return "SEPARATOR";
    if (regex_match(tokenStr, boolPattern)) return "BOOLEAN";
    if (regex_match(tokenStr, strPattern)) return "STRING";
    if (regex_match(tokenStr, floatPattern)) return "FLOAT";
    if (regex_match(tokenStr, intPattern)) return "INTEGER";
    if (regex_match(tokenStr, identifierPattern)) return "IDENTIFIER";
    return "UNKNOWN";
}

vector<Token> Lexer::tokenizeLineContent(const string& line, int lineNumber) {
    static regex token_regex(R"((\"[^\"]*\"|'[^']*')|\b\d+\.\d+\b|\b\d+\b|\b(True|False)\b|\b(if|else|elif|print|input|int|float|bool|str|list)\b|(\+|\-|\*|\/|==|!=|<=|>=|<|>|=|&|\||\^|~|\b(and|or|not)\b)|[(){}:,\[\]]|\b[a-zA-Z_][a-zA-Z0-9_]*\b)");

    vector<Token> tokens;
    auto begin = sregex_iterator(line.begin(), line.end(), token_regex);
    auto end = sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        string tokenStr = it->str();
        if (trim(tokenStr).empty())
            continue;
        tokens.push_back({ classifyToken(tokenStr), tokenStr, lineNumber });
    }
    return tokens;
}

vector<Token> Lexer::analyze(const string& fileName, SymbolTable& symbolTable, LiteralTable& literalTable) {
    cout << "Lexical Analysis Started\n";
    ifstream infile(fileName);
    if (!infile) {
        cerr << "Error: Unable to open file " << fileName << endl;
        exit(1);
    }

    ofstream lexReport("lexical_analysis.txt");
    vector<Token> allTokens;
    stack<int> indentStack;
    indentStack.push(0);

    string line;
    int lineNumber = 1;
    bool hasUnknown = false;

    while (getline(infile, line)) {
        string noCommentLine = removeComments(line);
        int currentIndent = countLeadingSpaces(noCommentLine);
        string trimmed = trim(noCommentLine);

        if (trimmed.empty()) {
            lineNumber++;
            continue;
        }

        // Handle INDENT / DEDENT
        if (currentIndent > indentStack.top()) {
            indentStack.push(currentIndent);
            allTokens.push_back({ "INDENT", "", lineNumber });
            lexReport << lineNumber << setw(15) << "" << setw(15) << "INDENT" << endl;
        }
        else {
            while (currentIndent < indentStack.top()) {
                indentStack.pop();
                allTokens.push_back({ "DEDENT", "", lineNumber });
                lexReport << lineNumber << setw(15) << "" << setw(15) << "DEDENT" << endl;
            }
            if (currentIndent != indentStack.top()) {
                cerr << "Indentation Error at line " << lineNumber << endl;
                exit(1);
            }
        }

        // Tokenize line
        vector<Token> tokens = tokenizeLineContent(noCommentLine, lineNumber);
        for (auto& tok : tokens) {
            allTokens.push_back(tok);
            lexReport << tok.line << setw(15) << tok.value << setw(15) << tok.type << endl;

            if (tok.type == "IDENTIFIER") {
                symbolTable.addSymbol(tok.value);
            }
            else if (tok.type == "STRING" || tok.type == "INTEGER" || tok.type == "FLOAT" || tok.type == "BOOLEAN") {
                literalTable.addLiteral(tok.value, tok.type);
            }
            else if (tok.type == "UNKNOWN") {
                cerr << "Lexical Error: " << tok.value << " at line " << tok.line << endl;
                hasUnknown = true;
            }
        }

        // Add NEWLINE token
        allTokens.push_back({ "NEWLINE", "", lineNumber });
        lexReport << lineNumber << setw(15) << "" << setw(15) << "NEWLINE" << endl;

        lineNumber++;
    }

    // Handle remaining DEDENTs
    while (indentStack.size() > 1) {
        indentStack.pop();
        allTokens.push_back({ "DEDENT", "", lineNumber });
        lexReport << lineNumber << setw(15) << "" << setw(15) << "DEDENT" << endl;
    }

    allTokens.push_back({ "$", "$", lineNumber });

    // Write symbol table
    ofstream symbolFile("symbol_table.txt");
    if (symbolFile) {
        streambuf* coutBuf = cout.rdbuf();
        cout.rdbuf(symbolFile.rdbuf());
        symbolTable.print();
        cout.rdbuf(coutBuf);
    }

    // Write literal table
    ofstream literalFile("literal_table.txt");
    if (literalFile) {
        streambuf* coutBuf = cout.rdbuf();
        cout.rdbuf(literalFile.rdbuf());
        literalTable.print();
        cout.rdbuf(coutBuf);
    }

    infile.close();
    lexReport.close();

    if (hasUnknown) exit(1);

    cout << "Lexical Analysis Completed\n";
    return allTokens;
}

#ifdef DEBUG_LEXER
int main(int argc, char* argv[]) {
    string fileName = "test.py";

    SymbolTable symbolTable;
    LiteralTable literalTable;
    Lexer lexer;

    vector<Token> tokens = lexer.analyze(fileName, symbolTable, literalTable);

    cout << "Tokens:\n";
    for (const auto& token : tokens) {
        cout << "Line " << token.line << ": " << token.value << " --> " << token.type << endl;
    }

    symbolTable.print();
    literalTable.print();

    return 0;
}
#endif