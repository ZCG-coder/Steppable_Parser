#include "stpInterp/stpBetterTS.hpp"
#include "stpInterp/stpInit.hpp"
#include "stpInterp/stpStore.hpp"
#include "stpInterp/stpTypeName.hpp"

using namespace std::literals;

namespace steppable::parser
{
    void handleSymbolDeclStmt(const TSNode* node, const STP_InterpState& state)
    {
        TSNode nameNode = ts_node_child_by_field_name(*node, "sym_name"s);
        const std::string& name = state->getChunk(&nameNode);

        STP_LocalValue assignmentVal(STP_TypeID_SYMBOL);
        assignmentVal.data = name;
        assignmentVal.typeName = STP_typeNames.at(STP_TypeID_SYMBOL);
        state->getCurrentScope()->addVariable(name, assignmentVal);
    }
} // namespace steppable::parser