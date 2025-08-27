#pragma once
#include "stpInit.hpp"
#include "stpStore.hpp"

namespace steppable::parser
{
    STP_LocalValue handleExpr(const TSNode* exprNode, const STP_InterpState& state, bool printResult = false);
}