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

    CFG(const string& filename) {
        cout << "Loading grammar from " << filename << endl;
        ifstream file(filename);
        json grammar;
        file >> grammar;
        cout << "Grammar loaded successfully\n";
        terminals = unordered_set<string>(grammar["terminals"].begin(), grammar["terminals"].end());
        nonTerminals = unordered_set<string>(grammar["nonTerminals"].begin(), grammar["nonTerminals"].end());

        for (const auto& rule : grammar["productions"].items()) {
            string lhs = rule.key();
            vector<vector<string>> rhsList = rule.value();
            for (const auto& rhs : rhsList) {
                productions[lhs].push_back(rhs);
                numberedProductions.emplace_back(lhs, rhs);
            }
        }

        startSymbol = grammar["startSymbol"];
        computeFirstSets();
        computeFollowSets();
    }

    pair<string, vector<string>> getProductionByIndex(int index) const {
        return numberedProductions[index];
    }

    int getProductionIndex(const string& lhs, const vector<string>& rhs) const {
        for (size_t i = 0; i < numberedProductions.size(); ++i) {
            if (numberedProductions[i].first == lhs && numberedProductions[i].second == rhs)
                return static_cast<int>(i);
        }
        return -1;
    }

private:
    void computeFirstSets() {
        cout << "Computing FIRST sets...\n";
        for (const string& terminal : terminals)
            first[terminal] = { terminal };

        for (const string& nonTerminal : nonTerminals)
            first[nonTerminal] = {};

        bool changed;
        do {
            changed = false;
            for (const string& A : nonTerminals) {
                size_t oldSize = first[A].size();
                for (const auto& production : productions[A]) {
                    bool epsilonInAll = true;
                    for (const string& symbol : production) {

                        for (const string& val : first[symbol]) {
                            if (val != "ε")
                                first[A].insert(val);
                        }

                        if (first[symbol].find("ε") == first[symbol].end()) {
                            epsilonInAll = false;
                            break;
                        }
                    }

                    if (epsilonInAll)
                        first[A].insert("ε");

                    if (first[A].size() > oldSize) {
                        changed = true;
                        /*cout << "FIRST(" << A << ") updated: { ";
                        for (const string& s : first[A])
                            cout << s << " ";
                        cout << "}" << endl;*/
                    }
                }
            }
        } while (changed);
        cout << "FIRST sets computed successfully\n";
    }

    void computeFollowSets() {
        cout << "Computing FOLLOW sets...\n";
        for (const auto& nonTerminal : nonTerminals)
            follow[nonTerminal] = unordered_set<string>();

        follow[startSymbol].insert("$");

        bool changed = true;
        while (changed) {
            changed = false;

            for (const auto& [lhs, rhsList] : productions) {
                for (const auto& rhs : rhsList) {
                    for (size_t i = 0; i < rhs.size(); ++i) {
                        string symbol = rhs[i];

                        if (nonTerminals.count(symbol)) {
                            unordered_set<string> firstNext;

                            if (i + 1 < rhs.size()) {
                                string nextSymbol = rhs[i + 1];
                                if (terminals.count(nextSymbol))
                                    firstNext.insert(nextSymbol);
                                else
                                    firstNext.insert(first[nextSymbol].begin(), first[nextSymbol].end());

                                firstNext.erase("ε");
                            }

                            if (i + 1 == rhs.size() || (i + 1 < rhs.size() && first[rhs[i + 1]].count("ε")))
                                firstNext.insert(follow[lhs].begin(), follow[lhs].end());

                            size_t beforeSize = follow[symbol].size();
                            follow[symbol].insert(firstNext.begin(), firstNext.end());

                            if (follow[symbol].size() > beforeSize) {
                                changed = true;
                                /*cout << "FOLLOW(" << symbol << ") updated: { ";
                                for (const string& s : follow[symbol])
                                    cout << s << " ";
                                cout << "}" << endl;*/
                            }
                        }
                    }
                }
            }
        }
        cout << "FOLLOW sets computed successfully\n";
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
};

class LR0State {
public:
    set<LR0Item> rules;
    int index;
    static int count;

    LR0State() : index(count++) {}
};

int LR0State::count = 0;

class SLR1Parser {
public:
    CFG cfg;
    vector<LR0State> states;
    unordered_map<int, unordered_map<string, string>> actionTable;
    unordered_map<int, unordered_map<string, int>> gotoTable;

    void closure(LR0State& state) {
        //cout << "Computing closure for state " << state.index << endl;
        queue<LR0Item> q;
        for (const auto& item : state.rules)
            q.push(item);
        while (!q.empty()) {
            LR0Item item = q.front();
            q.pop();
            if (item.dotPosition < item.rhs.size()) {
                string symbol = item.rhs[item.dotPosition];
                if (cfg.nonTerminals.count(symbol)) {
                    for (const auto& production : cfg.productions[symbol]) {
                        LR0Item newItem(symbol, production, 0);
                        if (state.rules.insert(newItem).second) {
                            //cout << "Added to closure: " << symbol << endl;
                            q.push(newItem);
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
        for (const auto& item : state.rules) {
            if (item.dotPosition < item.rhs.size() && item.rhs[item.dotPosition] == symbol)
                newState.rules.insert(LR0Item(item.lhs, item.rhs, item.dotPosition + 1));
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

        states.push_back(startState);
        unordered_map<int, int> stateMapping;
        stateMapping[startState.index] = 0;

        queue<LR0State> q;
        q.push(startState);

        while (!q.empty()) {
            LR0State currState = q.front();
            q.pop();
            cout << "Processing state " << currState.index << endl;
            for (const string& symbol : cfg.terminals) {
                LR0State newState = gotoFunction(currState, symbol);
                if (!newState.rules.empty() && stateMapping.find(newState.index) == stateMapping.end()) {
                    stateMapping[newState.index] = states.size();
                    states.push_back(newState);
                    q.push(newState);
                    cout << "New LR(0) state created on terminal symbol: " << symbol << endl;
                }
                gotoTable[currState.index][symbol] = stateMapping[newState.index];
            }

            for (const string& symbol : cfg.nonTerminals) {
                LR0State newState = gotoFunction(currState, symbol);
                if (!newState.rules.empty() && stateMapping.find(newState.index) == stateMapping.end()) {
                    stateMapping[newState.index] = states.size();
                    states.push_back(newState);
                    q.push(newState);
                    cout << "New LR(0) state created on non-terminal symbol: " << symbol << endl;
                }
                gotoTable[currState.index][symbol] = stateMapping[newState.index];
            }
        }
        cout << "LR(0) items constructed successfully\n";
        cout << "Total states: " << states.size() << endl;
    }

    void constructParseTable() {
        cout << "Constructing parse table...\n";
        for (const auto& state : states) {
            for (const auto& item : state.rules) {
                // Case 1: SHIFT
                if (item.dotPosition < item.rhs.size()) {
                    string symbol = item.rhs[item.dotPosition];

                    if (cfg.terminals.count(symbol)) {
                        int nextState = gotoTable[state.index][symbol];
                        string action = "SHIFT " + to_string(nextState);

                        if (actionTable[state.index].count(symbol))
                            cerr << "Conflict at state " << state.index << " on symbol '" << symbol
                            << "' — existing: " << actionTable[state.index][symbol]
                            << ", new: " << action << endl;
                        else
                            actionTable[state.index][symbol] = action;
                    }
                    //cout << "Action[State " << state.index << "][" << symbol << "] = " << action << endl;
                }
                // Case 2: REDUCE / ACCEPT
                else {
                    if (item.lhs == "S'") {
                        string symbol = "$";
                        string action = "ACCEPT";

                        if (actionTable[state.index].count(symbol))
                            cerr << "Conflict at state " << state.index << " on symbol '$'"
                            << " — existing: " << actionTable[state.index][symbol]
                            << ", new: " << action << endl;
                        else
                            actionTable[state.index]["$"] = "ACCEPT";
                    }
                    else {
                        int prodIndex = cfg.getProductionIndex(item.lhs, item.rhs);
                        if (prodIndex != -1) {
                            string action = "REDUCE " + to_string(prodIndex);
                            for (const string& followSym : cfg.follow[item.lhs]) {
                                if (actionTable[state.index].count(followSym))
                                    cerr << "Conflict at state " << state.index << " on symbol '" << followSym
                                    << "' — existing: " << actionTable[state.index][followSym]
                                    << ", new: " << action << endl;
                                else
                                    actionTable[state.index][followSym] = action;
                            }
                        }
                        /*for (const string& followSym : cfg.follow[item.lhs]) {
                            cout << "Action[State " << state.index << "][" << followSym << "] = REDUCE " << prodIndex << endl;
                        }*/
                    }
                }
            }
        }
        cout << "Parse table constructed successfully\n";
    }

    SLR1Parser(const string& grammarFile = "grammar.json") : cfg(grammarFile) {
        constructLR0Items();
        constructParseTable();
    }

    void parse(const vector<Token>& tokens) {
        cout << "Parsing...\n";
        vector<int> stateStack = { 0 };
        vector<ASTNode*> symbolStack;

        size_t i = 0;
        Token currentToken = tokens[i];

        while (true) {
            int currentState = stateStack.back();
            string action = actionTable[currentState][currentToken.type];

            if (action.empty()) {
                cerr << "Syntax error at line " << currentToken.line << " near '" << currentToken.value << "'\n";
                return;
            }

            if (action == "ACCEPT") {
                cout << "Parsing successful. AST written to AST.txt\n";
                AST ast(symbolStack.back());
                ast.writeToFile();
                return;
            }

            if (action.substr(0, 5) == "SHIFT") {
                int nextState = stoi(action.substr(6));
                stateStack.push_back(nextState);
                symbolStack.push_back(new ASTNode(currentToken.type, currentToken));
                currentToken = tokens[++i];
            }
            else if (action.substr(0, 6) == "REDUCE") {
                int prodIndex = stoi(action.substr(7));
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
                stateStack.push_back(gotoTable[topState][lhs]);
            }
        }
        cout << "Parsing completed\n";
    }
};