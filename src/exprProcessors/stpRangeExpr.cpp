#include "steppable/mat2d.hpp"
#include "stpInterp/stpBetterTS.hpp"
#include "stpInterp/stpInit.hpp"
#include "stpInterp/stpStore.hpp"
#include "tree_sitter/api.h"

using namespace steppable::__internals::stringUtils;

namespace steppable::parser
{
    STP_Value STP_handleRangeExpr(const TSNode* exprNode, const STP_InterpState& state)
    {
        TSNode startNode = ts_node_child_by_field_name(*exprNode, "start"s);
        TSNode stepNode = ts_node_child_by_field_name(*exprNode, "step"s);
        TSNode endNode = ts_node_child_by_field_name(*exprNode, "end"s);

        std::string start = state->getChunk(&startNode);

        std::string step;
        // default step is 1
        if (not ts_node_is_null(stepNode))
            step = state->getChunk(&stepNode);
        else
            step = "1";

        std::string end = state->getChunk(&endNode);

        start = bothEndsReplace(start, ' ');
        step = bothEndsReplace(step, ' ');
        end = bothEndsReplace(end, ' ');

        Number startN(start);
        Number stepN(step);
        Number endN(end);

        std::vector<Number> row;
        // include end as well
        for (Number i = startN; i <= endN; i += stepN)
            row.emplace_back(i);

        Matrix mat({ row });
        return STP_Value(STP_TypeID::MATRIX_2D, mat);
    }
} // namespace steppable::parser