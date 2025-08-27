#include "stpInterp/stpExprHandler.hpp"
#include "stpInterp/stpInit.hpp"
#include "stpInterp/stpProcessor.hpp"
#include "tree_sitter/api.h"

using namespace std::literals;

namespace steppable::parser
{
    void STP_processIfElseStmt(const TSNode* node, const STP_InterpState& state)
    {
        // if_else_stmt := 'if'      expr '{' if_clause_stmt '}'
        //                 'else if' expr '{' statement '}'
        //                 'else'         '{' else_clause_stmt '}'
        TSNode exprNode = ts_node_named_child(*node, 0);
        STP_LocalValue res = handleExpr(&exprNode, state);
        TSNode ifClauseStmtNode = ts_node_next_named_sibling(exprNode);
        TSNode lastNode = ifClauseStmtNode;

        if (res.asBool())
        {
            STP_processChunkChild(ifClauseStmtNode, state, true);
            return;
        }

        if (ts_node_is_null(lastNode))
        {
            // Don't go further if there is no elseif and else statements
            return;
        }

        while (true)
        {
            TSNode elseifClauseNode = ts_node_next_named_sibling(lastNode);
            if (ts_node_is_null(elseifClauseNode))
                break;
            if (ts_node_type(elseifClauseNode) != "elseif_clause"s)
                break;
            lastNode = elseifClauseNode;

            TSNode elseifExprNode = ts_node_named_child(elseifClauseNode, 0);
            TSNode elseifClauseStmtNode = ts_node_named_child(elseifClauseNode, 1);
            res = handleExpr(&elseifExprNode, state);

            if (res.asBool())
            {
                STP_processChunkChild(elseifClauseStmtNode, state, true);
                return;
            }
        }

        // Handle else statement
        if (ts_node_is_null(lastNode))
        {
            // If there is no else statement, skip it
            return;
        }

        TSNode elseClauseNode = ts_node_next_named_sibling(lastNode);
        TSNode elseClauseStmtNode = ts_node_named_child(elseClauseNode, 0);
        STP_processChunkChild(elseClauseStmtNode, state, true);
    }
} // namespace steppable::parser