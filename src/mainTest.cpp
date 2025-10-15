#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <tree_sitter/api.h>

using namespace std::literals;

extern "C" TSLanguage* tree_sitter_stp();

// Read a chunk with overlap
std::string read_file_chunk(const std::string& path, size_t offset, size_t chunk_size, size_t overlap, bool& eof)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file)
        return "";

    file.seekg(offset, std::ios::beg);
    std::string chunk(chunk_size + overlap, '\0');
    file.read(&chunk[0], chunk_size + overlap);
    std::streamsize bytes_read = file.gcount();

    if (bytes_read < static_cast<std::streamsize>(chunk_size + overlap))
    {
        chunk.resize(bytes_read);
        eof = true;
    }
    else
    {
        eof = false;
    }
    return chunk;
}

void print_node_in_chunk(TSNode node,
                         const std::string& chunk,
                         size_t chunk_start,
                         size_t chunk_end,
                         int indent = 0)
{
    std::string type = ts_node_type(node);
    uint32_t start = ts_node_start_byte(node);
    uint32_t end = ts_node_end_byte(node);

    // Ensure whole block is fully read
    if (start >= chunk_start and end <= chunk_end)
    {
        std::string text;
        if (end - chunk_start <= chunk.size())
            text = chunk.substr(start - chunk_start, end - start);

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
                or type == "\\{" or type == "\\}" // string formatting
                ))
        {
            for (int i = 0; i < indent - 1; ++i)
                std::cout << "    ";
            std::cout << "'" << type << "' : \"" << text << "\"\n";
        }
    }

    uint32_t child_count = ts_node_child_count(node);
    for (uint32_t i = 0; i < child_count; ++i)
        print_node_in_chunk(ts_node_child(node, i), chunk, chunk_start, chunk_end, indent + 1);
}

int main(int argc, const char** argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: stp_parse <source-file> [chunk-size] [overlap-size]\n";
        return 1;
    }

    std::string path = argv[1];
    size_t chunk_size = 1024ULL * 1024ULL; // 1MB
    if (argc > 2)
        chunk_size = std::stoull(argv[2]);
    size_t overlap = 4096ULL; // 4KB default overlap
    if (argc > 3)
        overlap = std::stoull(argv[3]);

    size_t offset = 0;
    bool eof = false;
    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_stp());
    TSTree* tree = nullptr;

    while (!eof)
    {
        std::string chunk = read_file_chunk(path, offset, chunk_size, overlap, eof);
        if (chunk.empty())
            break;

        if (tree != nullptr)
            ts_tree_delete(tree);

        // Parse only the current chunk plus overlap
        tree = ts_parser_parse_string(parser, nullptr, chunk.c_str(), chunk.size());

        size_t chunk_start = offset;
        size_t chunk_end = offset + chunk_size; // Only print nodes in non-overlap region

        TSNode root_node = ts_tree_root_node(tree);
        print_node_in_chunk(root_node, chunk, chunk_start, chunk_end);

        offset += chunk_size;
    }

    if (tree != nullptr)
        ts_tree_delete(tree);
    ts_parser_delete(parser);

    return 0;
}