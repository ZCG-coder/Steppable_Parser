#pragma once
#include "stpInit.hpp"
#include "stpStore.hpp"

namespace steppable::parser
{
    STP_Value handleExpr(const TSNode* exprNode,
                              const STP_InterpState& state,
                              bool printResult = false,
                              const std::string& exprName = "");
}