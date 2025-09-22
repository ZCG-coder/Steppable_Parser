#include "stpInterp/stpExprHandler.hpp"
#include "stpInterp/stpInit.hpp"
#include "tree_sitter/api.h"

#include <iostream>

using namespace std::literals;

namespace steppable::parser
{
    void handleAssignment(const TSNode* node, const STP_InterpState& state = nullptr)
    {
        // assignment := nameNode "=" exprNode
        const TSNode nameNode = ts_node_child(*node, 0);
        const TSNode exprNode = ts_node_child(*node, 2);

        const std::string name = state->getChunk(&nameNode);
        bool printValue = false;

        const TSNode semicolonSibling = ts_node_next_sibling(exprNode);
        if (ts_node_is_null(semicolonSibling))
            printValue = true; // NOLINT(*-branch-clone)
        else if (ts_node_type(semicolonSibling) != ";"s)
            printValue = true;

        const STP_Value val = handleExpr(&exprNode, state, printValue, name);
        // Write to scope / global variables
        state->getCurrentScope()->addVariable(name, val);
    }
} // namespace steppable::parser