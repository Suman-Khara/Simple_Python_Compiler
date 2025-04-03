#include <bits/stdc++.h>

using namespace std;

class LiteralTable {
private:
    struct LiteralInfo {
        queue<size_t> unassigned;
        unordered_set<size_t> assigned;
    };

    map<pair<string, string>, LiteralInfo> table;
    size_t memoryCounter = 1000;
public:
    void addLiteral(const string& value, const string& type) {
        void* newAddress = nullptr;
        if (type == "int")
            newAddress = new int;
        else if (type == "float")
            newAddress = new float;
        else if (type == "string")
            newAddress = new string;
        else if (type == "bool")
            newAddress = new bool;
        else {
            cerr << "Error: Unsupported type '" << type << "'\n";
            return;
        }
        table[{value, type}].unassigned.push(reinterpret_cast<size_t>(newAddress));
    }

    size_t getLiteral(const string& value, const string& type) {
        auto& literalInfo = table[{value, type}];
        if (literalInfo.unassigned.empty()) {
            cerr << "Error: No unassigned addresses for literal " << value << " of type " << type << endl;
            return 0; // Indicating an error
        }
        size_t addr = literalInfo.unassigned.front();
        literalInfo.unassigned.pop();
        literalInfo.assigned.insert(addr);
        return addr;
    }

    void print() const {
        cout << "Literal Table:\n";
        for (const auto& entry : table) {
            const auto& key = entry.first;
            const auto& info = entry.second;
            cout << "Value: " << key.first << ", Type: " << key.second << "\n";
            cout << "  Unassigned Addresses: ";
            queue<size_t> tempQueue = info.unassigned;
            while (!tempQueue.empty()) {
                cout << tempQueue.front() << " ";
                tempQueue.pop();
            }
            cout << "\n  Assigned Addresses: ";
            for (size_t addr : info.assigned) {
                cout << addr << " ";
            }
            cout << "\n\n";
        }
    }
};

int main() {
    LiteralTable lt;

    // Adding literals
    lt.addLiteral("42", "int");
    lt.addLiteral("3.14", "float");
    lt.addLiteral("Hello", "string");
    lt.addLiteral("True", "bool");

    // Retrieving literals
    size_t addr1 = lt.getLiteral("42", "int");
    size_t addr2 = lt.getLiteral("3.14", "float");
    size_t addr3 = lt.getLiteral("Hello", "string");

    // Print the table
    lt.print();

    // Additional retrievals
    size_t addr4 = lt.getLiteral("42", "int"); // Should work if another 42 was added
    size_t addr5 = lt.getLiteral("False", "bool"); // Should show error (not added)

    // Final table state
    lt.print();

    return 0;
}
