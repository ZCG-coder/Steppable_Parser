#include "output.hpp"
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

extern "C" TSLanguage* tree_sitter_stp();

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

    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_stp());
    TSTree* tree = nullptr;

    const STP_InterpState state = STP_getState();
    state->setFile(path);

    // Read the entire file at once
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file)
    {
        steppable::output::error("parser"s, "Unable to open file {0}"s, { path });
        ts_parser_delete(parser);
        return 1;
    }

    const std::string source((std::istreambuf_iterator(file)), std::istreambuf_iterator<char>());
    file.close();

    tree = ts_parser_parse_string(parser, nullptr, source.c_str(), source.size());
    TSNode rootNode = ts_tree_root_node(tree);
    state->setChunk(source, 0, static_cast<long>(source.size()));
    STP_processChunkChild(rootNode, state);

    if (tree != nullptr)
        ts_tree_delete(tree);
    ts_parser_delete(parser);

    STP_destroy();

    return 0;
}
