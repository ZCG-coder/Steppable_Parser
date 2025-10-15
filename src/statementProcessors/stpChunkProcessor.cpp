#include "output.hpp"
#include "platform.hpp"
#include "steppable/mat2dBase.hpp"
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
    void processChunk(const TSNode& node, const STP_InterpState& state)
    {
        const std::string type = ts_node_type(node);

        if (type == "source_file")
            STP_checkRecursiveNodeSanity(node, state);

        if (type == "\n" // Newline
            or type == "comment" // Comment
        )
        {
            return;
        }

        if (type == "return_stmt")
        {
            // Handle return statement
            TSNode exprNode = ts_node_child_by_field_name(node, "ret_expr"s);
            STP_Value val = STP_handleExpr(&exprNode, state);

            // By writing to a illegally-named variable,
            state->getCurrentScope()->addVariable("04795", val);
            state->setExecState(STP_ExecState::RETURNED);
            return;
        }

        if (type == "cont")
        {
            state->setExecState(STP_ExecState::CONT);
            return;
        }

        if (type == "break")
        {
            state->setExecState(STP_ExecState::BREAK);
            return;
        }

        if (type == "exit")
        {
            programSafeExit(0);
            return;
        }

        // Handle scoped statements before assignment statements
        if (type == "function_definition")
        {
            STP_processFuncDefinition(&node, state);
            return;
        }
        if (type == "if_else_stmt")
        {
            STP_processIfElseStmt(&node, state);
            return;
        }
        if (type == "while_stmt")
        {
            const TSNode exprNode = ts_node_child_by_field_name(node, "loop_expr"s);
            const TSNode bodyNode = ts_node_next_named_sibling(exprNode);

            if (ts_node_is_null(bodyNode))
            {
                output::error("runtime"s, "No statements in while loop"s);
                programSafeExit(1);
            }

            // Create one scope for the entire loop body
            auto loopScope = std::make_shared<STP_Scope>(state->addChildScope());
            state->setCurrentScope(loopScope);

            STP_Value loopVal = STP_handleExpr(&exprNode, state);
            while (true)
            {
                loopVal = STP_handleExpr(&exprNode, state);
                if (not loopVal.asBool(&exprNode))
                    break;

                processChunk(bodyNode, state);

                // Loop flow control
                if (state->getExecState() == STP_ExecState::CONT)
                {
                    state->setExecState(STP_ExecState::NORMAL);
                    continue;
                }
                if (state->getExecState() == STP_ExecState::BREAK)
                {
                    state->setExecState(STP_ExecState::NORMAL);
                    break;
                }
            }

            // Restore the parent scope after the entire loop
            state->setCurrentScope(loopScope->parentScope);
            return;
        }
        if (type == "foreach_in_stmt")
        {
            //
            TSNode nameNode = ts_node_child_by_field_name(node, "loop_var"s);
            std::string name = state->getChunk(&nameNode);
            name = __internals::stringUtils::bothEndsReplace(name, ' ');

            TSNode exprNode = ts_node_child_by_field_name(node, "loop_expr"s);
            STP_Value exprValue = STP_handleExpr(&exprNode, state);

            switch (exprValue.typeID)
            {
            case STP_TypeID::MATRIX_2D:
            {
                std::vector<STP_Value> source;
                STP_Value loopVar(STP_TypeID::MATRIX_2D, 0);
                break;
            }
            case STP_TypeID::STRING:
            {
                STP_Value loopVar(STP_TypeID::STRING, ""s);
                break;
            }
            default:
            {
                output::error("runtime"s, "Object of type {0} is not itertable"s, { exprValue.typeName });
                break;
            }
            }
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
            STP_handleExpr(&exprNode, state, true);
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
            processChunk(ts_node_child(parent, i), stpState);
            if (stpState->getExecState() == STP_ExecState::RETURNED)
            {
                stpState->setExecState(STP_ExecState::NORMAL);
                break;
            }
        }

        if (createNewScope)
            stpState->setCurrentScope(newScope->parentScope);
    };
} // namespace steppable::parser