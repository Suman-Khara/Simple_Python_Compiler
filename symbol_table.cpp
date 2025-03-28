#include <bits/stdc++.h>

using namespace std;

// -----------------------------------------------------------------------------
// Value Type: A variant that can hold int, double, bool, string, or a list of Values.
// -----------------------------------------------------------------------------
struct Value; // Forward declaration for recursive type.

using List = vector<Value>;

struct Value {
    // The underlying variant storing our value.
    variant<int, double, bool, string, List> data;

    // Constructors for convenience.
    Value() : data(string("")) {} // default to empty string
    Value(int i) : data(i) {}
    Value(double d) : data(d) {}
    Value(bool b) : data(b) {}
    Value(const string& s) : data(s) {}
    Value(const char* s) : data(string(s)) {}
    Value(const List& lst) : data(lst) {}
};

// Helper function to print a Value (for debugging)
void printValue(const Value& val) {
    // Use std::visit to handle each alternative.
    visit([](auto&& arg) {
        using T = decay_t<decltype(arg)>;
        if constexpr (is_same_v<T, int>)
            cout << arg;
        else if constexpr (is_same_v<T, double>)
            cout << arg;
        else if constexpr (is_same_v<T, bool>)
            cout << (arg ? "True" : "False");
        else if constexpr (is_same_v<T, string>)
            cout << arg;
        else if constexpr (is_same_v<T, List>) {
            cout << "[";
            for (size_t i = 0; i < arg.size(); ++i) {
                printValue(arg[i]);
                if (i != arg.size() - 1)
                    cout << ", ";
            }
            cout << "]";
        }
        }, val.data);
}

// -----------------------------------------------------------------------------
// SymbolEntry: Represents one entry in the symbol table.
// -----------------------------------------------------------------------------
struct SymbolEntry {
    string dataType;  // "int", "float", "bool", "str", "list", or "undefined"
    Value value;      // The associated value
};

// -----------------------------------------------------------------------------
// SymbolTable Class
// -----------------------------------------------------------------------------
class SymbolTable {
private:
    unordered_map<string, SymbolEntry> table;
public:
    // Insert or update a symbol in the table.
    void addSymbol(const string& symbol, const string& dataType, const Value& val) {
        SymbolEntry entry;
        entry.dataType = dataType;
        entry.value = val;
        table[symbol] = entry;
    }

    // If a symbol is used before assignment (e.g. c = a), we mark it as undefined.
    void addUndefinedSymbol(const string& symbol) {
        if (table.find(symbol) == table.end()) {
            SymbolEntry entry;
            entry.dataType = "undefined";
            entry.value = Value(""); // Empty string value.
            table[symbol] = entry;
        }
    }

    // Retrieve the value for a given symbol or list element.
    // query can be of the form "symbol" or "lst[index]".
    Value getValue(const string& query) {
        // Check if the query contains a '[' indicating list indexing.
        size_t bracketPos = query.find('[');
        if (bracketPos != string::npos) {
            // Extract base symbol and index string.
            string baseSymbol = query.substr(0, bracketPos);
            size_t closingBracket = query.find(']', bracketPos);
            if (closingBracket == string::npos) {
                throw runtime_error("Syntax Error: Missing closing bracket in list index for symbol '" + query + "'");
            }
            string indexStr = query.substr(bracketPos + 1, closingBracket - bracketPos - 1);
            // Convert index string to an integer.
            int index;
            try {
                index = stoi(indexStr);
            }
            catch (const invalid_argument&) {
                throw runtime_error("Syntax Error: Invalid index '" + indexStr + "' for symbol '" + baseSymbol + "'");
            }
            // Look up the base symbol.
            if (table.find(baseSymbol) == table.end()) {
                throw runtime_error("Error: Symbol '" + baseSymbol + "' is not defined.");
            }
            SymbolEntry entry = table[baseSymbol];
            if (entry.dataType != "list") {
                throw runtime_error("Error: Symbol '" + baseSymbol + "' is not a list.");
            }
            // Get the list and perform index check.
            List lst = get<List>(entry.value.data);
            if (index < 0 || index >= (int)lst.size()) {
                throw runtime_error("Error: Index " + to_string(index) + " out of range for list '" + baseSymbol + "'.");
            }
            return lst[index];
        }
        // Otherwise, treat query as a simple symbol.
        if (table.find(query) == table.end()) {
            throw runtime_error("Error: Symbol '" + query + "' is not defined.");
        }
        return table[query].value;
    }

    // For debugging: print the symbol table.
    void printTable() const {
        cout << left << setw(15) << "Symbol"
            << left << setw(15) << "Type"
            << "Value" << "\n";
        cout << string(50, '-') << "\n";
        for (const auto& entry : table) {
            cout << left << setw(15) << entry.first
                << left << setw(15) << entry.second.dataType;
            printValue(entry.second.value);
            cout << "\n";
        }
    }
};

//
// Example usage (could be removed in the actual module and used via main.cpp)
//
#ifdef DEBUG_SYMBOL_TABLE
int main() {
    SymbolTable st;
    // Adding some symbols:
    st.addSymbol("a", "int", Value(5));
    st.addSymbol("b", "float", Value(3.14));
    st.addSymbol("c", "str", Value("Hello"));
    st.addSymbol("d", "bool", Value(true));

    // Create a list value with mixed types.
    List lst;
    lst.push_back(Value(4));
    lst.push_back(Value(5.6));
    lst.push_back(Value("World"));
    lst.push_back(Value(false));
    st.addSymbol("e", "list", Value(lst));

    // Assignment by variable (f = c). If c exists, copy its value.
    if (true /* suppose c exists */) {
        st.addSymbol("f", "str", st.getValue("c"));
    }
    else {
        st.addUndefinedSymbol("f");
    }

    // Test retrieving a list element.
    try {
        Value elem = st.getValue("e[2]");
        cout << "e[2] = ";
        printValue(elem);
        cout << "\n";
    }
    catch (const exception& e) {
        cerr << e.what() << "\n";
    }

    // Print the full symbol table.
    st.printTable();

    return 0;
}
#endif