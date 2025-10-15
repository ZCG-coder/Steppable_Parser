#pragma once
#include "stpInit.hpp"
#include "stpStore.hpp"

namespace steppable::parser
{
    STP_Value STP_handleStringExpr(const TSNode* exprNode, const STP_InterpState& state);

    STP_Value STP_handleMatrixExpr(const TSNode* exprNode, const STP_InterpState& state);

    STP_Value STP_handleRangeExpr(const TSNode* exprNode, const STP_InterpState& state);

    STP_Value STP_handleSuffixExpr(const TSNode* exprNode, const STP_InterpState& state);

    std::vector<STP_Argument> STP_extractArgVector(const TSNode* exprNode, const STP_InterpState& state);

    STP_Value STP_processFnCall(const TSNode* exprNode, const STP_InterpState& state);

    STP_Value STP_handleExpr(const TSNode* exprNode,
                         const STP_InterpState& state,
                         bool printResult = false,
                         const std::string& exprName = "");
} // namespace steppable::parser