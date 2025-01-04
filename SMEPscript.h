#ifndef SMEPSCRIPT_H
#define SMEPSCRIPT_H

#include <functional>
#include <variant>
#include <vector>
#include <map>
#include <string>
#include <iostream>

// Type definitions
using var = std::variant<int, float, double, bool>;
using args = std::vector<var>;

// Forward declaration
struct Function;
using functionTable = std::map<std::string, Function>;

// Full definition of Function struct
struct Function {
    std::string name;
    std::function<var(const args&)> body;
};

// Function declarations
var callFunction(const std::string& name, const args& args, functionTable& functionTable);
std::ostream& operator<<(std::ostream& os, const var& v);

// Macro definition for creating a function
#define makeFunction(name, numArgs, returnStatement, FunctionTable) \
FunctionTable[name] = Function{name, [](const args& args) -> var { \
if (args.size() != numArgs) { \
throw std::invalid_argument("Incorrect number of arguments."); \
} \
auto get = [&](size_t i) -> double { \
return std::visit([](auto&& arg) -> double { return static_cast<double>(arg); }, args[i]); \
}; \
return returnStatement; \
}};






#endif // SMEPSCRIPT_H
