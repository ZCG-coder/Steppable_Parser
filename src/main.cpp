#include "include/stpInit.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <tree_sitter/api.h>

using namespace std::literals;
constexpr long chunkSize = 1024L * 1024L; // 1MB
constexpr long overlap = 4096L; // 4KB

extern "C" TSLanguage* tree_sitter_stp();

std::string readFileChunk(const std::string& path, long offset, long chunkSize, long overlap, bool& eof)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (not file)
        return "";

    file.seekg(offset, std::ios::beg);
    std::string chunk(chunkSize + overlap, '\0');
    file.read(chunk.data(), chunkSize + overlap);
    std::streamsize bytesRead = file.gcount();

    if (bytesRead < static_cast<std::streamsize>(chunkSize + overlap))
    {
        chunk.resize(bytesRead);
        eof = true;
    }
    else
    {
        eof = false;
    }
    return chunk;
}

void processChunk(TSNode node, const std::string& chunk, size_t chunkStart, size_t chunkEnd, int indent = 0)
{
    std::string type = ts_node_type(node);
    uint32_t start = ts_node_start_byte(node);
    uint32_t end = ts_node_end_byte(node);

    // Ensure whole block is fully read
    if (start >= chunkStart and end <= chunkEnd)
    {
        std::string text;
        if (end - chunkStart <= chunk.size())
            text = chunk.substr(start - chunkStart, end - start);

        if (not(type == "\n" // Newline
                or type == "comment" // Comment
                or type == "source_file" // Root node
                or type == "(" or type == ")" // Parantheses
                or type == "[" or type == "]" // Square brackets
                or type == "{" or type == "}" // Braces
                or type == "=" // Assignment
                or type == "->" // Return type
                or type == "," //
                or type == "." // Member access
                or type == ";" // Force statement break
                or type == "\"" // String quotes
                or type == "|" // Matrix sep
                or type == "\\{" or type == "\\}" // String formatting
                ))
        {
            for (int i = 0; i < indent - 1; ++i)
                std::cout << "    ";
            std::cout << "'" << type << "' : \"" << text << "\"\n";
        }
    }

    uint32_t child_count = ts_node_child_count(node);
    for (uint32_t i = 0; i < child_count; ++i)
        processChunk(ts_node_child(node, i), chunk, chunkStart, chunkEnd, indent + 1);
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: stp_parse <source-file>\n";
        return 1;
    }

    std::string path = argv[1];
    long offset = 0;
    bool eof = false;
    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_stp());
    TSTree* tree = nullptr;

    steppable::parser::STP_init();

    while (!eof)
    {
        std::string chunk = readFileChunk(path, offset, chunkSize, overlap, eof);
        if (chunk.empty())
            break;

        if (tree != nullptr)
            ts_tree_delete(tree);

        tree = ts_parser_parse_string(parser, nullptr, chunk.c_str(), chunk.size());

        size_t chunkStart = offset;
        size_t chunk_end = offset + chunkSize;

        TSNode rootNode = ts_tree_root_node(tree);
        processChunk(rootNode, chunk, chunkStart, chunk_end);

        offset += chunkSize;
    }

    if (tree != nullptr)
        ts_tree_delete(tree);
    ts_parser_delete(parser);

    steppable::parser::STP_destroy();

    return 0;
}
