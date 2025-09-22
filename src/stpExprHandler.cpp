#include "steppable/mat2d.hpp"
#include "steppable/number.hpp"
#include "steppable/stpArgSpace.hpp"
#include "stpInterp/stpBetterTS.hpp"
#include "stpInterp/stpErrors.hpp"
#include "stpInterp/stpInit.hpp"
#include "stpInterp/stpStore.hpp"

#include <cassert>

namespace steppable::parser
{
    STP_Value handleExpr(const TSNode* exprNode,
                         const STP_InterpState& state,
                         const bool printResult = false,
                         const std::string& exprName = "")
    {
        assert(exprNode != nullptr);
        if (ts_node_type(*exprNode) == "ERROR"s or ts_node_is_missing(*exprNode))
            STP_throwSyntaxError(*exprNode, state);

        std::string exprType = ts_node_type(*exprNode);

        STP_Value retVal(STP_TypeID_NULL);

        if (exprType == "number")
        {
            // Number
            std::string data = state->getChunk(exprNode);
            retVal = STP_Value(STP_TypeID_NUMBER, Number(data));
        }
        if (exprType == "percentage")
        {
            // percentage := number "%"
            TSNode numberNode = ts_node_child(*exprNode, 0);
            std::string number = state->getChunk(&numberNode);
            Number value(number);
            value /= 100; // NOLINT(*-avoid-magic-numbers)

            retVal = STP_Value(STP_TypeID_NUMBER, value);
        }
        if (exprType == "matrix")
        {
            // Matrix
            std::unique_ptr<size_t> lastColLength;
            MatVec2D<Number> matVec;
            for (size_t j = 0; j < ts_node_child_count(*exprNode); j++)
            {
                TSNode node = ts_node_child(*exprNode, j);
                if (const std::string& nodeType = ts_node_type(node);
                    nodeType != "matrix_row" and nodeType != "matrix_row_last")
                    continue;

                size_t currentCols = 0;
                std::vector<Number> currentMatRow;
                for (size_t i = 0; i < ts_node_child_count(node); i++)
                {
                    TSNode cell = ts_node_child(node, i);
                    if (ts_node_type(cell) == ";"s)
                        continue;
                    STP_Value val = handleExpr(&cell, state);
                    if (val.typeID != STP_TypeID_NUMBER)
                    {
                        output::error("parser"s, "Matrix should contain numbers only."s);
                        __internals::utils::programSafeExit(1);
                    }
                    auto value = std::any_cast<Number>(val.data);
                    currentMatRow.emplace_back(value);
                    currentCols++;
                }

                if (lastColLength)
                {
                    if (currentCols != *lastColLength)
                    {
                        output::error("parser"s, "Inconsistent matrix dimensions."s);
                        __internals::utils::programSafeExit(1);
                    }
                }
                matVec.emplace_back(currentMatRow);
                lastColLength = std::make_unique<size_t>(currentCols);
            }

            Matrix data(matVec);
            retVal = STP_Value(STP_TypeID_MATRIX_2D, data);
        }
        if (exprType == "string")
        {
            // String
            std::string data;
            for (size_t i = 0; i < ts_node_child_count(*exprNode); i++)
            {
                auto childNode = ts_node_child(*exprNode, i);
                const std::string childNodeType = ts_node_type(childNode);

                if (childNodeType == "string_char")
                    data += state->getChunk(&childNode);
                else if (childNodeType == "unicode_escape")
                {
                    auto hexDigitsNode = ts_node_named_child(childNode, 0);
                    const std::string hexCode = state->getChunk(&hexDigitsNode);

                    int codePoint = std::stoul(hexCode, nullptr, 16);
                    std::string text = __internals::stringUtils::unicodeToUtf8(codePoint);
                    data += text;
                }
                else if (childNodeType == "octal_escape")
                {
                    std::string octDigits = state->getChunk(&childNode);
                    octDigits.erase(octDigits.begin()); // Erase leading '\' character

                    int codePoint = std::stoul(octDigits, nullptr, 8);
                    std::string text = __internals::stringUtils::unicodeToUtf8(codePoint);
                    data += text;
                }
                else if (childNodeType == "formatting_snippet")
                {
                    auto formatExprNode = ts_node_child_by_field_name(childNode, "formatting_expr"s);
                    STP_Value value = handleExpr(&formatExprNode, state, printResult, exprName);

                    data += value.present("", false);
                }
            }
            retVal = STP_Value(STP_TypeID_STRING, data);
        }
        else if (exprType == "identifier_or_member_access")
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
        else if (exprType == "function_call")
        {
            TSNode nameNode = ts_node_child(ts_node_child(*exprNode, 0), 0);
            std::string nameNodeType = ts_node_type(nameNode);

            if (nameNodeType == "identifier")
            {
                std::string funcName = state->getChunk(&nameNode);
                funcName = "STP_" + funcName;

                auto stpLib = state->getLoadedLib(0);
                auto funcPtr = stpLib->getSymbol(funcName);

                if (funcPtr == nullptr)
                {
                    output::error("runtime"s, "Function {0} is not defined."s, { funcName });
                    programSafeExit(1);
                }

                auto args = STP_ArgContainer({ STP_Argument("", Number("10"), STP_TypeID_NUMBER) }, {});
                auto* val = static_cast<STP_ValuePrimitive*>(funcPtr(&args));
                std::cout << val->present("") << std::endl;
                delete val;
            }
        }
        else
        {
            // Type is not specified by notation, need to infer.
            if (exprType == "binary_expression")
            {
                // binary_expression := lhs 'operator' rhs
                TSNode binExprNode = ts_node_child(*exprNode, 0);
                TSNode lhsNode = ts_node_child(binExprNode, 0);
                TSNode operandNode = ts_node_child(ts_node_child(binExprNode, 1), 0);
                TSNode rhsNode = ts_node_child(binExprNode, 2);

                std::string operandType = ts_node_type(operandNode);

                STP_Value lhs = handleExpr(&lhsNode, state);
                STP_Value rhs = handleExpr(&rhsNode, state);

                retVal = lhs.applyBinaryOperator(operandType, rhs);
            }
            if (exprType == "unary_expression")
            {
                TSNode operandNode = ts_node_child(*exprNode, 0);
                TSNode child = ts_node_child(*exprNode, 1);
                std::string operandType = ts_node_type(operandNode);

                STP_Value childVal = handleExpr(&child, state);
                retVal = childVal.applyUnaryOperator(operandType);
            }
            if (exprType == "bracketed_expr")
            {
                TSNode innerExpr = ts_node_child(*exprNode, 1);
                retVal = handleExpr(&innerExpr, state);
            }
        }

        if (printResult)
            std::cout << retVal.present(exprName) << '\n';

        return retVal;
    }
} // namespace steppable::parser