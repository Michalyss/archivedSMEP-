#include "SMEPscript.h"

// Implementation of the callFunction
var callFunction(const std::string& name, const args& args, functionTable& functionTable) {
    if (functionTable.contains(name)) {  // Check if function exists in the table
        return functionTable[name].body(args);  // Execute the function
    }
    throw std::runtime_error("Function not found");
}

// Overloading the << operator for var
std::ostream& operator<<(std::ostream& os, const var& v) {
    std::visit([&os](auto&& arg) {  // Handle all possible types in the variant
        os << arg;
    }, v);
    return os;
}
