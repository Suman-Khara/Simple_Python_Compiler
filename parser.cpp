#include<bits/stdc++.h>
#include "json.hpp"

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

    CFG(const string& filename) {
        ifstream file(filename);
        json grammar;
        file >> grammar;

        terminals = unordered_set<string>(grammar["terminals"].begin(), grammar["terminals"].end());
        nonTerminals = unordered_set<string>(grammar["nonTerminals"].begin(), grammar["nonTerminals"].end());

        for (const auto& rule : grammar["productions"].items()) {
            string lhs = rule.key();
            vector<vector<string>> rhs = rule.value();
            productions[lhs] = rhs;
        }
        startSymbol = grammar["startSymbol"];
        computeFirstSets();
        computeFollowSets();
    }

private:
    void computeFirstSets() {

    }

    void computeFollowSets() {

    }
};
