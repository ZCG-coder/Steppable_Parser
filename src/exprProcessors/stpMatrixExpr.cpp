#include "steppable/mat2d.hpp"
#include "steppable/number.hpp"
#include "steppable/stpArgSpace.hpp"
#include "stpInterp/stpBetterTS.hpp"
#include "stpInterp/stpErrors.hpp"
#include "stpInterp/stpExprHandler.hpp"
#include "stpInterp/stpInit.hpp"
#include "stpInterp/stpStore.hpp"

#include <cassert>
#include <cstdint>

namespace steppable::parser
{
    STP_Value STP_handleMatrixExpr(const TSNode* exprNode, const STP_InterpState& state)
    {
        // Matrix
        std::unique_ptr<size_t> lastColLength;
        MatVec2D<Number> matVec;
        for (uint32_t j = 0; j < ts_node_child_count(*exprNode); j++)
        {
            TSNode node = ts_node_child(*exprNode, j);
            if (const std::string& nodeType = ts_node_type(node);
                nodeType != "matrix_row" and nodeType != "matrix_row_last")
                continue;

            size_t currentCols = 0;
            std::vector<Number> currentMatRow;
            for (uint32_t i = 0; i < ts_node_child_count(node); i++)
            {
                TSNode cell = ts_node_child(node, i);
                if (ts_node_type(cell) == ";"s)
                    continue;
                STP_Value val = STP_handleExpr(&cell, state);

                if (val.typeID != STP_TypeID::NUMBER)
                    STP_throwError(cell, state, "Matrix should contain numbers only."s);

                auto value = std::any_cast<Number>(val.data);
                currentMatRow.emplace_back(value);
                currentCols++;
            }

            if (lastColLength)
            {
                if (currentCols != *lastColLength)
                    STP_throwError(node, state, "Matrix should contain numbers only."s);
            }
            matVec.emplace_back(currentMatRow);
            lastColLength = std::make_unique<size_t>(currentCols);
        }

        Matrix data(matVec);
        return STP_Value(STP_TypeID::MATRIX_2D, data);
    }
} // namespace steppable::parser
