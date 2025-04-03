#include <bits/stdc++.h>

using namespace std;

struct SymbolInfo {
    size_t memAddress;
    string type;
    size_t valueAddress;
};

class SymbolTable {
private:
    unordered_map<string, SymbolInfo> table;

public:
    void addSymbol(const string& identifier) {
        if (table.find(identifier) == table.end()) {
            size_t memAddress = reinterpret_cast<size_t>(new int); // Generate a memory address
            table[identifier] = { memAddress, "UNKNOWN", 0 };
        }
    }

    void addValueToSymbol(const string& identifier, size_t valueAddress, const string& type) {
        if (table.find(identifier) != table.end()) {
            table[identifier].valueAddress = valueAddress;
            table[identifier].type = type;
        }
    }

    void print() {
        cout << "\nSymbol Table:\n";
        cout << "--------------------------------------\n";
        cout << "Identifier    | Address   | Type    | Value Address\n";
        cout << "--------------------------------------\n";
        for (const auto& entry : table) {
            cout << entry.first << "\t"
                << entry.second.memAddress << "\t"
                << entry.second.type << "\t"
                << entry.second.valueAddress << "\n";
        }
        cout << "--------------------------------------\n";
    }
};

int main() {
    SymbolTable symTable;
    symTable.addSymbol("a");
    symTable.addSymbol("b");
    symTable.addValueToSymbol("a", reinterpret_cast<size_t>(new int(10)), "int");
    symTable.print();
    return 0;
}