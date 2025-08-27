#include "stpInterp/stpExprHandler.hpp"
#include "stpInterp/stpInit.hpp"
#include "tree_sitter/api.h"

namespace steppable::parser
{
     void handleAssignment(const TSNode* node, const STP_InterpState& state = nullptr)
    {
        // assignment := nameNode "=" exprNode
        const TSNode nameNode = ts_node_child(*node, 0);
        const TSNode exprNode = ts_node_child(*node, 2);

        const std::string name = state->getChunk(&nameNode);

        const STP_LocalValue val = handleExpr(&exprNode, state);
        // Write to scope / global variables
        state->addVariable(name, val);
    }
} // namespace my_namespace