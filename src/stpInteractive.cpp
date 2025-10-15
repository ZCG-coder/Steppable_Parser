#include "stpInterp/stpInit.hpp"
#include "stpInterp/stpProcessor.hpp"

namespace steppable::parser
{
    int STP_startInteractiveMode(const STP_InterpState& state, TSParser* parser)
    {
        std::cout << "Steppable " STEPPABLE_VERSION "\n";
        std::cout << "Type an expression to evaluate. `exit` to quit.\n";

        std::string source;
        TSTree* tree = nullptr;

        while (1)
        {
            std::cout << ": ";
            source.clear();
            std::getline(std::cin, source);

            tree = ts_parser_parse_string(parser, nullptr, source.c_str(), static_cast<uint32_t>(source.size()));
            TSNode rootNode = ts_tree_root_node(tree);
            state->setChunk(source, 0, static_cast<long>(source.size()));
            STP_processChunkChild(rootNode, state);
        }
        return 0;
    }
} // namespace steppable::parser