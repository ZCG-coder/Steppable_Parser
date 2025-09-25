#include "stpInterp/stpExprHandler.hpp"

#include "steppable/mat2d.hpp"
#include "steppable/number.hpp"
#include "steppable/stpArgSpace.hpp"
#include "stpInterp/stpBetterTS.hpp"
#include "stpInterp/stpErrors.hpp"
#include "stpInterp/stpInit.hpp"
#include "stpInterp/stpStore.hpp"

#include <cassert>
#include <cstdint>

namespace steppable::parser
{
    STP_Value STP_handleExpr(const TSNode* exprNode,
                             const STP_InterpState& state,
                             const bool printResult,
                             const std::string& exprName)
    {
        assert(exprNode != nullptr);
        STP_checkRecursiveNodeSanity(*exprNode, state);

        std::string exprType = ts_node_type(*exprNode);

        STP_Value retVal(STP_TypeID::NONE);

        if (exprType == "number")
        {
            // Number
            std::string data = state->getChunk(exprNode);
            retVal = STP_Value(STP_TypeID::NUMBER, Number(data));
        }
        if (exprType == "percentage")
        {
            // percentage := number "%"
            TSNode numberNode = ts_node_child(*exprNode, 0);
            std::string number = state->getChunk(&numberNode);
            Number value(number);
            value /= 100; // NOLINT(*-avoid-magic-numbers)

            retVal = STP_Value(STP_TypeID::NUMBER, value);
        }
        if (exprType == "matrix")
            retVal = STP_handleMatrixExpr(exprNode, state);
        if (exprType == "string")
            retVal = STP_handleStringExpr(exprNode, state);
        if (exprType == "identifier_or_member_access")
        {
            TSNode childNode = ts_node_child(*exprNode, 0);
            std::string childNodeType = ts_node_type(childNode);

            // Get the variable
            if (childNodeType == "identifier")
            {
                std::string nameNode = state->getChunk(&childNode);
                retVal = state->getCurrentScope()->getVariable(nameNode);
            }
        }
        if (exprType == "function_call")
            retVal = STP_processFnCall(exprNode, state);
        if (exprType == "binary_expression")
        {
            // binary_expression := lhs 'operator' rhs
            TSNode binExprNode = ts_node_child(*exprNode, 0);
            TSNode lhsNode = ts_node_child(binExprNode, 0);
            TSNode operandNode = ts_node_child(ts_node_child(binExprNode, 1), 0);
            TSNode rhsNode = ts_node_child(binExprNode, 2);

            std::string operandType = ts_node_type(operandNode);

            STP_Value lhs = STP_handleExpr(&lhsNode, state);
            STP_Value rhs = STP_handleExpr(&rhsNode, state);

            retVal = lhs.applyBinaryOperator(operandType, rhs);
        }
        if (exprType == "unary_expression")
        {
            TSNode operandNode = ts_node_child(*exprNode, 0);
            TSNode child = ts_node_child(*exprNode, 1);
            std::string operandType = ts_node_type(operandNode);

            STP_Value childVal = STP_handleExpr(&child, state);
            retVal = childVal.applyUnaryOperator(operandType);
        }
        if (exprType == "bracketed_expr")
        {
            TSNode innerExpr = ts_node_child(*exprNode, 1);
            retVal = STP_handleExpr(&innerExpr, state);
        }

        if (printResult)
            std::cout << retVal.present(exprName) << '\n';

        return retVal;
    }

    std::vector<STP_Argument> STP_extractArgVector(const TSNode* exprNode, const STP_InterpState& state)
    {
        std::vector<STP_Argument> fnArgsVec;
        TSNode posArgumentsNode = ts_node_named_child(*exprNode, 1);
        if (not ts_node_is_null(posArgumentsNode))
        {
            uint32_t posArgumentsCount = ts_node_named_child_count(posArgumentsNode);

            for (uint32_t i = 0; i < posArgumentsCount; i++)
            {
                TSNode argNode = ts_node_named_child(posArgumentsNode, i);
                STP_Value res = STP_handleExpr(&argNode, state);
                STP_Argument argument("", res.data, res.typeID);
                fnArgsVec.emplace_back(argument);
            }

            TSNode kwArgumentsNode = ts_node_named_child(*exprNode, 2);
            if (not ts_node_is_null(kwArgumentsNode))
            {
                uint32_t keywordArgumentCount = ts_node_named_child_count(kwArgumentsNode);
                for (uint32_t i = 0; i < keywordArgumentCount; i++)
                {
                    TSNode argNode = ts_node_named_child(kwArgumentsNode, i);
                    TSNode argNameNode = ts_node_child_by_field_name(argNode, "argument_name"s);
                    TSNode argExprNode = ts_node_next_named_sibling(argNameNode);

                    std::string argName = state->getChunk(&argNameNode);

                    STP_Value res = STP_handleExpr(&argExprNode, state);
                    STP_Argument argument(argName, res.data, res.typeID);
                    fnArgsVec.emplace_back(argument);
                }
            }
        }

        return fnArgsVec;
    }

} // namespace steppable::parser
