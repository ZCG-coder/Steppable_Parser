#pragma once

#include "stpInterp/stpInit.hpp"

namespace steppable::parser
{
    void STP_processChunk(const TSNode& node, const STP_InterpState& state = nullptr);

    void STP_processChunkChild(const TSNode& parent, const STP_InterpState& stpState, bool createNewScope = false);

    // Statement processors
    void handleAssignment(const TSNode* node, const STP_InterpState& state = nullptr);

    void STP_processIfElseStmt(const TSNode* node, const STP_InterpState& state);

    void handleSymbolDeclStmt(const TSNode* node, const STP_InterpState& state);

} // namespace steppable::parser