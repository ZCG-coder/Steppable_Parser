#include "stpInterp/stpErrors.hpp"

#include "output.hpp"
#include "stpInterp/stpInit.hpp"
#include "util.hpp"

namespace steppable::parser
{
    using namespace steppable::__internals;

    void STP_throwSyntaxError(const TSNode& node, const STP_InterpState& state)
    {
        output::error("parser"s, "Syntax error"s);
        auto [startRow, startCol] = ts_node_start_point(node);
        auto [endRow, endCol] = ts_node_end_point(node);

        output::error("parser"s,
                                 "At {0} : Ln {1}, Col {2}"s,
                                 {
                                     state->getFile(),
                                     std::to_string(startRow + 1),
                                     std::to_string(startCol),
                                 });
        std::string errorChunk = state->getChunk(&node);
        auto lines = stringUtils::split(errorChunk, '\n');

        const auto& endLineLen = static_cast<size_t>(log10(endRow + 1) + 1);
        const auto& startLineLen = static_cast<size_t>(log10(startRow + 1) + 1);
        const size_t& padding = std::max(startLineLen, endLineLen);

        for (size_t i = 0; i < lines.size(); i++)
        {
            const auto& line = lines.at(i);
            std::string lineNo = stringUtils::lPad(std::to_string(startRow + i + 1), padding);
            output::info("parser"s,
                         "{0}  | {1}"s,
                         {
                             lineNo,
                             line,
                         });
        }

        utils::programSafeExit(1);
    }
} // namespace steppable::parser