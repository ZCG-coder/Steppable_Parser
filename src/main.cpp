#include "steppable/number.hpp"
#include "stpInterp/stpBetterTS.hpp"
#include "stpInterp/stpInit.hpp"
#include "stpInterp/stpProcessor.hpp"
#include "stpInterp/stpStore.hpp"

#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

extern "C" {
#include <tree_sitter/api.h>
}

using namespace std::literals;
using namespace steppable::parser;
using namespace steppable::__internals;

constexpr long chunkSize = 1024L * 1024L; // 1MB
constexpr long overlap = 4096L; // 4KB

extern "C" TSLanguage* tree_sitter_stp();

std::string readFileChunk(const std::string& path, const long offset, bool& eof)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (not file)
        return "";

    file.seekg(offset, std::ios::beg);
    std::string chunk(chunkSize + overlap, '\0');
    file.read(chunk.data(), chunkSize + overlap);
    std::streamsize bytesRead = file.gcount();

    if (bytesRead < chunkSize + overlap)
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

// void handleMemberAccess(TSNode* maNode, STP_InterpState state) {}

bool STP_isPureType(const std::string_view& str)
{
    return str == "matrix" or str == "number" or str == "string" or str == "identifier";
}

int main(int argc, char** argv) // NOLINT(*-exception-escape)
{
    if (argc < 2)
    {
        std::cerr << "Usage: stp_parse <source-file>\n";
        return 1;
    }

    const std::string path = argv[1];
    long offset = 0;
    bool eof = false;
    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_stp());
    TSTree* tree = nullptr;

    const STP_InterpState state = STP_getState();
    state->setFile(path);

    while (not eof)
    {
        std::string chunk = readFileChunk(path, offset, eof);
        if (chunk.empty())
            break;

        if (tree != nullptr)
            ts_tree_delete(tree);

        tree = ts_parser_parse_string(parser, nullptr, chunk.c_str(), chunk.size());

        size_t chunkStart = offset;
        size_t chunkEnd = offset + chunkSize;

        const TSNode rootNode = ts_tree_root_node(tree);
        state->setChunk(chunk, chunkStart, chunkEnd);
        STP_processChunk(rootNode, state);

        offset += chunkSize;
    }

    if (tree != nullptr)
        ts_tree_delete(tree);
    ts_parser_delete(parser);

    state->dbgPrintVariables();
    STP_destroy();

    return 0;
}
