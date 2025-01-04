#include <cmath>
#include <iostream>
#include <stack>
#include <string>
#include <sstream>
#include <vector>
#include <variant>
#include <stdexcept>
#include <filesystem>
#include <cctype>
#include <unordered_map>
#include <fstream> // For file handling
#include "Smepscript.h"

struct Parenthesis;
struct Variable;
namespace fs = std::filesystem;

struct Number { double value; };
struct Operator { char symbol;

    bool operator==(std::vector<std::variant<Number, Operator, ::Parenthesis, ::Variable>>::const_reference value) const;
};
struct Parenthesis { char symbol; };
struct Variable { std::string name; };

using Token = std::variant<Number, Operator, Parenthesis, Variable>;

std::unordered_map<std::string, double> variables;

template <typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

constexpr int precedence(const char op) {
    switch (op) {
        case '^': return 3;
        case '*': case '/': return 2;
        case '+': case '-': return 1;
        default: return 0;
    }
}

double apply_operator(const double a, const double b, const char op) {
    switch (op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/':
            if (b == 0) throw std::runtime_error("Division by zero");
            return a / b;
        case '^': return std::pow(a, b);
        default: throw std::invalid_argument("Unknown operator");
    }
}

bool is_valid_variable_name(const std::string& name) {
    return !name.empty() && std::isalpha(name[0]);
}

std::vector<Token> tokenize_script(const std::string& expr) {
    std::vector<Token> tokens;
    std::istringstream stream(expr);
    std::string word;

    while (stream >> word) {
        if (word == "=") {
            tokens.emplace_back(Operator{'='});
        } else if (is_valid_variable_name(word)) {
            tokens.emplace_back(Variable{word});  // Store as a Variable
        } else if (std::isdigit(word[0]) || word[0] == '.') {
            double num = std::stod(word);
            tokens.emplace_back(Number{num});
        } else if (std::strchr("+-*/^()", word[0])) {
            tokens.emplace_back(Operator{word[0]});
        }
    }
    return tokens;
}

std::vector<Token> tokenize(const std::string& expr) {
    std::vector<Token> tokens;
    std::istringstream stream(expr);
    char ch;
    while (stream >> ch) {
        if (std::isdigit(ch) || ch == '.') {
            stream.putback(ch);
            double num;
            stream >> num;
            tokens.emplace_back(Number{num});
        } else if (std::strchr("+-*/^", ch)) {
            tokens.emplace_back(Operator{ch});
        } else if (ch == '(' || ch == ')') {
            tokens.emplace_back(Parenthesis{ch});
        }
    }
    return tokens;
}

std::vector<Token> infix_to_postfix(const std::vector<Token>& tokens) {
    std::vector<Token> postfix;
    std::stack<Token> operators;

    for (const auto& token : tokens) {
        if (std::holds_alternative<Number>(token)) {
            postfix.push_back(token);
        } else if (std::holds_alternative<Operator>(token)) {
            const auto op = std::get<Operator>(token).symbol;
            while (!operators.empty() && std::holds_alternative<Operator>(operators.top())) {
                if (const auto top_op = std::get<Operator>(operators.top()).symbol; precedence(top_op) >= precedence(op)) {
                    postfix.push_back(operators.top());
                    operators.pop();
                } else {
                    break;
                }
            }
            operators.push(token);
        } else if (std::get<Parenthesis>(token).symbol == '(') {
            operators.push(token);
        } else if (std::get<Parenthesis>(token).symbol == ')') {
            while (!operators.empty() && !std::holds_alternative<Parenthesis>(operators.top())) {
                postfix.push_back(operators.top());
                operators.pop();
            }
            operators.pop();
        }
    }

    while (!operators.empty()) {
        postfix.push_back(operators.top());
        operators.pop();
    }
    return postfix;
}

double evaluate_postfix(const std::vector<Token>& tokens) {
    std::stack<double> values;

    for (const auto& token : tokens) {
        if (std::holds_alternative<Number>(token)) {
            values.push(std::get<Number>(token).value);
        } else if (std::holds_alternative<Operator>(token)) {
            const auto op = std::get<Operator>(token).symbol;
            const auto b = values.top(); values.pop();
            const auto a = values.top(); values.pop();
            values.push(apply_operator(a, b, op));
        }
    }
    return values.top();
}

void enter_script_mode() {
    std::string scriptName;
    std::cout << "Enter script name: ";
    std::getline(std::cin, scriptName);

    // Ensure the scripts directory exists
    fs::create_directory("scripts");

    std::string scriptPath = "scripts/" + scriptName + ".smp";
    std::ofstream scriptFile(scriptPath);

    if (!scriptFile) {
        std::cerr << "Error: Failed to create script file.\n";
        return;
    }

    std::cout << "Script mode (type 'end' to save and exit):\n";

    std::string line;
    while (true) {
        std::cout << ">> ";
        std::getline(std::cin, line);

        if (line == "end") break;

        scriptFile << line << '\n'; // Save each line in the file
    }

    scriptFile.close();
    std::cout << "Script saved to " << scriptPath << std::endl;
}

void executeScript(const std::string& filePath) {
    std::ifstream scriptFile(filePath);
    if (!scriptFile.is_open()) {
        std::cerr << "Error: Could not open the script file.\n";
        return;
    }

    std::string line;
    while (std::getline(scriptFile, line)) {
        // Tokenize the line from the script
        auto tokens = tokenize_script(line);

        try {
            // Check for assignments
            if (!tokens.empty() && std::holds_alternative<Operator>(tokens[0]) &&
                std::get<Operator>(tokens[0]).symbol == '=') {
                const auto var_name = std::get<Variable>(tokens[1]).name;
                double result = evaluate_postfix(tokens);
                variables[var_name] = result;
                std::cout << var_name << " = " << result << std::endl;
                } else if (tokens[0] == Operator{'p'}) { // For printf functionality (e.g., printf(result))
                    if (tokens.size() == 2 && std::holds_alternative<Variable>(tokens[1])) {
                        if (const auto var_name = std::get<Variable>(tokens[1]).name; variables.contains(var_name)) {
                            std::cout << variables[var_name] << std::endl;
                        } else {
                            std::cerr << "Error: Undefined variable " << var_name << std::endl;
                        }
                    }
                } else {
                    double result = evaluate_postfix(tokens);
                    std::cout << "Result: " << result << std::endl;
                }
        } catch (const std::exception& e) {
            std::cerr << "Error in script execution: " << e.what() << std::endl;
        }
    }
    scriptFile.close();
}

int main() {
    std::string input;
    std::cout << "Welcome to SMEP++, the simplest math evaluation program.\n";
    std::cout << "Enter an expression (type help to see the instructions).\n";

    while (true) {
        std::cout << "SIC>";
        std::getline(std::cin, input);

        if (input == "exit") break;
        if (input == "help") {
            std::cout << "-----HELP-----\n"
            << "Supported mathematical symbols: +, -, =, /, ^, *\n"
            << "To exit the program write 'exit'\n"
            << "To see the current version of the program write 'version'"
            << std::endl;
            continue;
        }
        if (input == "version") {
            std::cout << "SMEP++ version 0.1! Still in development!\n";
            continue;
        }

        if (input == "script") {
            enter_script_mode();
            continue;
        }

        try {
            auto tokens = tokenize(input);
            auto postfix = infix_to_postfix(tokens);
            const auto result = evaluate_postfix(postfix);
            std::cout << "Result: " << result << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error:"
                         " " << e.what() << std::endl;
        }
    }

    return 0;
}
