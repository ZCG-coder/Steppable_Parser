#pragma once

#include "stpInterp/stpInit.hpp"

namespace steppable::parser
{
    void STP_checkRecursiveNodeSanity(const TSNode& node, const STP_InterpState& state);

    void STP_throwSyntaxError(const TSNode& node, const STP_InterpState& state);
} // namespace steppable::parser