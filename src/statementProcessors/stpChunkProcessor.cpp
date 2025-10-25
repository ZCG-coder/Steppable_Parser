/**************************************************************************************************
 * Copyright (c) 2023-2025 NWSOFT                                                                 *
 *                                                                                                *
 * Permission is hereby granted, free of charge, to any person obtaining a copy                   *
 * of this software and associated documentation files (the "Software"), to deal                  *
 * in the Software without restriction, including without limitation the rights                   *
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell                      *
 * copies of the Software, and to permit persons to whom the Software is                          *
 * furnished to do so, subject to the following conditions:                                       *
 *                                                                                                *
 * The above copyright notice and this permission notice shall be included in all                 *
 * copies or substantial portions of the Software.                                                *
 *                                                                                                *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR                     *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,                       *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE                    *
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER                         *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,                  *
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE                  *
 * SOFTWARE.                                                                                      *
 **************************************************************************************************/

#include "output.hpp"
#include "platform.hpp"
#include "steppable/mat2dBase.hpp"
#include "stpInterp/stpBetterTS.hpp"
#include "stpInterp/stpErrors.hpp"
#include "stpInterp/stpExprHandler.hpp"
#include "stpInterp/stpProcessor.hpp"
#include "subprocess.hpp"

#include <iostream>
#include <string>
#include <tree_sitter/api.h>

using namespace std::literals;
using namespace steppable::utils;

namespace steppable::parser
{
    void processChunk(const TSNode& node, const STP_InterpState& state)
    {
        const std::string type = ts_node_type(node);

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
            try
            {
                STP_IPC ipc("/stp-4795", 128, false);
                char* mem = static_cast<char*>(ipc.data());
                strncpy(mem, "exit", 5);
                ipc.flush();
            }
            catch (const std::runtime_error&)
            {
                output::error("parser"s, "Unable to set IPC exit flag!"s);
            }
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
                STP_throwError(node, state, "No statements in while loop"s);
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
            name = stringUtils::bothEndsReplace(name, ' ');

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
                STP_throwError(
                    node, state, format::format("Object of type {0} is not itertable"s, { exprValue.typeName }));
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
