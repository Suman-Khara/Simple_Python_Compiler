#include <bits/stdc++.h>
using namespace std;

// -----------------------------------------------------------------------------
// Token Structure
// -----------------------------------------------------------------------------
struct Token {
    string type;
    string value;
    int line;
};

// -----------------------------------------------------------------------------
// Regular Expressions for Different Token Types
// -----------------------------------------------------------------------------

// Patterns for each token type.
string strPattern = R"((\"[^\"]*\"|'[^']*'))";         // String literal: everything inside double or single quotes.
string floatPattern = R"(\b\d+\.\d+\b)";                // Float literal.
string intPattern = R"(\b\d+\b)";                        // Integer literal.
string boolPattern = R"(\b(True|False)\b)";              // Boolean literal.
string keywordPattern = R"(\b(if|else|elif|print|input|int|float|bool|str|list)\b)";
string operatorPattern = R"((\+|\-|\*|\/|==|!=|<=|>=|<|>|=|&|\||\^|~|\b(and|or|not)\b))";
string separatorPattern = R"([(){}:,\\[\]])";
string identifierPattern = R"(\b[a-zA-Z_][a-zA-Z0-9_]*\b)";

// Build a combined pattern that tries the alternatives in order:
// Note: The order matters – string literals first, then numbers, booleans, keywords, operators, separators, identifiers.
string combinedPattern = strPattern + "|" +
floatPattern + "|" +
intPattern + "|" +
boolPattern + "|" +
keywordPattern + "|" +
operatorPattern + "|" +
separatorPattern + "|" +
identifierPattern;

regex token_regex(combinedPattern);

// -----------------------------------------------------------------------------
// Helper Functions
// -----------------------------------------------------------------------------

// Trim whitespace from both ends.
string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    return (start == string::npos) ? "" : s.substr(start, end - start + 1);
}

// Count the number of leading spaces.
int countLeadingSpaces(const string& s) {
    int count = 0;
    for (char c : s) {
        if (c == ' ')
            count++;
        else if (c == '\t')
            count += 4; // Assume tab = 4 spaces.
        else
            break;
    }
    return count;
}

// Remove comments (assume '#' starts a comment outside of string literals).
string removeComments(const string& line) {
    size_t pos = line.find('#');
    if (pos != string::npos)
        return line.substr(0, pos);
    return line;
}

// Determine the token type based on the token value.
string classifyToken(const string& tokenStr) {
    if (regex_match(tokenStr, regex(keywordPattern)))
        return "KEYWORD";
    if (regex_match(tokenStr, regex(operatorPattern)))
        return "OPERATOR";
    if (regex_match(tokenStr, regex(separatorPattern)))
        return "SEPARATOR";
    if (regex_match(tokenStr, regex(boolPattern)))
        return "BOOLEAN";
    if (regex_match(tokenStr, regex(strPattern)))
        return "STRING";
    if (regex_match(tokenStr, regex(floatPattern)))
        return "FLOAT";
    if (regex_match(tokenStr, regex(intPattern)))
        return "INTEGER";
    if (regex_match(tokenStr, regex(identifierPattern)))
        return "IDENTIFIER";
    return "UNKNOWN";
}

// Tokenize the content of a line using regex_iterator.
vector<Token> tokenizeLineContent(const string& line, int lineNumber) {
    vector<Token> tokens;
    auto begin = sregex_iterator(line.begin(), line.end(), token_regex);
    auto end = sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        string tokenStr = it->str();
        // Skip pure whitespace matches (should not happen due to our pattern).
        if (trim(tokenStr).empty())
            continue;
        Token tok;
        tok.value = tokenStr;
        tok.line = lineNumber;
        tok.type = classifyToken(tokenStr);
        tokens.push_back(tok);
    }
    return tokens;
}

// -----------------------------------------------------------------------------
// analyzeFile Function
// Performs lexical analysis (skipping comments), writes a table to lexical_analysis.txt,
// and returns a map from symbol names (from assignments) to their types.
// -----------------------------------------------------------------------------
unordered_map<string, string> analyzeFile(const string& fileName) {
    ifstream infile(fileName);
    if (!infile) {
        cerr << "Error: Unable to open file " << fileName << endl;
        exit(1);
    }

    ofstream lexReport("lexical_analysis.txt");
    if (!lexReport) {
        cerr << "Error: Unable to open lexical_analysis.txt for writing." << endl;
        exit(1);
    }

    // Write header.
    lexReport << left << setw(15) << "Symbol"
        << left << setw(15) << "Type"
        << left << setw(10) << "Line" << "\n";
    lexReport << string(40, '-') << "\n";

    vector<Token> allTokens;
    unordered_map<string, string> symbolMap;
    stack<int> indentStack;
    indentStack.push(0);

    string line;
    int lineNumber = 1;
    while (getline(infile, line)) {
        // Remove comments.
        string noCommentLine = removeComments(line);
        if (trim(noCommentLine).empty()) {
            lineNumber++;
            continue;
        }

        int currentIndent = countLeadingSpaces(noCommentLine);
        string trimmedLine = trim(noCommentLine);

        // Handle INDENT/DEDENT.
        int prevIndent = indentStack.top();
        if (currentIndent > prevIndent) {
            Token indentToken{ "INDENT", "", lineNumber };
            allTokens.push_back(indentToken);
            lexReport << left << setw(15) << "INDENT"
                << left << setw(15) << " "
                << left << setw(10) << lineNumber << "\n";
            indentStack.push(currentIndent);
        }
        else if (currentIndent < prevIndent) {
            while (!indentStack.empty() && currentIndent < indentStack.top()) {
                indentStack.pop();
                Token dedentToken{ "DEDENT", "", lineNumber };
                allTokens.push_back(dedentToken);
                lexReport << left << setw(15) << "DEDENT"
                    << left << setw(15) << " "
                    << left << setw(10) << lineNumber << "\n";
            }
            if (indentStack.empty() || currentIndent != indentStack.top()) {
                cerr << "Indentation Error at line " << lineNumber << endl;
                exit(1);
            }
        }

        // Tokenize the line.
        vector<Token> tokens = tokenizeLineContent(trimmedLine, lineNumber);
        for (auto& tok : tokens) {
            allTokens.push_back(tok);
            lexReport << left << setw(15) << tok.value
                << left << setw(15) << tok.type
                << left << setw(10) << tok.line << "\n";
        }

        // Emit NEWLINE token.
        Token newlineToken{ "NEWLINE", "", lineNumber };
        allTokens.push_back(newlineToken);
        lexReport << left << setw(15) << "NEWLINE"
            << left << setw(15) << " "
            << left << setw(10) << lineNumber << "\n";

        // --- Simple Assignment Detection ---
        // If the line is an assignment, assume pattern: IDENTIFIER, "=" OPERATOR, then a literal.
        if (tokens.size() >= 3) {
            if (tokens[1].type == "OPERATOR" && tokens[1].value == "=") {
                string id = tokens[0].value;
                string literalType = tokens[2].type;
                symbolMap[id] = literalType;
            }
        }
        // ---
        lineNumber++;
    }

    // End-of-file: Emit remaining DEDENT tokens.
    while (indentStack.top() > 0) {
        indentStack.pop();
        Token dedentToken{ "DEDENT", "", lineNumber };
        allTokens.push_back(dedentToken);
        lexReport << left << setw(15) << "DEDENT"
            << left << setw(15) << " "
            << left << setw(10) << lineNumber << "\n";
    }

    infile.close();
    lexReport.close();

    // For debugging, print tokens.
    for (const Token& tok : allTokens) {
        cout << "Line " << tok.line << ": " << tok.value << " --> " << tok.type << "\n";
    }

    return symbolMap;
}

// -----------------------------------------------------------------------------
// Main Function
// -----------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Usage: lexer <source_file.py>" << endl;
        return 1;
    }

    unordered_map<string, string> symbolTypes = analyzeFile(argv[1]);

    // Print the symbol table.
    cout << "\nSymbol Table:" << endl;
    for (const auto& entry : symbolTypes) {
        cout << entry.first << " : " << entry.second << endl;
    }

    return 0;
}