#pragma once

#include "stpInterp/stpInit.hpp"

namespace steppable::parser
{
    void STP_checkRecursiveNodeSanity(const TSNode& node, const STP_InterpState& state);

    void STP_throwSyntaxError(const TSNode& node, const STP_InterpState& state);

    void STP_throwError(const TSNode& node, const STP_InterpState& state, const std::string& reason);
} // namespace steppable::parser