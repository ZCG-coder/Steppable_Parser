#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <tree_sitter/api.h>

using namespace std::literals;

extern "C" TSLanguage* tree_sitter_stp();

std::string read_file(const std::string& path)
{
    std::ifstream file(path, std::ios::in);
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

// Recursively print the syntax tree
void print_node(TSNode node, const std::string& source_code, int indent = 0)
{
    std::string type = ts_node_type(node);

    uint32_t start = ts_node_start_byte(node);
    uint32_t end = ts_node_end_byte(node);
    std::string text;

    // Ignore the following tokens
    if (not(type == "\n" // newline
            or type == "comment" // comments
            or type == "source_file" // top-level of tree
            or type == "(" or type == ")" // brackets
            or type == "[" or type == "]" // square brackets
            or type == "{" or type == "}" // braces
            or type == "=" // braces
            or type == "->" // return value notation
            or type == "," // param separator
            or type == ";" // stmt force separator
            or type == "\"" // quotes
            or type == "|" // matrix notation
            or type == "\\{" // string formatting operator
            ))
    {
        text = source_code.substr(start, end - start);

        for (int i = 0; i < indent - 1; ++i)
            std::cout << "    ";
        std::cout << "'" << type << "' : \"" << text << "\"\n";
    }

    uint32_t child_count = ts_node_child_count(node);
    for (uint32_t i = 0; i < child_count; ++i)
        print_node(ts_node_child(node, i), source_code, indent + 1);
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: stp_parse <source-file>\n";
        return 1;
    }

    std::string source_code = read_file(argv[1]);

    // Create parser
    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_stp());

    // Parse source code
    TSTree* tree = ts_parser_parse_string(parser, nullptr, source_code.c_str(), source_code.size());

    // Print the syntax tree
    TSNode root_node = ts_tree_root_node(tree);
    print_node(root_node, source_code);

    // Clean up
    ts_tree_delete(tree);
    ts_parser_delete(parser);

    return 0;
}
