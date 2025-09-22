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
    bool processChunk(const TSNode& node, const STP_InterpState& state)
    {
        if (ts_node_is_error(node) or ts_node_is_missing(node))
            STP_throwSyntaxError(node, state);

        const std::string type = ts_node_type(node);

        if (type == "\n" // Newline
            or type == "comment" // Comment
        )
        {
            return false;
        }

        if (type == "return_stmt")
        {
            // Handle return statement
            TSNode exprNode = ts_node_child_by_field_name(node, "ret_expr"s);
            STP_Value val = handleExpr(&exprNode, state);

            // By writing to a illegally-named variable,
            state->getCurrentScope()->addVariable("04795", val);
            return true;
        }

        // Handle scoped statements before assignment statements
        if (type == "function_definition")
        {
            TSNode fnNameNode = ts_node_child_by_field_name(node, "fn_name"s);
            std::string fnName = state->getChunk(&fnNameNode);

            const TSNode argsNode = ts_node_child_by_field_name(node, "parameter_list"s);
            const size_t argsCount = ts_node_child_count(argsNode);

            std::vector<std::string> argNames;
            for (size_t i = 0; i < argsCount; i++)
            {
                TSNode paramNode = ts_node_child(argsNode, i);
                if (ts_node_type(paramNode) != "param_name"s)
                    continue;
                std::string paramName = state->getChunk(&paramNode);
                argNames.emplace_back(paramName);
            }

            TSNode bodyNode = ts_node_child_by_field_name(node, "fn_body"s);

            STP_FunctionDefinition fn;
            fn.interpFn = [&](const STP_StringValMap& map) {
                STP_Scope scope = state->addChildScope();
                scope.variables = map;
                state->setCurrentScope(std::make_shared<STP_Scope>(scope));

                STP_processChunkChild(bodyNode, state, false);

                state->setCurrentScope(state->getCurrentScope()->parentScope);
            };
            fn.argNames = argNames;

            return false;
        }
        if (type == "if_else_stmt")
        {
            STP_processIfElseStmt(&node, state);
            return false;
        }
        if (type == "while_stmt")
        {
            const TSNode exprNode = ts_node_next_named_sibling(node);
            std::cout << ts_node_type(exprNode) << "\n";
            return false;
        }
        if (type == "foreach_in_stmt")
        {
            //
            return false;
        }
        if (type == "assignment")
        {
            handleAssignment(&node, state);
            return false;
        }
        if (type == "expression_statement")
        {
            const TSNode exprNode = ts_node_child(node, 0);
            handleExpr(&exprNode, state, true);
            return false;
        }
        if (type == "import_statement")
        {
            //
            return false;
        }
        if (type == "symbol_decl_statement")
        {
            handleSymbolDeclStmt(&node, state);
            return false;
        }

        STP_processChunkChild(node, state);
        return false;
    }

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
        {
            if (processChunk(ts_node_child(parent, i), stpState))
                break;
        }

        if (createNewScope)
            stpState->setCurrentScope(newScope->parentScope);
    };
} // namespace steppable::parser