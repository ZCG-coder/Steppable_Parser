#include "steppable/mat2d.hpp"
#include "stpInterp/stpBetterTS.hpp"
#include "stpInterp/stpExprHandler.hpp"
#include "stpInterp/stpInit.hpp"
#include "stpInterp/stpStore.hpp"

namespace steppable::parser
{
    STP_Value STP_handleSuffixExpr(const TSNode* exprNode, const STP_InterpState& state)
    {
        TSNode opNode = ts_node_child_by_field_name(*exprNode, "operator"s);
        TSNode valueNode = ts_node_prev_sibling(opNode);
        STP_Value value = STP_handleExpr(&valueNode, state);

        STP_Value retValue(value.typeID, value.data);

        char op = *ts_node_type(opNode);

        switch (op)
        {
        case '\'':
        {
            // Matrix transpose
            if (value.typeID != STP_TypeID::MATRIX_2D)
            {
                output::error("runtime"s, "Cannot perform transpose on a non-matrix object"s);
                programSafeExit(1);
            }

            auto mat = std::any_cast<Matrix>(value.data);
            mat = mat.transpose();
            retValue = STP_Value(STP_TypeID::MATRIX_2D, mat);
            break;
        }
        case '!':
        {
            // Factorial
            if (value.typeID == STP_TypeID::NUMBER)
            {
                auto num = std::any_cast<Number>(value.data);
                num = __internals::calc::factorial(num.present(), 0);
                retValue = STP_Value(STP_TypeID::NUMBER, num);
            }
            else if (value.typeID == STP_TypeID::MATRIX_2D)
            {
                auto mat = std::any_cast<Matrix>(value.data);
                mat = mat.apply([&](const Number& item, const YXPoint& /*unused*/) -> Number {
                    return __internals::calc::factorial(item.present(), 0);
                });
                retValue = STP_Value(STP_TypeID::MATRIX_2D, mat);
            }
            else
            {
                output::error("runtime"s, "Factorial can only be applied to matrices and numbers"s);
                programSafeExit(1);
            }
            break;
        }
        default:
        {
            // Should not reach here
        }
        }
        return retValue;
    }
} // namespace steppable::parser