#include "luna/version.hpp"
#include "vm/VM.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace {

constexpr std::string_view BANNER = R"(
 ██╗     ██╗   ██╗███╗   ██╗ █████╗
 ██║     ██║   ██║████╗  ██║██╔══██╗
 ██║     ██║   ██║██╔██╗ ██║███████║
 ██║     ██║   ██║██║╚██╗██║██╔══██║
 ███████╗╚██████╔╝██║ ╚████║██║  ██║
 ╚══════╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝  ╚═╝
)";

void print_usage(std::string_view prog) {
    std::cerr << "Usage: " << prog << " <script.luna>\n";
    std::cerr << "       " << prog << " --version\n";
    std::cerr << "       " << prog << " --help\n";
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc == 2) {
        std::string_view arg{argv[1]};
        if (arg == "--version") {
            std::cout << "Luna Language Compiler v" << luna::VERSION_STRING << '\n';
            std::cout << "Gravity Bytecode VM & Sandboxed Runtime\n";
            return EXIT_SUCCESS;
        }
        if (arg == "--help" || arg == "-h") {
            std::cout << BANNER << '\n';
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        }

        // Read source file
        std::ifstream file(argv[1]);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file '" << argv[1] << "'.\n";
            return EXIT_FAILURE;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string source = buffer.str();

        luna::vm::VM vm;
        auto result = vm.interpret(source);
        if (result != luna::vm::InterpretResult::Ok) {
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    std::cout << BANNER << '\n';
    print_usage(argv[0]);
    return EXIT_SUCCESS;
}
