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

            const TSNode posArgsNode = ts_node_next_named_sibling(fnNameNode);
            TSNode keywordArgsNode{};

            std::vector<std::string> posArgNames;
            STP_StringValMap keywordArgs;

            if (not ts_node_is_null(posArgsNode))
            {
                const size_t posArgsCount = ts_node_named_child_count(posArgsNode);

                for (size_t i = 0; i < posArgsCount; i++)
                {
                    TSNode paramNode = ts_node_named_child(posArgsNode, i);
                    std::string paramName = state->getChunk(&paramNode);
                    posArgNames.emplace_back(paramName);
                }

                keywordArgsNode = ts_node_next_named_sibling(posArgsNode);
            }
            if (not ts_node_is_null(keywordArgsNode))
            {
                const size_t keywordArgsCount = ts_node_named_child_count(posArgsNode);

                for (size_t i = 0; i < keywordArgsCount; i++)
                {
                    TSNode paramNode = ts_node_named_child(keywordArgsNode, i);
                    TSNode paramNameNode = ts_node_named_child(paramNode, 0);
                    TSNode paramValueNode = ts_node_named_child(paramNode, 1);

                    STP_Value defaultVal = handleExpr(&paramValueNode, state);
                    std::string paramName = state->getChunk(&paramNameNode);
                    keywordArgs.insert_or_assign(paramName, defaultVal);
                }
            }

            TSNode bodyNode = ts_node_child_by_field_name(node, "fn_body"s);

            STP_FunctionDefinition fn;
            fn.interpFn = [&](const STP_StringValMap& map) -> STP_Value {
                STP_Scope scope = state->addChildScope();

                // default return value
                scope.addVariable("04795", STP_Value(STP_TypeID_NUMBER, 0));

                scope.variables = map;
                state->setCurrentScope(std::make_shared<STP_Scope>(scope));

                STP_processChunkChild(bodyNode, state, false);
                STP_Value ret = state->getCurrentScope()->getVariable("04795");
                state->setCurrentScope(state->getCurrentScope()->parentScope);

                return ret;
            };
            fn.posArgNames = posArgNames;
            fn.keywordArgs = keywordArgs;

            state->getCurrentScope()->addFunction(fnName, fn);

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
            if (processChunk(ts_node_child(parent, i), stpState))
                break;

        if (createNewScope)
            stpState->setCurrentScope(newScope->parentScope);
    };
} // namespace steppable::parser