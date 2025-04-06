#include<bits/stdc++.h>
#include "json.hpp"
#include "lexer.cpp"

using namespace std;
using json = nlohmann::json;

class CFG {
public:
    unordered_set<string> terminals;
    unordered_set<string> nonTerminals;
    unordered_map<string, vector<vector<string>>> productions;
    unordered_map<string, unordered_set<string>> first;
    unordered_map<string, unordered_set<string>> follow;
    string startSymbol;
    vector<pair<string, vector<string>>> numberedProductions;
    unordered_set<string> nullable;

    CFG(const string& filename) {
        //cout << "Loading grammar from " << filename << endl;
        ifstream file(filename);
        json grammar;
        file >> grammar;
        //cout << "Grammar loaded successfully\n";
        terminals = unordered_set<string>(grammar["terminals"].begin(), grammar["terminals"].end());
        nonTerminals = unordered_set<string>(grammar["nonTerminals"].begin(), grammar["nonTerminals"].end());
        ofstream out("rules.txt");
        for (const auto& rule : grammar["productions"].items()) {
            string lhs = rule.key();
            vector<vector<string>> rhsList = rule.value();
            for (const auto& rhs : rhsList) {
                productions[lhs].push_back(rhs);
                numberedProductions.emplace_back(lhs, rhs);
                out << numberedProductions.size() - 1 << " : ";
                out << "Production: " << lhs << " -> ";
                for (const auto& symbol : rhs) {
                    out << symbol << " ";
                }
                out << endl;
            }
        }

        startSymbol = grammar["startSymbol"];
        computeFirstSets();
        computeFollowSets();
        writeFirstToFile();    // writes first.txt
        writeFollowToFile();   // writes follow.txt
    }
    void writeFirstToFile(const string& filename = "first.txt") const {
        ofstream out(filename);
        for (const auto& nt : nonTerminals) {
            out << nt << " : ";
            for (const auto& sym : first.at(nt)) {
                out << sym << " ";
            }
            out << "\n";
        }
        out.close();
    }

    void writeFollowToFile(const string& filename = "follow.txt") const {
        ofstream out(filename);
        for (const auto& nt : nonTerminals) {
            out << nt << " : ";
            for (const auto& sym : follow.at(nt)) {
                out << sym << " ";
            }
            out << "\n";
        }
        out.close();
    }

    pair<string, vector<string>> getProductionByIndex(int index) const {
        return numberedProductions[index];
    }

    int getProductionIndex(const string& lhs, const vector<string>& rhs) const {
        for (int i = 0; i < numberedProductions.size(); ++i) {
            if (numberedProductions[i].first == lhs && numberedProductions[i].second == rhs)
                return i;
        }
        return -1;
    }

private:
    void computeFirstSets() {
        for (const string& terminal : terminals)
            first[terminal] = { terminal };

        for (const string& nonTerminal : nonTerminals)
            first[nonTerminal] = {};

        bool changed;
        do {
            changed = false;
            for (const string& A : nonTerminals) {
                for (const auto& production : productions[A]) {
                    bool allNullable = true;
                    size_t oldSize = first[A].size();

                    for (const string& symbol : production) {
                        for (const string& val : first[symbol]) {
                            if (val != "")
                                first[A].insert(val);
                        }

                        if (!first[symbol].count("")) {
                            allNullable = false;
                            break;
                        }
                    }

                    if (allNullable) {
                        if (!first[A].count("")) {
                            first[A].insert("");
                            changed = true;
                        }
                        nullable.insert(A);
                    }

                    if (oldSize < first[A].size())
                        changed = true;
                }
            }
        } while (changed);
    }

    void computeFollowSets() {
        for (const auto& nonTerminal : nonTerminals)
            follow[nonTerminal] = {};

        follow[startSymbol].insert("$");

        bool changed = true;
        while (changed) {
            changed = false;

            for (const auto& [lhs, rhsList] : productions) {
                for (const auto& rhs : rhsList) {
                    for (size_t i = 0; i < rhs.size(); ++i) {
                        const string& B = rhs[i];
                        if (!nonTerminals.count(B))
                            continue;

                        unordered_set<string> trailer;

                        // Compute FIRST(β) where β = rhs[i+1..end]
                        bool allNullable = true;
                        for (size_t j = i + 1; j < rhs.size(); ++j) {
                            const string& beta = rhs[j];

                            for (const string& sym : first[beta]) {
                                if (sym != "")
                                    trailer.insert(sym);
                            }

                            if (!nullable.count(beta)) {
                                allNullable = false;
                                break;
                            }
                        }

                        // If everything after B is nullable or it's the end, add FOLLOW(lhs)
                        if (i == rhs.size() - 1 || allNullable)
                            trailer.insert(follow[lhs].begin(), follow[lhs].end());

                        size_t oldSize = follow[B].size();
                        follow[B].insert(trailer.begin(), trailer.end());
                        if (follow[B].size() > oldSize)
                            changed = true;
                    }
                }
            }
        }
    }
};

class ASTNode {
public:
    string type;
    Token token;
    vector<ASTNode*> children;

    ASTNode(const string& type) : type(type), token({ "", "", -1 }) {}

    ASTNode(const string& type, const Token& token) : type(type), token(token) {}

    void addChild(ASTNode* child) {
        children.push_back(child);
    }
};

class AST {
public:
    ASTNode* root;

    AST(ASTNode* rootNode) : root(rootNode) {}

    void writeToFile(const string& filename = "AST.txt") {
        ofstream outFile(filename);
        if (!outFile) {
            cerr << "Error writing AST to file!" << endl;
            return;
        }
        writeNode(outFile, root, 0);
        outFile.close();
    }

private:
    void writeNode(ofstream& outFile, ASTNode* node, int depth) {
        for (int i = 0; i < depth; i++) outFile << "  ";
        outFile << node->type;
        if (!node->token.value.empty()) outFile << " (" << node->token.value << ")";
        outFile << "\n";
        for (auto child : node->children)
            writeNode(outFile, child, depth + 1);
    }
};

struct LR0Item {
    string lhs;
    vector<string> rhs;
    int dotPosition;

    LR0Item(string lhs, vector<string> rhs, int dotPosition)
        : lhs(lhs), rhs(rhs), dotPosition(dotPosition) {
    }

    bool operator==(const LR0Item& other) const {
        return lhs == other.lhs && rhs == other.rhs && dotPosition == other.dotPosition;
    }

    bool operator<(const LR0Item& other) const {
        return tie(lhs, rhs, dotPosition) < tie(other.lhs, other.rhs, other.dotPosition);
    }

    string encode() const {
        stringstream ss;
        ss << lhs << ": ";
        for (size_t i = 0; i < rhs.size(); ++i) {
            if (i == dotPosition) ss << "• ";
            ss << rhs[i] << " ";
        }
        if (dotPosition == rhs.size()) ss << "•";
        return ss.str();
    }
};

class LR0State {
public:
    set<LR0Item> rules;
    int index;
    static int count;

    LR0State() : index(count++) {}

    bool operator<(const LR0State& other) const {
        vector<string> thisEnc, otherEnc;
        for (const auto& rule : rules)
            thisEnc.push_back(rule.encode());
        for (const auto& rule : other.rules)
            otherEnc.push_back(rule.encode());
        return thisEnc < otherEnc;
    }

    bool operator==(const LR0State& other) const {
        return rules == other.rules;
    }

    string encode() const {
        stringstream ss;
        for (const auto& rule : rules)
            ss << rule.encode() << " | ";
        return ss.str();
    }
};

int LR0State::count = 0;

class SLR1Parser {
public:
    CFG cfg;
    vector<LR0State> states;
    unordered_map<int, unordered_map<string, int>> actionTable;
    unordered_map<int, unordered_map<string, int>> gotoTable;
    unordered_map<int, unordered_map<string, int>> reductionTable;
    unordered_set<string> types;

    void closure(LR0State& state) {
        //cout << "Computing closure for state " << state.index << endl;
        queue<LR0Item> q;
        for (const auto& rule : state.rules)
            q.push(rule);
        while (!q.empty()) {
            LR0Item rule = q.front();
            q.pop();
            if (rule.dotPosition < rule.rhs.size()) {
                string symbol = rule.rhs[rule.dotPosition];
                if (cfg.nonTerminals.count(symbol)) {
                    for (const auto& production : cfg.productions[symbol]) {
                        LR0Item newRule(symbol, production, 0);
                        if (state.rules.insert(newRule).second) {
                            //cout << "Added to closure: " << symbol << endl;
                            q.push(newRule);
                        }
                    }
                }
            }
        }
        //cout << "Closure computed for state " << state.index << endl;
    }

    LR0State gotoFunction(const LR0State& state, const string& symbol) {
        //cout << "Computing GOTO for state " << state.index << " on symbol '" << symbol << "'\n";
        LR0State newState;
        for (const auto& rule : state.rules) {
            if (rule.dotPosition < rule.rhs.size() && rule.rhs[rule.dotPosition] == symbol)
                newState.rules.insert(LR0Item(rule.lhs, rule.rhs, rule.dotPosition + 1));
        }
        /*if (!newState.rules.empty()) {
            cout << "GOTO on symbol '" << symbol << "' from state " << state.index << " yields new state\n";
        }*/
        closure(newState);
        //cout << "GOTO computed for state " << state.index << " on symbol '" << symbol << "'\n";
        return newState;
    }

    void constructLR0Items() {
        cout << "Constructing LR(0) items...\n";

        LR0State startState;
        startState.rules.insert(LR0Item("S'", { cfg.startSymbol }, 0));
        closure(startState);

        map<LR0State, int> stateCache;
        startState.index = 0;
        states.push_back(startState);
        stateCache[startState] = 0;

        queue<LR0State> q;
        q.push(startState);

        while (!q.empty()) {
            LR0State currState = q.front();
            q.pop();

            // Handle GOTO for terminals (SHIFT)
            for (const string& symbol : cfg.terminals) {
                LR0State newState = gotoFunction(currState, symbol);
                if (!newState.rules.empty()) {
                    if (stateCache.find(newState) == stateCache.end()) {
                        newState.index = states.size();
                        states.push_back(newState);
                        stateCache[newState] = newState.index;
                        q.push(newState);
                    }
                    actionTable[currState.index][symbol] = stateCache[newState];
                }
            }

            // Handle GOTO for non-terminals
            for (const string& symbol : cfg.nonTerminals) {
                LR0State newState = gotoFunction(currState, symbol);
                if (!newState.rules.empty()) {
                    if (stateCache.find(newState) == stateCache.end()) {
                        newState.index = states.size();
                        states.push_back(newState);
                        stateCache[newState] = newState.index;
                        q.push(newState);
                    }
                    gotoTable[currState.index][symbol] = stateCache[newState];
                }
            }

            // Check for REDUCE actions in this state
            for (const auto& item : currState.rules) {
                if (item.dotPosition == item.rhs.size() || item.rhs[item.dotPosition] == "") {
                    if (item.lhs == "S'") {
                        // Accept on $
                        if (reductionTable[currState.index].count("$")) {
                            cerr << "Conflict at state " << currState.index << " on symbol '$' — multiple ACCEPT/SHIFT/REDUCE\n";
                        }
                        else {
                            reductionTable[currState.index]["$"] = -1; // convention: -1 for ACCEPT
                        }
                    }
                    else {
                        int ruleNumber = cfg.getProductionIndex(item.lhs, item.rhs);
                        cout << "State : " << currState.index << " Rule " << ruleNumber << " : " << item.lhs << " -> ";
                        for (const string& sym : item.rhs) {
                            cout << sym << " ";
                        }
                        cout << "\n";
                        for (const string& sym : cfg.follow[item.lhs]) {
                            if (actionTable[currState.index].count(sym)) {
                                cerr << "SHIFT/REDUCE conflict at state " << currState.index
                                    << " on symbol '" << sym << "'\n";
                            }
                            if (reductionTable[currState.index].count(sym)) {
                                cerr << "REDUCE/REDUCE conflict at state " << currState.index
                                    << " on symbol '" << sym << "' — existing: "
                                    << reductionTable[currState.index][sym]
                                    << ", new: " << ruleNumber << endl;
                            }
                            else {
                                reductionTable[currState.index][sym] = ruleNumber;
                            }
                        }
                    }
                }
            }
        }
        // writeRulesToFile("rules.txt");
        writeItemsToFile("items.txt");
        writeActionTable();
        writeGotoTable();
        writeReductionTable();
        cout << "LR(0) items constructed successfully\n";
        cout << "Total states: " << states.size() << endl;
    }

    void writeRulesToFile(const string& filename) {
        ofstream fout(filename);
        if (!fout.is_open()) {
            cerr << "Could not open " << filename << " for writing\n";
            return;
        }

        int index = 0;
        for (const auto& [lhs, productions] : cfg.productions) {
            for (const auto& rhs : productions) {
                fout << index++ << ": " << lhs << " -> ";
                for (const string& sym : rhs) {
                    fout << sym << " ";
                }
                fout << "\n";
            }
        }

        fout.close();
    }

    void writeItemsToFile(const string& filename) {
        ofstream fout(filename);
        if (!fout.is_open()) {
            cerr << "Could not open " << filename << " for writing\n";
            return;
        }

        fout << "(\n";
        for (const auto& state : states) {
            fout << state.index << ":\n";
            for (const auto& item : state.rules) {
                fout << item.lhs << " -> ";
                for (int i = 0; i < item.rhs.size(); ++i) {
                    if (i == item.dotPosition)
                        fout << ". ";
                    fout << item.rhs[i] << " ";
                }
                if (item.dotPosition == item.rhs.size())
                    fout << ".";
                fout << "\n";
            }
            fout << "\n";
        }
        fout << ")\n";

        fout.close();
    }

    void writeActionTable(const string& filename = "action_table.txt") const {
        ofstream out(filename);
        for (const auto& [state, actions] : actionTable) {
            for (const auto& [symbol, nextState] : actions) {
                out << "action[" << state << "][" << symbol << "] = " << nextState << "\n";
            }
        }
        out.close();
    }
    void writeGotoTable(const string& filename = "goto_table.txt") const {
        ofstream out(filename);
        for (const auto& [state, gotos] : gotoTable) {
            for (const auto& [symbol, nextState] : gotos) {
                out << "goto[" << state << "][" << symbol << "] = " << nextState << "\n";
            }
        }
        out.close();
    }
    void writeReductionTable(const string& filename = "reduction_table.txt") const {
        ofstream out(filename);
        for (const auto& [state, reductions] : reductionTable) {
            for (const auto& [symbol, prodIndex] : reductions) {
                out << "reduce[" << state << "][" << symbol << "] = " << prodIndex << "\n";
            }
        }
        out.close();
    }

    SLR1Parser(const string& grammarFile = "grammar.json") : cfg(grammarFile) {
        types = { "IDENTIFIER","INTEGER","FLOAT","STRING","BOOLEAN", "NEWLINE", "INDENT", "DEDENT" };
        constructLR0Items();
    }

    void parse(const vector<Token>& tokens) {
        cout << "Parsing...\n";
        vector<int> stateStack = { 0 };
        vector<ASTNode*> symbolStack;

        size_t i = 0;
        Token currentToken = tokens[i];

        while (true) {
            int currentState = stateStack.back();
            string symbol = currentToken.type;
            //cout << "Current Symbol: " << symbol << endl;
            if (!types.count(symbol))
                symbol = currentToken.value;
            cout << "Current state: " << currentState << ", Current token: " << symbol << " ";
            // Case 2: SHIFT
            if (actionTable[currentState].count(symbol)) {
                int nextState = actionTable[currentState][symbol];
                stateStack.push_back(nextState);
                cout << "SHIFT to state: " << nextState << endl;
                symbolStack.push_back(new ASTNode(currentToken.type, currentToken));
                currentToken = tokens[++i];
            }

            // Case 3: REDUCE
            else if (reductionTable[currentState].count(symbol)) {
                int prodIndex = reductionTable[currentState][symbol];
                cout << "REDUCE by production: " << prodIndex << " ";
                if (prodIndex == -1) {
                    cout << "Parsing successful. AST written to AST.txt\n";
                    AST ast(symbolStack.back());
                    ast.writeToFile();
                    return;
                }
                auto [lhs, rhs] = cfg.getProductionByIndex(prodIndex);

                vector<ASTNode*> children;
                for (int j = 0; j < rhs.size(); ++j) {
                    stateStack.pop_back();
                    children.push_back(symbolStack.back());
                    symbolStack.pop_back();
                }

                reverse(children.begin(), children.end());
                ASTNode* parent = new ASTNode(lhs);
                for (auto child : children)
                    parent->addChild(child);

                symbolStack.push_back(parent);

                int topState = stateStack.back();
                if (!gotoTable[topState].count(lhs)) {
                    cerr << "No GOTO entry for state " << topState << " on symbol '" << lhs << "'\n";
                    //write everything which is in the stack currently
                    /*cout << "\nSymbol stack contents: ";
                    for (auto node : symbolStack) {
                        if (!types.count(node->type)) {
                            cout << node->token.value << " ";
                        }
                        else {
                            cout << node->type << " ";
                        }
                    }
                    cout << "\n";*/
                    return;
                }
                cout << "GOTO to state: " << gotoTable[topState][lhs] << endl;
                stateStack.push_back(gotoTable[topState][lhs]);
            }

            // Case 4: ERROR
            else {
                cerr << "Syntax error at line " << currentToken.line << " near '" << currentToken.value << "'\n";
                return;
            }
            cout << "Current state stack: ";
            for (auto state : stateStack) {
                cout << state << " ";
            }
            cout << endl;
        }
    }
};