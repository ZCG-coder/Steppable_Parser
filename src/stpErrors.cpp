#include "stpInterp/stpErrors.hpp"

#include "output.hpp"
#include "stpInterp/stpInit.hpp"
#include "util.hpp"

#include <cmath>

namespace steppable::parser
{
    using namespace steppable::__internals;

    void STP_checkRecursiveNodeSanity(const TSNode& node, const STP_InterpState& state)
    {
        if (ts_node_is_null(node))
            return;

        if (ts_node_is_error(node) or ts_node_is_missing(node))
            STP_throwSyntaxError(node, state);

        for (uint32_t i = 0; i < ts_node_child_count(node); i++)
        {
            TSNode child = ts_node_child(node, i);
            STP_checkRecursiveNodeSanity(child, state);
        }
    }

    void STP_throwError(const TSNode& node, const STP_InterpState& state, const std::string& reason)
    {
        output::error("parser"s, reason);

        auto [startRow, startCol] = ts_node_start_point(node);
        auto [endRow, endCol] = ts_node_end_point(node);

        output::error("parser"s,
                      "At {0} : Ln {1}, Col {2}"s,
                      {
                          state->getFile(),
                          std::to_string(startRow + 1),
                          std::to_string(startCol),
                      });
        std::string errorChunk = state->getChunk();
        auto lines = stringUtils::split(errorChunk, '\n');
        lines.erase(lines.begin(), lines.begin() + startRow);
        lines.erase(lines.begin() + (endRow - startRow + 1), lines.end());

        const auto& endLineLen = static_cast<size_t>(log10(endRow + 1) + 1);
        const auto& startLineLen = static_cast<size_t>(log10(startRow + 1) + 1);
        const size_t& padding = std::max(startLineLen, endLineLen);

        for (size_t i = 0; i < lines.size(); i++)
        {
            const auto& line = lines.at(i);
            std::string lineNo = stringUtils::lPad(std::to_string(startRow + i + 1), padding);
            std::string indicators = std::string(startCol, ' ') + std::string(endCol - startCol, '~');
            output::info("parser"s,
                         "{0}  | {1}"s,
                         {
                             lineNo,
                             line,
                         });
            output::info("parser"s, "{0}  | {1}"s, { std::string(lineNo.length(), ' '), indicators });
        }

        if (not state->isInteractive())
            programSafeExit(1);
    }

    void STP_throwSyntaxError(const TSNode& node, const STP_InterpState& state)
    {
        STP_throwError(node, state, "Syntax error"s);
    }
} // namespace steppable::parser