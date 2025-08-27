#include "stpInterp/stpBetterTS.hpp"
#include "stpInterp/stpErrors.hpp"
#include "stpInterp/stpExprHandler.hpp"
#include "stpInterp/stpProcessor.hpp"
#include "tree_sitter/api.h"

#include <iostream>
#include <string>

using namespace std::literals;

namespace steppable::parser
{
    void STP_processChunkChild(const TSNode& parent, const STP_InterpState& stpState, const bool createNewScope)
    {
        std::shared_ptr<STP_Scope> newScope;
        if (createNewScope)
        {
            newScope = std::make_shared<STP_Scope>(stpState->addChildScope());
            stpState->setCurrentScope(newScope);
        }

        if (ts_node_is_null(parent))
            return;

        const size_t childCount = ts_node_child_count(parent);
        for (uint32_t i = 0; i < childCount; ++i)
            STP_processChunk(ts_node_child(parent, i), stpState);

        if (createNewScope)
            stpState->setCurrentScope(newScope->parentScope);
    };

    void STP_processChunk(const TSNode& node, const STP_InterpState& state)
    {
        if (ts_node_is_error(node) or ts_node_is_missing(node))
            STP_throwSyntaxError(node, state);

        const std::string type = ts_node_type(node);

        if (type == "\n" // Newline
            or type == "comment" // Comment
        )
        {
            return;
        }

        // Handle scoped statements before assignment statements
        if (type == "function_definition")
        {
            TSNode fnNameNode = ts_node_child_by_field_name(node, "fn_name"s);
            std::string fnName = state->getChunk(&fnNameNode);

            std::cout << fnName << std::endl;
            return;
        }
        if (type == "if_else_stmt")
        {
            STP_processIfElseStmt(&node, state);
            return;
        }
        if (type == "while_stmt")
        {
            const TSNode exprNode = ts_node_next_named_sibling(node);
            std::cout << ts_node_type(exprNode) << "\n";
            return;
        }
        if (type == "foreach_in_stmt")
        {
            //
            return;
        }
        if (type == "assignment")
        {
            handleAssignment(&node, state);
            return;
        }
        if (type == "expression_statement")
        {
            const TSNode exprNode = ts_node_child(node, 0);
            handleExpr(&exprNode, state, true);
            return;
        }
        if (type == "import_statement")
        {
            //
            return;
        }
        if (type == "symbol_decl_statement")
        {
            handleSymbolDeclStmt(&node, state);
            return;
        }

        STP_processChunkChild(node, state);
    }
} // namespace steppable::parser